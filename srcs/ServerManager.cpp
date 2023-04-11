/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerManager.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lsalin <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/05 15:05:48 by lsalin            #+#    #+#             */
/*   Updated: 2023/04/11 19:46:54 by lsalin           ###   ########.fr       */
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


		// inet_ntop() convertit une ad IP binaire en une string lisible par l'homme
		Logger::logMsg(LIGHTMAGENTA, CONSOLE_OUTPUT, "Server Created: ServerName[%s] Host[%s] Port[%d]",it->getServerName().c_str(),
				inet_ntop(AF_INET, &it->getHost(), buf, INET_ADDRSTRLEN), it->getPort());
	}
}

/**
 * Runs main loop that goes through all file descriptors from 0 till the biggest fd in the set.
 * - check file descriptors returend from select():
 *      if server fd --> accept new client
 *      if client fd in read_set --> read message from client
 *      if client fd in write_set:
 *          1- If it's a CGI response and Body still not sent to CGI child process --> Send request body to CGI child process.
 *          2- If it's a CGI response and Body was sent to CGI child process --> Read outupt from CGI child process.
 *          3- If it's a normal response --> Send response to client.
 * - servers and clients sockets will be added to _recv_set_pool initially,
 *   after that, when a request is fully parsed, socket will be moved to _write_set_pool
 */

/**
	@brief Exécute la boucle principale du serveur qui surveille tous les fd associés aux serveurs & clients
	et gére les événements associès à ces fd.

	Check les fd renvoyés par select() :
		- si server fd --> accepte le nouveau client
		- si client fd in read_set --> lit le message du client
		- si write_set :
			1) si réponse CGI et que le body n'est toujours pas envoyé au processus enfant CGI 
			   --> envoyé le body de la requête au processus enfant du CGI
			2) si réponse CGI et body envoyé --> lire la sortie du processus enfant CGI
			3) si réponse normale --> l'envoyer au client
	
	Les sockets serveurs/clients sont initialement add à _recv_set_pool.
	Puis, lorsqu'une requête est entièrement parsée, le socket est déplacé vers _write_set_pool

 */

void	ServerManager::runServers()
{
	fd_set  recv_set_cpy;
	fd_set  write_set_cpy;
	int     select_ret;

	_biggest_fd = 0;
	initializeSets();
	struct timeval	timer;

	while (true)
	{
		timer.tv_sec = 1;
		timer.tv_usec = 0;
		recv_set_cpy = _recv_fd_pool;
		write_set_cpy = _write_fd_pool;

		if ( (select_ret = select(_biggest_fd + 1, &recv_set_cpy, &write_set_cpy, NULL, &timer)) < 0 )
		{
			Logger::logMsg(RED, CONSOLE_OUTPUT, "webserv: select error %s   Closing ....", strerror(errno));
			exit(1);
			continue ;
		}
		for (int i = 0; i <= _biggest_fd; ++i)
		{
			if (FD_ISSET(i, &recv_set_cpy) && _servers_map.count(i))
				acceptNewConnection(_servers_map.find(i)->second);
			else if (FD_ISSET(i, &recv_set_cpy) && _clients_map.count(i))
				readRequest(i, _clients_map[i]);
			else if (FD_ISSET(i, &write_set_cpy) && _clients_map.count(i))
			{
				int cgi_state = _clients_map[i].response.getCgiState(); // 0->NoCGI 1->CGI write/read to/from script 2-CGI read/write done
				if (cgi_state == 1 && FD_ISSET(_clients_map[i].response._cgi_obj.pipe_in[1], &write_set_cpy))
					sendCgiBody(_clients_map[i], _clients_map[i].response._cgi_obj);
				else if (cgi_state == 1 && FD_ISSET(_clients_map[i].response._cgi_obj.pipe_out[0], &recv_set_cpy))
					readCgiResponse(_clients_map[i], _clients_map[i].response._cgi_obj);
				else if ((cgi_state == 0 || cgi_state == 2)  && FD_ISSET(i, &write_set_cpy))
					sendResponse(i, _clients_map[i]);
			}
		}
		checkTimeout();
	}
}

// Initialise recv/write_fd_pool
// Et ajoute tous les sockets d'écoute à _recv_fd_pool
// Est call lors de l'initialisation des serveurs

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

/**
 * Accept new incomming connection.
 * Create new Client object and add it to _client_map
 * Add client socket to _recv_fd_pool
*/

void	ServerManager::acceptNewConnection(ServerConfig &serv)
{
	struct sockaddr_in	client_address;
	long				client_address_size = sizeof(client_address);
	int					client_sock;
	Client				new_client(serv);
	char				buf[INET_ADDRSTRLEN];

	if ( (client_sock = accept(serv.getFd(), (struct sockaddr *)&client_address,
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

	if (_clients_map.count(client_sock) != 0)
		_clients_map.erase(client_sock);

	_clients_map.insert(std::make_pair(client_sock, new_client));
}

// Ajoute un fd (i) à l'ensemble de fd spécifié (set)
// Si ce fd est plus grand que le plus grand des fd, on m.a.j _biggest_fd

void	ServerManager::addToSet(const int i, fd_set &set)
{
	FD_SET(i, &set);

	if (i > _biggest_fd)
		_biggest_fd = i;
}

// Supprime un fd de l'ensemble de fd spécifié
// Si ce fd est == _biggest_fd, on décrémente _biggest_fd pour garantir que sa valeur reste à jour

void	ServerManager::removeFromSet(const int i, fd_set &set)
{
	FD_CLR(i, &set);
	if (i == _biggest_fd)
		_biggest_fd--;
}