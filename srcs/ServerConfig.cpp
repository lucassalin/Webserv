/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerConfig.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lsalin <lsalin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/05 15:23:14 by lsalin            #+#    #+#             */
/*   Updated: 2023/04/08 15:49:27 by lsalin           ###   ########.fr       */
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



void	ServerConfig::setRoot(std::string root)
{
	checkToken(root);

	if (ConfigFile::getTypePath(root) == 2)
	{
		this->_root = root;
		return ;
	}

	char	dir[1024];
	getcwd(dir, 1024);

	std::string	full_root = dir + root;

	if (ConfigFile::getTypePath(full_root) != 2)
		throw ErrorException("Wrong syntax: root");

	this->_root = full_root;
}

