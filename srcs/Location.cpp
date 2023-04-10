/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Location.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lsalin <lsalin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/05 15:28:18 by lsalin            #+#    #+#             */
/*   Updated: 2023/04/10 10:18:48 by lsalin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Location.hpp"

Location::Location()
{
	this->_path = "";
	this->_root = "";
	this->_autoindex = false;
	this->_index = "";
	this->_return = "";
	this->_alias = "";
	this->_client_max_body_size = MAX_CONTENT_LENGTH;
	this->_methods.reserve(5);		// allocation pour les 5 méthodes
	this->_methods.push_back(1);	// GET activée par défaut
	this->_methods.push_back(0);	// Les 4 autres sont désactivées par défaut
	this->_methods.push_back(0);	// car modification d'état du serveur
	this->_methods.push_back(0);
	this->_methods.push_back(0);
}

Location::Location(const Location &other)
{
	this->_path = other._path;
	this->_root = other._root;
	this->_autoindex = other._autoindex;
	this->_index = other._index;
	this->_cgi_path = other._cgi_path;
	this->_cgi_ext = other._cgi_ext;
	this->_return = other._return;
	this->_alias = other._alias;
    this->_methods = other._methods;
	this->_ext_path = other._ext_path;
	this->_client_max_body_size = other._client_max_body_size;
}

Location	&Location::operator=(const Location &rhs)
{
	if (this != &rhs)
	{
		this->_path = rhs._path;
		this->_root = rhs._root;
		this->_autoindex = rhs._autoindex;
		this->_index = rhs._index;
		this->_cgi_path = rhs._cgi_path;
		this->_cgi_ext = rhs._cgi_ext;
		this->_return = rhs._return;
		this->_alias = rhs._alias;
		this->_methods = rhs._methods;
		this->_ext_path = rhs._ext_path;
		this->_client_max_body_size = rhs._client_max_body_size;
    }
	return (*this);
}

Location::~Location() {}

void	Location::setPath(std::string parametr)
{
	this->_path = parametr;
}

/**
 @brief Définit le répertoire racine pour une instance de Location

 @param parametr : path du répertoire racine
 */

void	Location::setRootLocation(std::string parametr)
{
	if (ConfigFile::getTypePath(parametr) != 2)
		throw ServerConfig::ErrorException("root of location");
	this->_root = parametr;
}

// Définit les méthodes HTTP autorisées pour une instance de Localisation

void	Location::setMethods(std::vector<std::string> methods)
{
	this->_methods[0] = 0;
	this->_methods[1] = 0;
	this->_methods[2] = 0;
	this->_methods[3] = 0;
	this->_methods[4] = 0;

	// Définit a 1 la méthode passée en paramètre
	for (size_t i = 0; i < methods.size(); i++)
	{
		if (methods[i] == "GET")
			this->_methods[0] = 1;
		else if (methods[i] == "POST")
			this->_methods[1] = 1;
		else if (methods[i] == "DELETE")
			this->_methods[2] = 1;
		else if (methods[i] == "PUT")
			this->_methods[3] = 1;
		else if (methods[i] == "HEAD")
			this->_methods[4] = 1;
		else
			throw ServerConfig::ErrorException("Allow method not supported " + methods[i]);
	}
}

// Définit la valeur de _autoindex
// En fonction que l'auto-indexation est validée ou non
// 

void	Location::setAutoindex(std::string parametr)
{
	// si on --> _autoindex = true
	// si off --> _autoindex = false
	if (parametr == "on" || parametr == "off")
		this->_autoindex = (parametr == "on");
	else
		throw ServerConfig::ErrorException("Wrong autoindex");
}

void	Location::setIndexLocation(std::string parametr)
{
	this->_index = parametr;
}

void	Location::setReturn(std::string parametr)
{
	this->_return = parametr;
}

void	Location::setAlias(std::string parametr)
{
	this->_alias = parametr;
}

void	Location::setCgiPath(std::vector<std::string> path)
{
	this->_cgi_path = path;
}

void	Location::setCgiExtension(std::vector<std::string> extension)
{
	this->_cgi_ext = extension;
}

// Définit la taille maximale du body d'une requête acceptée pour cette location
// Si paramtr = 100, la taille max du body est de 100 octets

void	Location::setMaxBodySize(std::string parametr)
{
	unsigned long body_size = 0;

	// Check que paramtr contient bien des chiffres
	for (size_t i = 0; i < parametr.length(); i++)
	{
		if (parametr[i] < '0' || parametr[i] > '9')
			throw ServerConfig::ErrorException("Wrong syntax: client_max_body_size");
	}

	// Convertit la string en un unsigned int
	if (!ft_stoi(parametr))
		throw ServerConfig::ErrorException("Wrong syntax: client_max_body_size");
		
	body_size = ft_stoi(parametr);
	this->_client_max_body_size = body_size;
}

void Location::setMaxBodySize(unsigned long parametr)
{
	this->_client_max_body_size = parametr;
}

const	std::string &Location::getPath() const
{
	return (this->_path);
}

const	std::string &Location::getRootLocation() const
{
	return (this->_root);
}

const	std::string &Location::getIndexLocation() const
{
	return (this->_index);
}

const	std::vector<short> &Location::getMethods() const
{
	return (this->_methods);
}

const	std::vector<std::string> &Location::getCgiPath() const
{
	return (this->_cgi_path);
}

const	std::vector<std::string> &Location::getCgiExtension() const
{
	return (this->_cgi_ext);
}

const	bool &Location::getAutoindex() const
{
	return (this->_autoindex);
}

const	std::string &Location::getReturn() const
{
	return (this->_return);
}

const	std::string &Location::getAlias() const
{
	return (this->_alias);
}

const	std::map<std::string, std::string> &Location::getExtensionPath() const
{
	return (this->_ext_path);
}

const	unsigned long &Location::getMaxBodySize() const
{
	return (this->_client_max_body_size);
}

// Retourne une string représentant les méthodes HTTP autorisées pour la location

std::string Location::getPrintMethods() const
{
	std::string	res;
	
	if (_methods[4])
		res.insert(0, "HEAD");

	if (_methods[3])
	{
		if (!res.empty())
			res.insert(0, ", ");
		res.insert(0, "PUT");
	}

	if (_methods[2])
	{
		if (!res.empty())
			res.insert(0, ", ");
		res.insert(0, "DELETE");
	}

	if (_methods[1])
	{
		if (!res.empty())
			res.insert(0, ", ");
		res.insert(0, "POST");
	}

	if (_methods[0])
	{
		if (!res.empty())
			res.insert(0, ", ");
		res.insert(0, "GET");
	}

	return (res);
}