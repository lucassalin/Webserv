/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerManager.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lsalin <lsalin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/05 15:05:48 by lsalin            #+#    #+#             */
/*   Updated: 2023/05/25 10:36:42 by lsalin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ServerManager.hpp"

ServerManager::ServerManager() {}
ServerManager::~ServerManager() {}

// Initialise tous les serveurs sur les ports spécifiés dans le fichier de configuration
void	ServerManager::setupServers(std::vector<ServerConfig> servers)
{
	std::cout << std::endl;
	Logger::logMsg(LIGHTMAGENTA, CONSOLE_OUTPUT, "Initializing  Servers...");

	_servers = servers;

	// buf = adresse IP du serveur
	// INET_ADDRSTRLEN = taille max adresse IP Ipv4 (16 caractères)
	char	buf[INET_ADDRSTRLEN];

	// Indique si un serveur utilise la même IP/port/fd qu'un autre
	// true = oui --> le fd est déjà attribué au serveur, on attribue le fd du serveur précédent
	// false = non --> le serveur doit être init avec un nouveau fd
	bool	serverDub;

	// Première boucle = serveur actuel
	// Deuxième boucle = serveur précédent

	for (std::vector<ServerConfig>::iterator it = _servers.begin(); it != _servers.end(); ++it)
	{
		serverDub = false;

		for (std::vector<ServerConfig>::iterator it2 = _servers.begin(); it2 != it; ++it2)
		{
			// les deux serveurs ont la même ad et le même port --> ils écoutent sur la même interface réseau
			// ils doivent donc partager le même fd
			if (it2->getHost() == it->getHost() && it2->getPort() == it->getPort())
			{
				it->setFd(it2->getFd()); // on attribue le fd du serveur précédent au serveur courant
				serverDub = true;
			}
		}

		if (!serverDub)
			it->setupServer();

		// inet_ntop() convertit une adresse IPv4 binaire en une string lisible par l'homme
		Logger::logMsg(LIGHTMAGENTA, CONSOLE_OUTPUT, "Server Created: ServerName[%s] Host[%s] Port[%d]",it->getServerName().c_str(),
				inet_ntop(AF_INET, &it->getHost(), buf, INET_ADDRSTRLEN), it->getPort());
	}
}

/**
	@brief Boucle principale du serveur : surveille les fd associés aux serveurs & clients
	et gére les événements associès à ces fd.

	Check les fd renvoyés par select() :

	1) Lecture :
		- si server fd --> accepte le nouveau client
		- si client fd --> le serveur traite la requête du client

	2) Écriture :
			- si réponse CGI et que le body n'est toujours pas envoyé au processus enfant CGI
			   --> envoyé le body de la requête au processus enfant du CGI
			- si réponse CGI et body envoyé --> lire la sortie du processus enfant CGI
			- si réponse normale --> l'envoyer au client

	Les sockets serveurs/clients sont initialement add à _recv_set_pool.
	Puis, lorsqu'une requête est entièrement parsée, le socket est déplacé vers _write_set_pool
*/

void	ServerManager::runServers()
{
	fd_set	recv_set_cpy;
	fd_set	write_set_cpy;
	int		select_ret;

	_biggest_fd = 0;
	initializeSets(); // initialise l'ensemble des sockets a surveiller

	struct timeval	timer;

	while (true)
	{
		timer.tv_sec = 1;
		timer.tv_usec = 0;
		recv_set_cpy = _recv_fd_pool;
		write_set_cpy = _write_fd_pool;

		if ((select_ret = select(_biggest_fd + 1, &recv_set_cpy, &write_set_cpy, NULL, &timer)) < 0 )
		{
			Logger::logMsg(RED, CONSOLE_OUTPUT, "webserv: select error %s   Closing ....", strerror(errno));
			exit(1);
			continue ;
		}

		for (int i = 0; i <= _biggest_fd; ++i)
		{
			// si le fd est prêt pour une opération de lecture et que c'est un fd de serveur
			// --> on accepte la connexion
			if (FD_ISSET(i, &recv_set_cpy) && _servers_map.count(i))
				acceptNewConnection(_servers_map.find(i)->second);

			// si fd de client --> le serveur lit la requête
			else if (FD_ISSET(i, &recv_set_cpy) && _clients_map.count(i))
				readRequest(i, _clients_map[i]);

			// écriture
			else if (FD_ISSET(i, &write_set_cpy) && _clients_map.count(i))
			{
				int cgi_state = _clients_map[i].response.getCgiState();
				
				// Si cgi_state = 1 --> lecture/écriture à partir du script CGI
				// Et que que le fd d'entrée du pipe est prêt pour l'écriture
				// --> on envoie les données au script CGI
				if (cgi_state == 1 && FD_ISSET(_clients_map[i].response._cgi_obj.pipe_in[1], &write_set_cpy))
					sendCgiBody(_clients_map[i], _clients_map[i].response._cgi_obj);

				// si fd d'entrée du pipe est prêt pour la lecture
				// --> on lit la réponse du script CGI
				else if (cgi_state == 1 && FD_ISSET(_clients_map[i].response._cgi_obj.pipe_out[0], &recv_set_cpy))
					readCgiResponse(_clients_map[i], _clients_map[i].response._cgi_obj);

				// Si 0 = pas de CGI & 2 = lecture/écriture CGI terminée
				// Et que fd prêt pour écriture
				// --> on envoie la réponse au client
				else if ((cgi_state == 0 || cgi_state == 2)  && FD_ISSET(i, &write_set_cpy))
					sendResponse(i, _clients_map[i]);
			}
		}
		checkTimeout();
	}
}

// Check le temps ecoule depuis le dernier message du client
// Si > CONNECTION_TIMEOUT --> close

void	ServerManager::checkTimeout()
{
	for (std::map<int, Client>::iterator it = _clients_map.begin() ; it != _clients_map.end(); ++it)
	{
		if (time(NULL) - it->second.getLastTime() > CONNECTION_TIMEOUT)
		{
			Logger::logMsg(YELLOW, CONSOLE_OUTPUT, "Client %d Timeout, Closing Connection..", it->first);
			closeConnection(it->first);
			return;
		}
	}
}

// Initialise les ensembles de fd lecture/écriture
// Et ajoute tous les sockets d'écoute à _recv_fd_pool
// Utilisée lors de l'initialisation des serveurs

void	ServerManager::initializeSets()
{
	FD_ZERO(&_recv_fd_pool);
	FD_ZERO(&_write_fd_pool);

	// ajoute les sockets des serveurs à l'ensemble de fd _recv_fd_pool
	for(std::vector<ServerConfig>::iterator it = _servers.begin(); it != _servers.end(); ++it)
	{
		if (listen(it->getFd(), 512) == -1)
		{
			Logger::logMsg(RED, CONSOLE_OUTPUT, "webserv: listen error: %s   Closing....", strerror(errno));
			exit(EXIT_FAILURE);
		}

		// set le fd en mode non bloquant
		if (fcntl(it->getFd(), F_SETFL, O_NONBLOCK) < 0)
		{
			Logger::logMsg(RED, CONSOLE_OUTPUT, "webserv: fcntl error: %s   Closing....", strerror(errno));
			exit(EXIT_FAILURE);
		}

		addToSet(it->getFd(), _recv_fd_pool);
		// associe le fd du serveur à sa config correspondante
		_servers_map.insert(std::make_pair(it->getFd(), *it));
	}
	// _biggest_fd contient à ce stade le fd du dernier serveur crée
	_biggest_fd = _servers.back().getFd();
}

// Accepte la nouvelle connexion d'un client
// Et gère les operations nécessaires pour son ajout à la liste des clients co au serveur

void	ServerManager::acceptNewConnection(ServerConfig &serv)
{
	struct sockaddr_in	client_address;
	long				client_address_size = sizeof(client_address);
	int					client_sock;
	Client				new_client(serv);
	char				buf[INET_ADDRSTRLEN];

	if ((client_sock = accept(serv.getFd(), (struct sockaddr *)&client_address,
	 (socklen_t*)&client_address_size)) == -1)
	{
		Logger::logMsg(RED, CONSOLE_OUTPUT, "webserv: accept error %s", strerror(errno));
		return;
	}

	Logger::logMsg(YELLOW, CONSOLE_OUTPUT, "New Connection From %s, Assigned Socket %d",inet_ntop(AF_INET, &client_address, buf, INET_ADDRSTRLEN), client_sock);

	addToSet(client_sock, _recv_fd_pool);

	if (fcntl(client_sock, F_SETFL, O_NONBLOCK) < 0)
	{
		Logger::logMsg(RED, CONSOLE_OUTPUT, "webserv: fcntl error %s", strerror(errno));
		removeFromSet(client_sock, _recv_fd_pool);
		close(client_sock);
		return;
	}

	new_client.setSocket(client_sock);

	// si un client qui a deja une co ouverte essaye de se co a nouveau
	// --> l'ancienne co est supprimee et remplacee par la nouvelle

	if (_clients_map.count(client_sock) != 0)
		_clients_map.erase(client_sock);

	_clients_map.insert(std::make_pair(client_sock, new_client));
}

// Ferme la connexion du fd "i" et supprime l'objet Client associe de _clients_map
void	ServerManager::closeConnection(const int i)
{
	if (FD_ISSET(i, &_write_fd_pool))
		removeFromSet(i, _write_fd_pool);

	if (FD_ISSET(i, &_recv_fd_pool))
		removeFromSet(i, _recv_fd_pool);

	close(i);
	_clients_map.erase(i);
}

// Est appelée lorsque le serveur a terminé de construire la réponse HTTP pour une requete client
// Envoie la reponse au client sur le socket correspondant
// Si pas d'erreur dans la requete et que le header de Connection == keep-alive --> connexion maintenue
// Sinon on la close

void	ServerManager::sendResponse(const int &i, Client &c)
{
	int			bytes_sent;
	std::string	response = c.response.getRes();

	if (response.length() >= MESSAGE_BUFFER)
		bytes_sent = write(i, response.c_str(), MESSAGE_BUFFER);
	else
		bytes_sent = write(i, response.c_str(), response.length());

	if (bytes_sent < 0)
	{
		Logger::logMsg(RED, CONSOLE_OUTPUT, "sendResponse(): error sending : %s", strerror(errno));
		closeConnection(i);
	}

	else if (bytes_sent == 0 || (size_t) bytes_sent == response.length())
	{
		Logger::logMsg(CYAN, CONSOLE_OUTPUT, "Response Sent To Socket %d, Stats=<%d>"
		, i, c.response.getCode());

		if (c.request.keepAlive() == false || c.request.errorCode() || c.response.getCgiState())
		{
			Logger::logMsg(YELLOW, CONSOLE_OUTPUT, "Client %d: Connection Closed.", i);
			closeConnection(i);
		}

		else
		{
			removeFromSet(i, _write_fd_pool);
			addToSet(i, _recv_fd_pool);
			c.clearClient();
		}
	}

	else
	{
		c.updateTime();
		c.response.cutRes(bytes_sent);
	}
}

// Determine a quel serveur la requete doit etre assignee en fonction de l'adresse IP,
// du numero de port de la requete et du nom de serveur dans le header de la requete
// Si une correspondance est trouvee, l'objet Client est m.a.j avec la config du serveur correspondant
// Sinon il garde la config du serveur precedent auquel il etait assigne

void	ServerManager::assignServer(Client &c)
{
	for (std::vector<ServerConfig>::iterator it = _servers.begin(); it != _servers.end(); ++it)
	{
		if (c.server.getHost() == it->getHost() && c.server.getPort() == it->getPort()
			&& c.request.getServerName() == it->getServerName())
		{
			c.setServer(*it);
			return;
		}
	}
}

// Lit les données envoyées par le client connecte sur le socket i
// Et les envoient au parser
// Une fois que le parseur a finit son job ou qu'une erreur a ete trouvee dans la requête
// le socket sera deplace de _recv_fd_pool a _write_fd_pool et la reponse sera envoyée
// a la prochaine iteration de select()

void	ServerManager::readRequest(const int &i, Client &c)
{
	char	buffer[MESSAGE_BUFFER];
	int		bytes_read = 0;

	bytes_read = read(i, buffer, MESSAGE_BUFFER);

	if (bytes_read == 0)
	{
		Logger::logMsg(YELLOW, CONSOLE_OUTPUT, "webserv: Client %d Closed Connection", i);
		closeConnection(i);
		return;
	}

	else if (bytes_read < 0)
	{
		Logger::logMsg(RED, CONSOLE_OUTPUT, "webserv: fd %d read error %s", i, strerror(errno));
		closeConnection(i);
		return;
	}

	else if (bytes_read != 0)
	{
		c.updateTime();
		c.request.feed(buffer, bytes_read);
		memset(buffer, 0, sizeof(buffer));
	}

	// 1 = parsing complet, on peut travailler sur la reponse
	if (c.request.parsingCompleted() || c.request.errorCode())
	{
		assignServer(c);

		Logger::logMsg(CYAN, CONSOLE_OUTPUT, "Request Recived From Socket %d, Method=<%s>  URI=<%s>"
		, i, c.request.getMethodStr().c_str(), c.request.getPath().c_str());

		c.buildResponse();

		// la requete necessite l'exe d'un script CGI
		// Le body de la requete doit etre envoye au script pour traitement
		// Puis la reponse du script doit etre envoyee au client

		if (c.response.getCgiState())
		{
			// le body de la requete est traite et stocke dans une variable tempo
			// Cette variable est ensuite ecrite dans l'entree du script CGI a travers un pipe
			handleReqBody(c);
			addToSet(c.response._cgi_obj.pipe_in[1], _write_fd_pool);
			addToSet(c.response._cgi_obj.pipe_out[0], _recv_fd_pool);
		}

		removeFromSet(i, _recv_fd_pool);
		addToSet(i, _write_fd_pool);
	}
}

// Est utilisee lorsqu'une requête contenant un body (POST par ex) doit etre traitee par un CGI
void	ServerManager::handleReqBody(Client &c)
{
		// taille du body == 0 --> le body n'est pas lu depuis le socket client
		// mais est dispo dans un fichier temporaire

		if (c.request.getBody().length() == 0)
		{
			std::string			tmp; // stocke temporairement les donnees de la requete
			std::fstream		file;(c.response._cgi_obj.getCgiPath().c_str());
			std::stringstream	ss;

			ss << file.rdbuf();
			tmp = ss.str();
			c.request.setBody(tmp);
		}
}

// Envoie le corps de la requete au script CGI
void	ServerManager::sendCgiBody(Client &c, CgiHandler &cgi)
{
	int			bytes_sent;
	std::string	&req_body = c.request.getBody();

	if (req_body.length() == 0)
		bytes_sent = 0;

	// Le contenu du body est ecrit dans le tube d'entree du processus CGI
	else if (req_body.length() >= MESSAGE_BUFFER)
		bytes_sent = write(cgi.pipe_in[1], req_body.c_str(), MESSAGE_BUFFER);

	else
		bytes_sent = write(cgi.pipe_in[1], req_body.c_str(), req_body.length());

	if (bytes_sent < 0)
	{
		Logger::logMsg(RED, CONSOLE_OUTPUT, "sendCgiBody() Error Sending: %s", strerror(errno));
		removeFromSet(cgi.pipe_in[1], _write_fd_pool);
		close(cgi.pipe_in[1]);
		close(cgi.pipe_out[1]);
		c.response.setErrorResponse(500);
	}

	// tout le contenu du body a ete envoye
	else if (bytes_sent == 0 || (size_t) bytes_sent == req_body.length())
	{
		removeFromSet(cgi.pipe_in[1], _write_fd_pool);
		close(cgi.pipe_in[1]);
		close(cgi.pipe_out[1]);
	}

	// puis on m.a.j le temps de la derniere activite du client
	else
	{
		c.updateTime();
		req_body = req_body.substr(bytes_sent);
	}
}

// Lit les outputs produits par le script CGI
void	ServerManager::readCgiResponse(Client &c, CgiHandler &cgi)
{
	char	buffer[MESSAGE_BUFFER * 2];
	int		bytes_read = 0;
	// read sur le pipe de sortie du CGI (sortie standard du script)
	bytes_read = read(cgi.pipe_out[0], buffer, MESSAGE_BUFFER* 2);

	if (bytes_read == 0) // --> plus rien a lire (le CGI a ferme sa sortie standard)
	{
		removeFromSet(cgi.pipe_out[0], _recv_fd_pool);
		close(cgi.pipe_in[0]);
		close(cgi.pipe_out[0]);

		int	status;
		waitpid(cgi.getCgiPid(), &status, 0);

		if (WEXITSTATUS(status) != 0)
			c.response.setErrorResponse(502);

		// indique que la reponse a envoyer provient d'un processus CGI
		c.response.setCgiState(2);

		if (c.response._response_content.find("HTTP/1.1") == std::string::npos)
			c.response._response_content.insert(0, "HTTP/1.1 200 OK\r\n");

		return;
	}

	else if (bytes_read < 0)
	{
		Logger::logMsg(RED, CONSOLE_OUTPUT, "readCgiResponse() Error Reading From CGI Script: ", strerror(errno));
		
		removeFromSet(cgi.pipe_out[0], _recv_fd_pool);
		close(cgi.pipe_in[0]);
		close(cgi.pipe_out[0]);

		c.response.setCgiState(2);
		c.response.setErrorResponse(500);

		return;
	}

	else
	{
		c.updateTime();
		c.response._response_content.append(buffer, bytes_read);
		memset(buffer, 0, sizeof(buffer));
	}
}

// Ajoute un fd (i) à l'ensemble de fd spécifié (set)
void	ServerManager::addToSet(const int i, fd_set &set)
{
	FD_SET(i, &set);

	if (i > _biggest_fd)
		_biggest_fd = i;
}

// Supprime un fd de l'ensemble de fd spécifié
void	ServerManager::removeFromSet(const int i, fd_set &set)
{
	FD_CLR(i, &set);

	// on décrémente _biggest_fd pour garantir que sa valeur reste à jour
	if (i == _biggest_fd)
		_biggest_fd--;
}