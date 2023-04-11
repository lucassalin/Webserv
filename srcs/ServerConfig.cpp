/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerConfig.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lsalin <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/05 15:23:14 by lsalin            #+#    #+#             */
/*   Updated: 2023/04/11 13:04:26 by lsalin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ServerConfig.hpp"

ServerConfig::ServerConfig()
{
	this->_port = 0;
	this->_host = 0;
	this->_server_name = "";
	this->_root = "";
	this->_client_max_body_size = MAX_CONTENT_LENGTH;
	this->_index = "";
	this->_listen_fd = 0;
	this->_autoindex = false;
	this->initErrorPages();
}

ServerConfig::~ServerConfig() {}

ServerConfig::ServerConfig(const ServerConfig &other)
{
	if (this != &other)
	{
		this->_server_name = other._server_name;
		this->_root = other._root;
		this->_host = other._host;
		this->_port = other._port;
		this->_client_max_body_size = other._client_max_body_size;
		this->_index = other._index;
		this->_error_pages = other._error_pages;
		this->_locations = other._locations;
		this->_listen_fd = other._listen_fd;
		this->_autoindex = other._autoindex;
		this->_server_address = other._server_address;

	}
	return ;
}

ServerConfig	&ServerConfig::operator=(const ServerConfig & rhs)
{
	if (this != &rhs)
	{
		this->_server_name = rhs._server_name;
		this->_root = rhs._root;
		this->_port = rhs._port;
		this->_host = rhs._host;
		this->_client_max_body_size = rhs._client_max_body_size;
		this->_index = rhs._index;
		this->_error_pages = rhs._error_pages;
		this->_locations = rhs._locations;
		this->_listen_fd = rhs._listen_fd;
		this->_autoindex = rhs._autoindex;
		this->_server_address = rhs._server_address;
	}
	return (*this);
}

void	ServerConfig::initErrorPages(void)
{
	_error_pages[301] = "";
	_error_pages[302] = "";
	_error_pages[400] = "";
	_error_pages[401] = "";
	_error_pages[402] = "";
	_error_pages[403] = "";
	_error_pages[404] = "";
	_error_pages[405] = "";
	_error_pages[406] = "";
	_error_pages[500] = "";
	_error_pages[501] = "";
	_error_pages[502] = "";
	_error_pages[503] = "";
	_error_pages[505] = "";
	_error_pages[505] = "";
}

void	ServerConfig::setServerName(std::string server_name)
{
	checkToken(server_name);
	this->_server_name = server_name;
}

// Definit l'adresse IP du serveur

void	ServerConfig::setHost(std::string parametr)
{
	checkToken(parametr);

	if (parametr == "localhost")
		parametr = "127.0.0.1";

	if (!isValidHost(parametr))
		throw ErrorException("Wrong syntax: host");

	// Convertit la string parametr en une adresse IP de type in_addr_t
	this->_host = inet_addr(parametr.data());
}

// Definit le dossier racine a partir d'une string root

void	ServerConfig::setRoot(std::string root)
{
	checkToken(root);

	if (ConfigFile::getTypePath(root) == 2)
	{
		this->_root = root;
		return ;
	}

	char	dir[1024];
	getcwd(dir, 1024); // obtient le path du repertoire courant

	std::string	full_root = dir + root;

	if (ConfigFile::getTypePath(full_root) != 2)
		throw ErrorException("Wrong syntax: root");

	this->_root = full_root;
}

// Check si la string parametr contient un nᵒ de port valide

void	ServerConfig::setPort(std::string parametr)
{
	unsigned int port;
	port = 0;

	checkToken(parametr);

	for (size_t i = 0; i < parametr.length(); i++)
	{
		if (!std::isdigit(parametr[i]))
			throw ErrorException("Wrong syntax: port");
	}

	port = ft_stoi((parametr));

	if (port < 1 || port > 65636)
		throw ErrorException("Wrong syntax: port");

	this->_port = (uint16_t) port; //  uint16_t = 0 à 65535
}

// Definit la taille max du body d'une requête que le serveur peut accepter

void	ServerConfig::setClientMaxBodySize(std::string parametr)
{
	unsigned long body_size;
	body_size = 0;

	checkToken(parametr);
	
	for (size_t i = 0; i < parametr.length(); i++)
	{
		if (parametr[i] < '0' || parametr[i] > '9')
			throw ErrorException("Wrong syntax: client_max_body_size");
	}
	
	if (!ft_stoi(parametr))
		throw ErrorException("Wrong syntax: client_max_body_size");
		
	body_size = ft_stoi(parametr);
	this->_client_max_body_size = body_size;
}

void	ServerConfig::setIndex(std::string index)
{
	checkToken(index);
	this->_index = index;
}

void	ServerConfig::setAutoindex(std::string autoindex)
{
	checkToken(autoindex);

	if (autoindex != "on" && autoindex != "off")
		throw ErrorException("Wrong syntax: autoindex");
		
	if (autoindex == "on")
		this->_autoindex = true;
}

/**
	@brief Configure les pages d'erreurs personnalisées du serveur web
		   Check s'il existe un tel code d'erreur par defaut
		   Si oui --> ecrase le path d'acces au fichier
		   Sinon --> creer une nouvelle paire key-value pour le path vers le fichier
*/

void	ServerConfig::setErrorPages(std::vector<std::string> &parametr)
{
	if (parametr.empty())
		return;

	// la taille du vecteur parametr doit etre paire
	// car 2 elements associes : code d'erreur + path vers la page d'erreur
	if (parametr.size() % 2 != 0)
		throw ErrorException ("Error page initialization failed");

	for (size_t i = 0; i < parametr.size() - 1; i++)
	{
		for (size_t j = 0; j < parametr[i].size(); j++) 
		{
			if (!std::isdigit(parametr[i][j]))
				throw ErrorException("Error code is invalid");
		}

		if (parametr[i].size() != 3) // code d'erreur = 3 chiffres
			throw ErrorException("Error code is invalid");

		short code_error = ft_stoi(parametr[i]);

		if (statusCodeString(code_error)  == "Undefined" || code_error < 400)
			throw ErrorException ("Incorrect error code: " + parametr[i]);

		i++;

		std::string	path = parametr[i];
		checkToken(path);

		// Check si le path spécifié pour la page d'erreur est valide/acessible ou non
		if (ConfigFile::getTypePath(path) != 2)
		{
			if (ConfigFile::getTypePath(this->_root + path) != 1)
				throw ErrorException ("Incorrect path for error page file: " + this->_root + path);
				
			if (ConfigFile::checkFile(this->_root + path, 0) == -1 || ConfigFile::checkFile(this->_root + path, 4) == -1)
				throw ErrorException ("Error page file :" + this->_root + path + " is not accessible");
		}

		// On cherche si une paire key-value avec cette clé de code d'erreur existe déjà
		std::map<short, std::string>::iterator	it = this->_error_pages.find(code_error);

		// Si oui, on m.a.j le path de la page d'erreur correspondante
		if (it != _error_pages.end())
			this->_error_pages[code_error] = path;
		// sinon on insere une nouvelle paire key-value dans la map
		else
			this->_error_pages.insert(std::make_pair(code_error, path));
	}
}

/**
	@brief Definit et configure la location des differents types de ressources possibles 

	@param path : path de la location qu'on veut definir
	@param parametr : contient les parametres a definir pour cette location (key-value)
*/

void	ServerConfig::setLocation(std::string path, std::vector<std::string> parametr)
{
	Location					new_location;
	std::vector<std::string>	methods;
	int							valid;

	bool flag_methods = false;
	bool flag_autoindex = false;
	bool flag_max_size = false;

	new_location.setPath(path);

	for (size_t i = 0; i < parametr.size(); i++)
	{
		// Check si "root" est present et qu'il n'a pas encore ete defini pour l'objet new_location
		// Si c'est le cas --> configure la racine de la nouvelle location en fonction du type de chemin
		// du parametre

		if (parametr[i] == "root" && (i + 1) < parametr.size())
		{
			// check si "root" n'a pas déjà été assignée a la location
			if (!new_location.getRootLocation().empty())
				throw ErrorException("Root of location is duplicated");

			checkToken(parametr[++i]);

			// definit la racine en fonction du type de parametr
			if (ConfigFile::getTypePath(parametr[i]) == 2)
				new_location.setRootLocation(parametr[i]);
			else
				new_location.setRootLocation(this->_root + parametr[i]);
		}

		// location /test
		// "allow_methods GET;POST;PUT"

		else if ((parametr[i] == "allow_methods" || parametr[i] == "methods") && (i + 1) < parametr.size())
		{
			if (flag_methods)
				throw ErrorException("Allow_methods of location is duplicated");

			std::vector<std::string>	methods;

			while (++i < parametr.size())
			{
				// ";" = la liste des methodes est finit
				// Donc on ajoute cette derniere methode et on sort de la boucle
				if (parametr[i].find(";") != std::string::npos)
				{
					checkToken(parametr[i]);
					methods.push_back(parametr[i]);
					break;
				}

				else
				{
					methods.push_back(parametr[i]);

					if (i + 1 >= parametr.size())
						throw ErrorException("Token is invalid");
				}
			}

			new_location.setMethods(methods);
			flag_methods = true;
		}

		// Gestion de l'autoindexation
		else if (parametr[i] == "autoindex" && (i + 1) < parametr.size())
		{
			if (path == "/cgi-bin")
				throw ErrorException("Parametr autoindex not allow for CGI");

			if (flag_autoindex)
				throw ErrorException("Autoindex of location is duplicated");

			checkToken(parametr[++i]);

			new_location.setAutoindex(parametr[i]);
			flag_autoindex = true;
		}

		else if (parametr[i] == "index" && (i + 1) < parametr.size())
		{
			if (!new_location.getIndexLocation().empty())
				throw ErrorException("Index of location is duplicated");

			checkToken(parametr[++i]);
			new_location.setIndexLocation(parametr[i]);
		}

		else if (parametr[i] == "return" && (i + 1) < parametr.size())
		{
			if (path == "/cgi-bin")
				throw ErrorException("Parametr return not allow for CGI");

			if (!new_location.getReturn().empty())
				throw ErrorException("Return of location is duplicated");

			checkToken(parametr[++i]);
			new_location.setReturn(parametr[i]);
		}

		else if (parametr[i] == "alias" && (i + 1) < parametr.size())
		{
			if (path == "/cgi-bin")
				throw ErrorException("Parametr alias not allow for CGI");

			if (!new_location.getAlias().empty())
				throw ErrorException("Alias of location is duplicated");

			checkToken(parametr[++i]);
			new_location.setAlias(parametr[i]);
		}

		else if (parametr[i] == "cgi_ext" && (i + 1) < parametr.size())
		{
			std::vector<std::string>	extension;

			while (++i < parametr.size())
			{
				if (parametr[i].find(";") != std::string::npos)
				{
					checkToken(parametr[i]);
					extension.push_back(parametr[i]);
					break;
				}

				else
				{
					extension.push_back(parametr[i]);

					if (i + 1 >= parametr.size())
						throw ErrorException("Token is invalid");
				}
			}

			new_location.setCgiExtension(extension);
		}

		else if (parametr[i] == "cgi_path" && (i + 1) < parametr.size())
		{
			std::vector<std::string>	path;

			while (++i < parametr.size())
			{
				if (parametr[i].find(";") != std::string::npos)
				{
					checkToken(parametr[i]);
					path.push_back(parametr[i]);
					break;
				}

				else
				{
					path.push_back(parametr[i]);

					if (i + 1 >= parametr.size())
						throw ErrorException("Token is invalid");
				}

				// si le path ne contient ni "/python" ni "/bash"
				if (parametr[i].find("/python") == std::string::npos && parametr[i].find("/bash") == std::string::npos)
					throw ErrorException("cgi_path is invalid");
			}

			new_location.setCgiPath(path);
		}

		else if (parametr[i] == "client_max_body_size" && (i + 1) < parametr.size())
		{
			if (flag_max_size)
				throw ErrorException("Maxbody_size of location is duplicated");

			checkToken(parametr[++i]);
			new_location.setMaxBodySize(parametr[i]);
			flag_max_size = true;
		}

		else if (i < parametr.size())
			throw ErrorException("Parametr in a location is invalid");
	}

	// si la location nouvellement cree n'est pas une location CGI
	// et que l'index de la location est vide --> affecte valeur par defaut

	if (new_location.getPath() != "/cgi-bin" && new_location.getIndexLocation().empty())
		new_location.setIndexLocation(this->_index);

	if (!flag_max_size)
		new_location.setMaxBodySize(this->_client_max_body_size);

	valid = isValidLocation(new_location);

	if (valid == 1)
		throw ErrorException("Failed CGI validation");
	else if (valid == 2)
		throw ErrorException("Failed path in locaition validation");
	else if (valid == 3)
		throw ErrorException("Failed redirection file in locaition validation");
	else if (valid == 4)
		throw ErrorException("Failed alias file in locaition validation");

	this->_locations.push_back(new_location);
}

void	ServerConfig::setFd(int fd)
{
	this->_listen_fd = fd;
}

/**
	@brief Check si l'adresse IP contenue dans "host" est valide ou non

	@return true si valide, sinon false
*/

bool	ServerConfig::isValidHost(std::string host) const
{
	struct sockaddr_in sockaddr;
  	return (inet_pton(AF_INET, host.c_str(), &(sockaddr.sin_addr)) ? true : false);
}

/**
	@brief Check si les pages d'erreurs definies pour le serveur sont valides

	@return true si oui, sinon false
*/

bool	ServerConfig::isValidErrorPages()
{
	std::map<short, std::string>::const_iterator	it;

	for (it = this->_error_pages.begin(); it != this->_error_pages.end(); it++)
	{
		if (it->first < 100 || it->first > 599)
			return (false);

		// check acces + permissions
		if (ConfigFile::checkFile(getRoot() + it->second, 0) < 0 || ConfigFile::checkFile(getRoot() + it->second, 4) < 0)
			return (false);
	}

	return (true);
}

// Check si les parametres d'une location sont valides

int	ServerConfig::isValidLocation(Location &location) const
{
	if (location.getPath() == "/cgi-bin")
	{
		if (location.getCgiPath().empty() || location.getCgiExtension().empty() || location.getIndexLocation().empty())
			return (1);

		if (ConfigFile::checkFile(location.getIndexLocation(), 4) < 0)
		{
			std::string path = location.getRootLocation() + location.getPath() + "/" + location.getIndexLocation();

			if (ConfigFile::getTypePath(path) != 1)
			{				
				std::string root = getcwd(NULL, 0);
				location.setRootLocation(root);
				path = root + location.getPath() + "/" + location.getIndexLocation();
			}

			if (path.empty() || ConfigFile::getTypePath(path) != 1 || ConfigFile::checkFile(path, 4) < 0)
				return (1);
		}

		if (location.getCgiPath().size() != location.getCgiExtension().size())
			return (1);

		std::vector<std::string>::const_iterator	it;

		for (it = location.getCgiPath().begin(); it != location.getCgiPath().end(); ++it)
		{
			if (ConfigFile::getTypePath(*it) < 0)
				return (1);
		}

		std::vector<std::string>::const_iterator it_path;

		for (it = location.getCgiExtension().begin(); it != location.getCgiExtension().end(); ++it)
		{
			std::string tmp = *it;

			if (tmp != ".py" && tmp != ".sh" && tmp != "*.py" && tmp != "*.sh")
				return (1);

			for (it_path = location.getCgiPath().begin(); it_path != location.getCgiPath().end(); ++it_path)
			{
				std::string tmp_path = *it_path;
				
				if (tmp == ".py" || tmp == "*.py")
				{
					if (tmp_path.find("python") != std::string::npos)
						location._ext_path.insert(std::make_pair(".py", tmp_path));
				}
				
				else if (tmp == ".sh" || tmp == "*.sh")
				{
					if (tmp_path.find("bash") != std::string::npos)
						location._ext_path[".sh"] = tmp_path;
				}
			}
		}
		
		if (location.getCgiPath().size() != location.getExtensionPath().size())
			return (1);
	}
	
	else
	{
		if (location.getPath()[0] != '/')
			return (2);
			
		if (location.getRootLocation().empty()) {
			location.setRootLocation(this->_root);
		}
		
		if (ConfigFile::isFileExistAndReadable(location.getRootLocation() + location.getPath() + "/", location.getIndexLocation()))
			return (5);
			
		if (!location.getReturn().empty())
		{
			if (ConfigFile::isFileExistAndReadable(location.getRootLocation(), location.getReturn()))
				return (3);
		}
		
		if (!location.getAlias().empty())
		{
			if (ConfigFile::isFileExistAndReadable(location.getRootLocation(), location.getAlias()))
			 	return (4);
		}
	}

	return (0);
}

const std::string &ServerConfig::getServerName()
{
	return (this->_server_name);
}

const std::string &ServerConfig::getRoot()
{
	return (this->_root);
}

const bool &ServerConfig::getAutoindex()
{
	return (this->_autoindex);
}

const in_addr_t &ServerConfig::getHost()
{
	return (this->_host);
}

const uint16_t &ServerConfig::getPort()
{
	return (this->_port);
}

const size_t &ServerConfig::getClientMaxBodySize()
{
	return (this->_client_max_body_size);
}

const std::vector<Location> &ServerConfig::getLocations()
{
	return (this->_locations);
}

const std::map<short, std::string> &ServerConfig::getErrorPages()
{
	return (this->_error_pages);
}

const std::string &ServerConfig::getIndex()
{
	return (this->_index);
}

int	ServerConfig::getFd() 
{ 
	return (this->_listen_fd); 
}

// Recupere le path vers une page d'erreur
// en fonction de sa key dans la map _error_pages

const std::string	&ServerConfig::getPathErrorPage(short key)
{
	std::map<short, std::string>::iterator it = this->_error_pages.find(key);

	if (it == this->_error_pages.end())
		throw ErrorException("Error_page does not exist");

	return (it->second);
}

// Trouve une location dans le vecteur _locations en fonction de sa key

const std::vector<Location>::iterator	ServerConfig::getLocationKey(std::string key)
{
	std::vector<Location>::iterator it;

	for (it = this->_locations.begin(); it != this->_locations.end(); it++)
	{
		if (it->getPath() == key)
			return (it);
	}

	throw ErrorException("Error: path to location not found");
}

// S'assure qu'un token (parametre) est bien forme avant de l'utiliser en config

void	ServerConfig::checkToken(std::string &parametr)
{
	size_t	pos = parametr.rfind(';');

	// Si la string ne se termine pas par ";" --> erreur
	if (pos != parametr.size() - 1)
		throw ErrorException("Token is invalid");

	parametr.erase(pos); // on retire le ";" de la fin de string
}

// Verifie si deux locations de ressources sur le serveur n'ont pas la meme path
// Retourne true si c'est le cas, sinon false

bool	ServerConfig::checkLocaitons() const
{
	if (this->_locations.size() < 2)
		return (false);

	std::vector<Location>::const_iterator	it1;
	std::vector<Location>::const_iterator	it2;

	for (it1 = this->_locations.begin(); it1 != this->_locations.end() - 1; it1++)
	{
		for (it2 = it1 + 1; it2 != this->_locations.end(); it2++)
		{
			if (it1->getPath() == it2->getPath())
				return (true);
		}
	}

	return (false);
}

// Configure et initialise le socket d'ecoute du serveur

void	ServerConfig::setupServer(void)
{
	if ((_listen_fd = socket(AF_INET, SOCK_STREAM, 0) ) == -1)
	{
		Logger::logMsg(RED, CONSOLE_OUTPUT, "webserv: socket error %s   Closing ....", strerror(errno));
		exit(EXIT_FAILURE);
	}

	int	option_value = 1;

	// configuration du socket avec SO_REUSEADDR pour l'adresse locale du socket puisse etre reutilisee
	// par un autre socket s'il ferme
	// Evite l'erreur "Address already in use" si on relance rapidement le serveur apres l'avoir stop

	setsockopt(_listen_fd, SOL_SOCKET, SO_REUSEADDR, &option_value, sizeof(int));
	memset(&_server_address, 0, sizeof(_server_address));

	_server_address.sin_family = AF_INET;
	_server_address.sin_addr.s_addr = _host;
	_server_address.sin_port = htons(_port);

	if (bind(_listen_fd, (struct sockaddr *) &_server_address, sizeof(_server_address)) == -1)
	{
		Logger::logMsg(RED, CONSOLE_OUTPUT, "webserv: bind error %s   Closing ....", strerror(errno));
		exit(EXIT_FAILURE);
	}
}

