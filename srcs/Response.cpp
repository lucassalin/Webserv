/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lsalin <lsalin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/07 10:49:55 by lsalin            #+#    #+#             */
/*   Updated: 2023/04/11 11:13:58 by lsalin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Response.hpp"

// _mime = variable statique from Mime in Response
Mime Response::_mime;

Response::Response()
{
	_target_file = "";
	_body.clear();
	_body_length = 0;
	_response_content = "";
	_response_body = "";
	_location = "";
	_code = 0;
	_cgi = 0;
	_cgi_response_length = 0;
	_auto_index = 0;
}

Response::~Response() {}

Response::Response(HttpRequest &req) : request(req)
{
	_target_file = "";
	_body.clear();
	_body_length = 0;
	_response_content = "";
	_response_body = "";
	_location = "";
	_code = 0;
	_cgi = 0;
	_cgi_response_length = 0;
	_auto_index = 0;
}

/*

HTTP/1.1 200 OK
Date: Mon, 04 Apr 2023 12:34:56 UTC
Server: Apache/2.4.46
Location: https://www.example.com/newpage.html
Content-Type: text/html; charset=UTF-8
Content-Length: 1234
Connection: close

<!DOCTYPE html>
<html>
<head>
  <title>Exemple de page web</title>
</head>
<body>
  <h1>Bienvenue sur mon site web !</h1>
  <p>Ceci est un exemple de page web.</p>
</body>
</html>

*/

// Definit le type de contenu de la reponse

void	Response::contentType()
{
	_response_content.append("Content-Type: ");

	// si "." --> extension --> recuperer le type MIME correspondant
	if (_target_file.rfind(".", std::string::npos) != std::string::npos && _code == 200)
		_response_content.append(_mime.getMimeType(_target_file.substr(_target_file.rfind(".", std::string::npos))) );
	
	else
		_response_content.append(_mime.getMimeType("default"));

	_response_content.append("\r\n");
}

void	Response::contentLength()
{
	std::stringstream ss;
	ss << _response_body.length();

	_response_content.append("Content-Length: ");
	_response_content.append(ss.str());
	_response_content.append("\r\n");
}

void	Response::connection()
{
	if(request.getHeader("connection") == "keep-alive")
		_response_content.append("Connection: keep-alive\r\n");
}

void	Response::server()
{
	_response_content.append("Server: AMAnix\r\n");
}

void	Response::location()
{
	if (_location.length())
		_response_content.append("Location: "+ _location +"\r\n");
}

// Ajoute la date et l'heure courante au contenu de la reponse

void	Response::date()
{
	char		date[1000];
	time_t		now = time(0);
	struct tm	tm = *gmtime(&now);
	
	strftime(date, sizeof(date), "%a, %d %b %Y %H:%M:%S %Z", &tm);

	_response_content.append("Date: ");
	_response_content.append(date);
	_response_content.append("\r\n");

}

// uri encoding
void	Response::setHeaders()
{
	contentType();
	contentLength();
	connection();
	server();
	location();
	date();

	_response_content.append("\r\n");
}

// Renvoie true si le file existe, sinon false
static bool	fileExists (const std::string& f)
{
	std::ifstream	file(f.c_str());
	return (file.good());
}

// Renvoie true si le path du fichier correspond a un repertoire
// Sinon false

static bool	isDirectory(std::string path)
{
	// Stat = structure contenant des informations sur un fichier (taille, heure de modif, permissions...)
	struct stat	file_stat;
	
	if (stat(path.c_str(), &file_stat) != 0)
		return (false);

	return (S_ISDIR(file_stat.st_mode));
}

// Check si la methode utilisee est autorisee en fonction de sa location
// Retourne true + affecte 405 ("Method Not Allowed") a code si pas autorisee
// Renvoie false sinon

static bool	isAllowedMethod(HttpMethod &method, Location &location, short &code)
{
	std::vector<short>	methods = location.getMethods();

	if ((method == GET && !methods[0]) || (method == POST && !methods[1]) ||
	   (method == DELETE && !methods[2])|| (method == PUT && !methods[3])||
		(method == HEAD && !methods[4]))
	{
		code = 405;
		return (1);
	}

	return (0);
}

/**
	@brief Check si l'objet Location contient une redirection vers une autre ressource

	@return true si c'est le cas, false sinon

	@example REQUETE
					GET /index.php HTTP/1.1
					Host: www.example.org

			REPONSE
					HTTP/1.1 301 Moved Permanently
					Location: http://www.example.org/index.asp
*/

static bool	checkReturn(Location &loc, short &code, std::string &location)
{
	if (!loc.getReturn().empty())
	{
		code = 301;
		location = loc.getReturn();
		
		if (location[0] != '/')
			location.insert(location.begin(), '/');

		return (1);
	}
	return (0);
}

// Combine trois paths (p1, p2 et p3) afin d'en creer un unique
// Utile si images, feuilles de style CSS et fichiers JS pas au meme endroit

static std::string combinePaths(std::string p1, std::string p2, std::string p3)
{
	std::string	res;
	int			len1;
	int			len2;

	len1 = p1.length();
	len2 = p2.length();

	if (p1[len1 - 1] == '/' && (!p2.empty() && p2[0] == '/') )
		p2.erase(0, 1);

	if (p1[len1 - 1] != '/' && (!p2.empty() && p2[0] != '/'))
		p1.insert(p1.end(), '/');

	if (p2[len2 - 1] == '/' && (!p3.empty() && p3[0] == '/') )
		p3.erase(0, 1);

	if (p2[len2 - 1] != '/' && (!p3.empty() && p3[0] != '/'))
		p2.insert(p1.end(), '/');

	res = p1 + p2 + p3;
	return (res);
}

// Remplace l'alias de la location par le path reel du fichier demande dans la requete

static void	replaceAlias(Location &location, HttpRequest &request, std::string &target_file)
{
	target_file = combinePaths(location.getAlias(), request.getPath().substr(location.getPath().length()), "");
}

// Combine path d'acces racine de la localisation + path de l'URL de la requete HTTP
// pour former le path d'acces complet du fichier cible a recuperer

static void	appendRoot(Location &location, HttpRequest &request, std::string &target_file)
{
	target_file = combinePaths(location.getRootLocation(), request.getPath(), "");
}

// Est call lorsque la ressource demandée nécessite l'exécution d'un script CGI

int	Response::handleCgiTemp(std::string &location_key)
{
	std::string	path;
	path = _target_file;

	_cgi_obj.clear();
	_cgi_obj.setCgiPath(path);
	_cgi = 1;

	// Processus parent = serveur web
	// Processus fils = script CGI

	if (pipe(_cgi_fd) < 0)
	{
		_code = 500;
		return (1);
	}

	_cgi_obj.initEnvCgi(request, _server.getLocationKey(location_key));
	_cgi_obj.execute(this->_code);

	return (0);
}

int	Response::handleCgi(std::string &location_key)
{
	std::string path;
	std::string exten;
	size_t      pos;

	path = this->request.getPath();

	if (path[0] && path[0] == '/')
		path.erase(0, 1);

	if (path == "cgi-bin")
		path += "/" + _server.getLocationKey(location_key)->getIndexLocation();

	else if (path == "cgi-bin/")
		path.append(_server.getLocationKey(location_key)->getIndexLocation());

	pos = path.find(".");

	if (pos == std::string::npos)
	{
		_code = 501;
		return (1);
	}

	exten = path.substr(pos);

	if (exten != ".py" && exten != ".sh")
	{
		_code = 501;
		return (1);
	}

	if (ConfigFile::getTypePath(path) != 1)
	{
		_code = 404;
		return (1);
	}

	if (ConfigFile::checkFile(path, 1) == -1 || ConfigFile::checkFile(path, 3) == -1)
	{
		_code = 403;
		return (1);
	}

	if (isAllowedMethod(request.getMethod(), *_server.getLocationKey(location_key), _code))
		return (1);

	_cgi_obj.clear();
	_cgi_obj.setCgiPath(path);
	_cgi = 1;

	if (pipe(_cgi_fd) < 0)
	{
		_code = 500;
		return (1);
	}

	_cgi_obj.initEnv(request, _server.getLocationKey(location_key));
	_cgi_obj.execute(this->_code);

	return (0);
}

/*
	Compares URI with locations from config file and tries to find the best match.
	If match found, then location_key is set to that location, otherwise location_key will be an empty string.
*/

/**
	@brief Parcourt le vecteur 'locations' et compare le path de chaque objet Location avec le path
	de la requete

	@param path : path de la requete
	@param locations : vecteur d'objets Location, contient la configuration des emplacements
	des ressources sur le serveur
	@param location_key : key de config de l'emplacement des ressources sur le serveur correspondant
	le mieux au path de la requete
*/

static void	getLocationMatch(std::string &path, std::vector<Location> locations, std::string &location_key)
{
	size_t	biggest_match = 0;

	for(std::vector<Location>::const_iterator it = locations.begin(); it != locations.end(); ++it)
	{
		if (path.find(it->getPath()) == 0)
		{
			if (it->getPath() == "/" || path.length() == it->getPath().length()
				|| path[it->getPath().length()] == '/')
			{
				if (it->getPath().length() > biggest_match)
				{
					biggest_match = it->getPath().length();
					location_key = it->getPath();
				}
			}
		}
	}
}

