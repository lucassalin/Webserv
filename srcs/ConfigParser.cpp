/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lsalin <lsalin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/12 13:55:22 by lsalin            #+#    #+#             */
/*   Updated: 2023/04/20 12:50:27 by lsalin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"

ConfigParser::ConfigParser()
{
	this->_nb_server = 0;
}

ConfigParser::~ConfigParser() {}

/**
	@brief Affiche les configurations des serveurs

	@example	------------- Config -------------
				Server #1
				Server name: localhost
				Host: 127.0.0.1
				Root: /var/www/html
				Index: index.html
				Port: 8080
				Max BSize: 100000
				Error pages: 2
				400 - /var/www/html/error_pages/400.html
				404 - /var/www/html/error_pages/404.html
				Locations: 2
				name location: /
				methods: GET HEAD POST
				index: /index.html
				root: /var/www/html
				name location: /cgi-bin/
				methods: GET
				index: /index.cgi
				cgi root: /var/www/html
				cgi_path: 11
				cgi_ext: 3
				-----------------------------
 */

int	ConfigParser::print()
{
	std::cout << "------------- Config -------------" << std::endl;

	for (size_t i = 0; i < _servers.size(); i++)
	{
		std::cout << "Server #" << i + 1 << std::endl;
		std::cout << "Server name: " << _servers[i].getServerName() << std::endl;
		std::cout << "Host: " << _servers[i].getHost() << std::endl;
		std::cout << "Root: " << _servers[i].getRoot() << std::endl;
		std::cout << "Index: " << _servers[i].getIndex() << std::endl;
		std::cout << "Port: " << _servers[i].getPort() << std::endl;
		std::cout << "Max BSize: " << _servers[i].getClientMaxBodySize() << std::endl;
		std::cout << "Error pages: " << _servers[i].getErrorPages().size() << std::endl;

		std::map<short, std::string>::const_iterator it = _servers[i].getErrorPages().begin();

		while (it != _servers[i].getErrorPages().end())
		{
			std::cout << (*it).first << " - " << it->second << std::endl;
			++it;
		}

		std::cout << "Locations: " << _servers[i].getLocations().size() << std::endl;
		
		std::vector<Location>::const_iterator itl = _servers[i].getLocations().begin();
		
		while (itl != _servers[i].getLocations().end())
		{
			std::cout << "name location: " << itl->getPath() << std::endl;
			std::cout << "methods: " << itl->getPrintMethods() << std::endl;
			std::cout << "index: " << itl->getIndexLocation() << std::endl;
			
			if (itl->getCgiPath().empty())
			{
				std::cout << "root: " << itl->getRootLocation() << std::endl;
				
				if (!itl->getReturn().empty())
					std::cout << "return: " << itl->getReturn() << std::endl;

				if (!itl->getAlias().empty())
					std::cout << "alias: " << itl->getAlias() << std::endl;
			}
			
			else
			{
				std::cout << "cgi root: " << itl->getRootLocation() << std::endl;
				std::cout << "sgi_path: " << itl->getCgiPath().size() << std::endl;
				std::cout << "sgi_ext: " << itl->getCgiExtension().size() << std::endl;
			}
			
			++itl;
		}

		itl = _servers[i].getLocations().begin();
		std::cout << "-----------------------------" << std::endl;
	}
	return (0);
}

// Créer un cluster de serveurs à partir d'un fichier de configuration
// Renvoie (0) si succès, sinon lève une exception

int	ConfigParser::createCluster(const std::string &config_file)
{
	std::string		content;
	ConfigFile		file(config_file);

	if (file.getTypePath(file.getPath()) != 1)
		throw ErrorException("File is invalid");

	if (file.checkFile(file.getPath(), 4) == -1)
		throw ErrorException("File is not accessible");

	content = file.readFile(config_file);

	if (content.empty())
		throw ErrorException("File is empty");

	// clean le fichier de config
	removeComments(content);
	removeWhiteSpace(content);
	splitServers(content);

	if (this->_server_config.size() != this->_nb_server)
		throw ErrorException("Number of server configurations doesn't match the number of servers");

	for (size_t i = 0; i < this->_nb_server; i++)
	{
		ServerConfig	server;
		createServer(this->_server_config[i], server);
		this->_servers.push_back(server);
	}

	if (this->_nb_server > 1)
		checkServers();

	return (0);
}

// Supprime les commentaires du fichier de config (de "#" à "\n")
void	ConfigParser::removeComments(std::string &content)
{
	size_t	pos;
	pos = content.find('#');

	while (pos != std::string::npos)
	{
		size_t	pos_end;
		pos_end = content.find('\n', pos);
		content.erase(pos, pos_end - pos);
		pos = content.find('#');
	}
}

// Supprime les whitespaces en début/fin de string, et dans le contenu s'il y en a plus d'un
void	ConfigParser::removeWhiteSpace(std::string &content)
{
	size_t	i = 0;

	while (content[i] && isspace(content[i]))
		i++;

	content = content.substr(i);
	i = content.length() - 1;

	while (i > 0 && isspace(content[i]))
		i--;

	content = content.substr(0, i + 1);
}

/**
	@brief Divise les différentes configs en configs distinctes

	@example	server
				{
    				host example.com;
    				port 8080;
				}

				server
				{
    				host example.org;
    				port 80;
				}

			--> _server_config[0] = "server {\n    host example.com;\n    port 8080;\n}\n"
				_server_config[1] = "server {\n    host example.org;\n    port 80;\n}\n"
 */

void	ConfigParser::splitServers(std::string &content)
{
	size_t	start = 0;
	size_t	end = 1;

	if (content.find("server", 0) == std::string::npos)
		throw ErrorException("Server did not find");

	while (start != end && start < content.length())
	{
		start = findStartServer(start, content);
		end = findEndServer(start, content);

		if (start == end)
			throw ErrorException("problem with scope");

		this->_server_config.push_back(content.substr(start, end - start + 1));
		this->_nb_server++;

		start = end + 1;
	}
}

// Trouve la position de début de configuration du serveur ('{')
// Retourne la position du premier '{'

size_t	ConfigParser::findStartServer(size_t start, std::string &content)
{
	size_t	i;

	for (i = start; content[i]; i++)
	{
		if (content[i] == 's')
			break;

		if (!isspace(content[i]))
			throw  ErrorException("Wrong character out of server scope{}");
	}

	if (!content[i])
		return (start);

	if (content.compare(i, 6, "server") != 0)
		throw ErrorException("Wrong character out of server scope{}");

	i += 6;

	while (content[i] && isspace(content[i]))
		i++;

	if (content[i] == '{')
		return (i);
	else
		throw  ErrorException("Wrong character out of server scope{}");
}

size_t	ConfigParser::findEndServer (size_t start, std::string &content)
{
	size_t	i;
	size_t	scope;
	
	scope = 0;

	for (i = start + 1; content[i]; i++)
	{
		if (content[i] == '{')
			scope++;

		if (content[i] == '}')
		{
			if (!scope)
				return (i);
			scope--;
		}
	}
	return (start);
}

// Divise une string en fonction d'un séparateur
std::vector<std::string>	splitParametrs(std::string line, std::string sep)
{
	std::vector<std::string>	str;
	std::string::size_type		start, end;

	start = end = 0;

	while (1)
	{
		end = line.find_first_of(sep, start);

		if (end == std::string::npos)
			break;

		std::string tmp = line.substr(start, end - start);
		str.push_back(tmp);
		start = line.find_first_not_of(sep, end);

		if (start == std::string::npos)
			break;
	}
	return (str);
}

void	ConfigParser::createServer(std::string &config, ServerConfig &server)
{
	std::vector<std::string>	parametrs;		// paramètres de config
	std::vector<std::string>	error_codes;	// codes d'erreurs du serveur
	int							flag_loc = 1;	// true = on est en train de traiter les paramètres d'un emplacement
	bool						flag_autoindex = false;
	bool						flag_max_size = false;

		// listen 8080;
		// server_name localhost;
		// root /var/www/html/;
		// index index.html;

// -->	// parametrs[0] = "listen"
		// parametrs[1] = "8080;"
		// parametrs[2] = "server_name"
		// parametrs[3] = "localhost;"
		// parametrs[4] = "root"
		// parametrs[5] = "/var/www/html/;"
		// parametrs[6] = "index"
		// parametrs[7] = "index.html;"

	parametrs = splitParametrs(config += ' ', std::string(" \n\t"));

	if (parametrs.size() < 3)
		throw  ErrorException("Failed server validation");

	for (size_t i = 0; i < parametrs.size(); i++)
	{
		if (parametrs[i] == "listen" && (i + 1) < parametrs.size() && flag_loc)
		{
			if (server.getPort()) // si port déjà défini
				throw  ErrorException("Port is duplicated");
			server.setPort(parametrs[++i]);
		}

		else if (parametrs[i] == "location" && (i + 1) < parametrs.size())
		{
			std::string	path;
			i++;

			if (parametrs[i] == "{" || parametrs[i] == "}")
				throw  ErrorException("Wrong character in server scope {}");

			path = parametrs[i];
			std::vector<std::string>	codes;
			
			if (parametrs[++i] != "{")
				throw  ErrorException("Wrong character in server scope {}");

			i++;

			while (i < parametrs.size() && parametrs[i] != "}")
				codes.push_back(parametrs[i++]);

			server.setLocation(path, codes);

			if (i < parametrs.size() && parametrs[i] != "}")
				throw  ErrorException("Wrong character in server scope {}");

			flag_loc = 0;
		}

		else if (parametrs[i] == "host" && (i + 1) < parametrs.size() && flag_loc)
		{
			if (server.getHost())
				throw  ErrorException("Host is duplicated");
			server.setHost(parametrs[++i]);
		}

		else if (parametrs[i] == "root" && (i + 1) < parametrs.size() && flag_loc)
		{
			if (!server.getRoot().empty())
				throw  ErrorException("Root is duplicated");

			server.setRoot(parametrs[++i]);
		}

		else if (parametrs[i] == "error_page" && (i + 1) < parametrs.size() && flag_loc)
		{
			while (++i < parametrs.size())
			{
				error_codes.push_back(parametrs[i]);

				if (parametrs[i].find(';') != std::string::npos)
					break ;

				if (i + 1 >= parametrs.size())
					throw ErrorException("Wrong character out of server scope {}");
			}
		}

		else if (parametrs[i] == "client_max_body_size" && (i + 1) < parametrs.size() && flag_loc)
		{
			if (flag_max_size)
				throw  ErrorException("Client_max_body_size is duplicated");

			server.setClientMaxBodySize(parametrs[++i]);
			flag_max_size = true;
		}

		else if (parametrs[i] == "server_name" && (i + 1) < parametrs.size() && flag_loc)
		{
			if (!server.getServerName().empty())
				throw  ErrorException("Server_name is duplicated");

			server.setServerName(parametrs[++i]);
		}

		else if (parametrs[i] == "index" && (i + 1) < parametrs.size() && flag_loc)
		{
			if (!server.getIndex().empty())
				throw  ErrorException("Index is duplicated");

			server.setIndex(parametrs[++i]);
		}

		else if (parametrs[i] == "autoindex" && (i + 1) < parametrs.size() && flag_loc)
		{
			if (flag_autoindex)
				throw ErrorException("Autoindex of server is duplicated");

			server.setAutoindex(parametrs[++i]);
			flag_autoindex = true;
		}

		else if (parametrs[i] != "}" && parametrs[i] != "{")
		{
			if (!flag_loc)
				throw  ErrorException("Parametrs after location");
			else
				throw  ErrorException("Unsupported directive");
		}
	}

	if (server.getRoot().empty())
		server.setRoot("/;");

	if (server.getHost() == 0)
		server.setHost("localhost;");
		
	if (server.getIndex().empty())
		server.setIndex("index.html;");

	if (ConfigFile::isFileExistAndReadable(server.getRoot(), server.getIndex()))
		throw ErrorException("Index from config file not found or unreadable");
		
	if (server.checkLocaitons())
		throw ErrorException("Locaition is duplicated");
		
	if (!server.getPort())
		throw ErrorException("Port not found");
		
	server.setErrorPages(error_codes);
	
	if (!server.isValidErrorPages())
		throw ErrorException("Incorrect path for error page or number of error");
}

// Renvoie (0) si les strings correspondent, sinon (1)
int	ConfigParser::stringCompare(std::string str1, std::string str2, size_t pos)
{
	size_t	i;
	i = 0;

	while (pos < str1.length() && i < str2.length() && str1[pos] == str2[i])
	{
		pos++;
		i++;
	}

	if (i == str2.length() && pos <= str1.length() && (str1.length() == pos || isspace(str1[pos])))
		return (0);
	return (1);
}

// Check si deux serveurs ont les mêmes paramètres de configs (même host etc.)
// Lève une exception si c'est le cas

void	ConfigParser::checkServers()
{
	std::vector<ServerConfig>::iterator	it1;
	std::vector<ServerConfig>::iterator	it2;

	for (it1 = this->_servers.begin(); it1 != this->_servers.end() - 1; it1++)
	{
		for (it2 = it1 + 1; it2 != this->_servers.end(); it2++)
		{
			if (it1->getPort() == it2->getPort() && it1->getHost() == it2->getHost() && it1->getServerName() == it2->getServerName())
				throw ErrorException("Failed server validation");
		}
	}
}

std::vector<ServerConfig>	ConfigParser::getServers()
{
	return (this->_servers);
}