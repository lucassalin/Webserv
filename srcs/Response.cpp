/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lsalin <lsalin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/07 10:49:55 by lsalin            #+#    #+#             */
/*   Updated: 2023/04/27 13:51:29 by lsalin           ###   ########.fr       */
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
		_response_content.append(_mime.getMimeType(_target_file.substr(_target_file.rfind(".", std::string::npos))));

	else
		_response_content.append(_mime.getMimeType("default"));

	_response_content.append("\r\n");
}

// Ajoute le header "Content-Length" à la réponse en spécifiant la taille du contenu en octets
void	Response::contentLength()
{
	std::stringstream	ss;
	ss << _response_body.length();

	_response_content.append("Content-Length: ");
	_response_content.append(ss.str());
	_response_content.append("\r\n");
}

void	Response::connection()
{
	if (request.getHeader("connection") == "keep-alive")
		_response_content.append("Connection: keep-alive\r\n");
}

void	Response::server()
{
	_response_content.append("Server: 42Paris\r\n");
}

void	Response::location()
{
	if (_location.length())
		_response_content.append("Location: "+ _location +"\r\n");
}

// Ajoute le header "Date" à la réponse
// Date: Mon, 04 Apr 2023 12:34:56 UTC
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

// Renvoie true si le path du fichier correspond a un répertoire
// Sinon false
static bool	isDirectory(std::string path)
{
	// stat = structure contenant des informations sur un fichier (taille, heure de modif, permissions...)
	struct stat	file_stat;
	
	if (stat(path.c_str(), &file_stat) != 0)
		return (false);

	return (S_ISDIR(file_stat.st_mode));
}

// Check si la methode utilisee est autorisee en fonction de son emplacement
// Retourne true + affecte 405 à code ("Method Not Allowed") si pas autorisée
// Retourne false sinon

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
					GET /old-pages/some-page.html HTTP/1.1
					Host: example.com

			 REPONSE
					HTTP/1.1 301 Moved Permanently
					Location: /new-pages/some-page.html
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

	// Si un path finit par "/" et que l'autre commence par "/"
	// On supprime un "/" pour éviter les doublons

	if (p1[len1 - 1] == '/' && (!p2.empty() && p2[0] == '/'))
		p2.erase(0, 1);

	if (p1[len1 - 1] != '/' && (!p2.empty() && p2[0] != '/'))
		p1.insert(p1.end(), '/');

	if (p2[len2 - 1] == '/' && (!p3.empty() && p3[0] == '/'))
		p3.erase(0, 1);

	if (p2[len2 - 1] != '/' && (!p3.empty() && p3[0] != '/'))
		p2.insert(p1.end(), '/');

	res = p1 + p2 + p3;
	return (res);
}

// Remplace l'alias de l'emplacement par le path réel du fichier demande dans la requete
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

// Initialise les variables d'env pour le script CGI, et l'execute
// Utilise le chemin d'accès complet de la ressource demandée

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

// Même objectif mais :
// 1) Extrait le chemin d'accès de la ressource
// 2) Vérifie qu'il se termine par la bonne extension (.py ou .sh)
// 3) Ajoute le chemin d'accès à la localisation CGI spécifiée dans fichier de config
// 4) Vérifie son accessibilité et ses permissions

int	Response::handleCgi(std::string &location_key)
{
	std::string path;
	std::string exten;
	size_t		pos;

	path = this->request.getPath();

	// si le premier char existe et que c'est "/", on l'enlève
	if (path[0] && path[0] == '/')
		path.erase(0, 1);

	// si la requête doit être traitée par un CGI
	// on ajoute sa localisation spécifique sur le serveur web
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

/**
	@brief Compare l'URI avec les locations du fichier de config et essaie de trouver la meilleure correspondance.
		   Si une correspondance est trouvée, location_key est défini sur cet emplacement.
		   Sinon, elle est vide.

	@example path = /html/index.html

	locations = {
    {
        path = "/",
        ...
    },
    {
        path = "/html",
        ...
    },
    {
        path = "/cgi-bin",
        ...
    }
}
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

// Analyse la requête HTTP reçue et décide comment y répondre
// Renvoie (0) si la requête est traitée avec succès
// 1 si erreur (_code = code de statut approprié)

int	Response::handleTarget()
{
	std::string	location_key;
	getLocationMatch(request.getPath(), _server.getLocations(), location_key);

	if (location_key.length() > 0)
	{
		// target_location contient l'objet Location correspondant à la key location_key
		// Rappel : Location contient les infos de config de la requête HTTP en cours
		// (path de l'emplacement, méthode autorisée, extensions CGI autorisées ...)

		Location	target_location = *_server.getLocationKey(location_key);

		if (isAllowedMethod(request.getMethod(), target_location, _code))
		{
			std::cout << "METHOD NOT ALLOWED \n";
			return (1);
		}

		if (request.getBody().length() > target_location.getMaxBodySize())
		{
			_code = 413;
			return (1);
		}

		if (checkReturn(target_location, _code, _location))
			return (1);

		if (target_location.getPath().find("cgi-bin") != std::string::npos)
			return (handleCgi(location_key));

		if (!target_location.getAlias().empty())
			replaceAlias(target_location, request, _target_file);

		else
			appendRoot(target_location, request, _target_file);

		if (!target_location.getCgiExtension().empty())
		{
			if (_target_file.rfind(target_location.getCgiExtension()[0]) != std::string::npos)
				return (handleCgiTemp(location_key));
		}

		if (isDirectory(_target_file))
		{
			if (_target_file[_target_file.length() - 1] != '/')
			{
				_code = 301;
				_location = request.getPath() + "/";
				return (1);
			}

			if (!target_location.getIndexLocation().empty())
				_target_file += target_location.getIndexLocation();

			else
				_target_file += _server.getIndex();

			if (!fileExists(_target_file))
			{
				if (target_location.getAutoindex())
				{
					_target_file.erase(_target_file.find_last_of('/') + 1);
					_auto_index = true;
					return (0);
				}

				else
				{
					_code = 403;
					return (1);
				}
			}

			if (isDirectory(_target_file))
			{
				_code = 301;

				if (!target_location.getIndexLocation().empty())
					_location = combinePaths(request.getPath(), target_location.getIndexLocation(), "");
					
				else
					_location = combinePaths(request.getPath(), _server.getIndex(), "");
					
				if (_location[_location.length() - 1] != '/')
					_location.insert(_location.end(), '/');

				return (1);
			}
		}
	}
	
	else
	{
		_target_file = combinePaths(_server.getRoot(), request.getPath(), "");

		if (isDirectory(_target_file))
		{
			if (_target_file[_target_file.length() - 1] != '/')
			{
				_code = 301;
				_location = request.getPath() + "/";
				return (1);
			}

			_target_file += _server.getIndex();

			if (!fileExists(_target_file))
			{
				_code = 403;
				return (1);
			}

			if (isDirectory(_target_file))
			{
				_code = 301;
				_location = combinePaths(request.getPath(), _server.getIndex(), "");

				if (_location[_location.length() - 1] != '/')
					_location.insert(_location.end(), '/');
				return (1);
			}
		}
	}
	return (0);
}

bool	Response::reqError()
{
	if (request.errorCode())
	{
		_code = request.errorCode();
		return (1);
	}
	return (0);
}

void	Response::setServerDefaultErrorPages()
{
	_response_body = getErrorPage(_code);
}

/**
	@brief Est call lorsqu'une erreur est détectée dans la requête (404 par exemple)

	@example	HTTP/1.1 404 Not Found
			 	Content-Type: text/html

				<!DOCTYPE html>
				<html>
				<head>
					<title>404 Not Found</title>
				</head>
				<body>
					<h1>404 Not Found</h1>
				<p>The requested URL was not found on this server.</p>
				</body>
				</html>
 */

void	Response::buildErrorBody()
{
		// rappel : DELETE et POST sont idempotente donc ne renvoient pas de page d'erreur
		if (!_server.getErrorPages().count(_code) || _server.getErrorPages().at(_code).empty() ||
		 request.getMethod() == DELETE || request.getMethod() == POST)
		{
			setServerDefaultErrorPages();
		}

		// une page d'erreur personnalisée est définie
		else 
		{
			// Redirige le client vers une page d'erreur personnalisée
			if (_code >= 400 && _code < 500)
			{
				// _location = URL de la page d'erreur correspondant au code d'erreur
				_location = _server.getErrorPages().at(_code);

				if (_location[0] != '/')
					_location.insert(_location.begin(), '/');

				_code = 404;
			}

			_target_file = _server.getRoot() +_server.getErrorPages().at(_code);
			short	old_code = _code;

			// charge le contenu du fichier d'erreur personnalisé
			// dans la variable _response_body
			if (readFile())
			{
				_code = old_code;
				_response_body = getErrorPage(_code);
			}
		}
}

// Construit la réponse HTTP à renvoyée au client
void	Response::buildResponse()
{
	if (reqError() || buildBody())
		buildErrorBody();

	// car réponse générée par script cgi
	if (_cgi)
		return;

	else if (_auto_index)
	{
		std::cout << "AUTO index " << std::endl;

		
		// si echec du build html
		if (buildHtmlIndex(_target_file, _body, _body_length))
		{
			_code = 500;
			buildErrorBody();
		}

		else
			_code = 200;
		_response_body.insert(_response_body.begin(), _body.begin(), _body.end());
	}

	setStatusLine();
	setHeaders();

	// HEAD = pas de body dans la réponse (que les headers)
	if (request.getMethod() != HEAD && (request.getMethod() == GET || _code != 200))
		_response_content.append(_response_body);
}

void	Response::setErrorResponse(short code)
{
	_response_content = "";
	_code = code;
	_response_body = "";
	buildErrorBody();
	setStatusLine();
	setHeaders();
	_response_content.append(_response_body);
}

// Retourne la réponse complète (headers + body)
std::string	Response::getRes()
{
	return (_response_content);
}

size_t	Response::getLen() const
{
	return (_response_content.length());
}

// Construit la ligne de statut basé sur le code de statut
// HTTP/1.1 404 Not Found\r\n
void	Response::setStatusLine()
{
	_response_content.append("HTTP/1.1 " + toString(_code) + " ");
	_response_content.append(statusCodeString(_code));
	_response_content.append("\r\n");
}

// Construit le corps de la réponse en fonction de la méthode de la requête
// et du code de réponse

int	Response::buildBody()
{
	if (request.getBody().length() > _server.getClientMaxBodySize())
	{
		_code = 413;
		return (1);
	}

	if (handleTarget())
		return (1);

	if (_cgi || _auto_index)
		return (0);

	if (_code)
		return (0);

	if (request.getMethod() == GET || request.getMethod() == HEAD)
	{
		if (readFile())
			return (1);
	}

	else if (request.getMethod() == POST || request.getMethod() == PUT)
	{
		if (fileExists(_target_file) && request.getMethod() == POST)
		{
			_code = 204;
			return (0);
		}

		std::ofstream file(_target_file.c_str(), std::ios::binary);

		if (file.fail())
		{
			_code = 404;
			return (1);
		}

		if (request.getMultiformFlag())
		{
			std::string body = request.getBody();
			body = removeBoundary(body, request.getBoundary());
			file.write(body.c_str(), body.length());
		}
		else
		{
			file.write(request.getBody().c_str(), request.getBody().length());
		}
	}

	else if (request.getMethod() == DELETE)
	{
		if (!fileExists(_target_file))
		{
			_code = 404;
			return (1);
		}
		if (remove(_target_file.c_str() ) != 0)
		{
			_code = 500;
			return (1);
		}
	}
	_code = 200;
	return (0);
}

// Ouvre le fichier correspondant à la requête dans _target_file, et lit son contenu
// Le stocke dans _response_body

int	Response::readFile()
{
	std::ifstream file(_target_file.c_str());

	if (file.fail())
	{
		_code = 404;
		return (1);
	}

	std::ostringstream	ss;
	
	ss << file.rdbuf();
	_response_body = ss.str();

	return (0);
}

void	Response::setServer(ServerConfig &server)
{
	_server = server;
}

void	Response::setRequest(HttpRequest &req)
{
	request = req;
}

void	Response::cutRes(size_t i)
{
	_response_content = _response_content.substr(i);
}

void   Response::clear()
{
	_target_file.clear();
	_body.clear();
	_body_length = 0;
	_response_content.clear();
	_response_body.clear();
	_location.clear();
	_code = 0;
	_cgi = 0;
	_cgi_response_length = 0;
	_auto_index = 0;
}

int	Response::getCode() const
{
	return (_code);
}

int	Response::getCgiState()
{
	return (_cgi);
}

/**
	@example 

	------WebKitFormBoundary7MA4YWxkTrZu0gW
Content-Disposition: form-data; name="username"

John Doe
------WebKitFormBoundary7MA4YWxkTrZu0gW
Content-Disposition: form-data; name="file"; filename="test.txt"
Content-Type: text/plain

Contenu du fichier test.txt

------WebKitFormBoundary7MA4YWxkTrZu0gW--

--> 

std::string body = "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
                   "Content-Disposition: form-data; name=\"username\"\r\n"
                   "\r\n"
                   "John Doe\r\n"
                   "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
                   "Content-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n"
                   "Content-Type: text/plain\r\n"
                   "\r\n"
                   "Contenu du fichier test.txt\r\n"
                   "------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n";

std::string boundary = "7MA4YWxkTrZu0gW";

std::string new_body = removeBoundary(body, boundary);

// new_body contient maintenant la chaîne suivante :
// "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
// "Content-Disposition: form-data; name=\"username\"\r\n"
// "\r\n"
// "John Doe\r\n"
// "------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n"

*/

std::string	Response::removeBoundary(std::string &body, std::string &boundary)
{
	std::string	buffer;
	std::string	new_body;
	std::string	filename;
	
	bool is_boundary = false;
	bool is_content = false;

	if (body.find("--" + boundary) != std::string::npos && body.find("--" + boundary + "--") != std::string::npos)
	{
		for (size_t i = 0; i < body.size(); i++)
		{
			buffer.clear();
			
			while(body[i] != '\n')
			{
				buffer += body[i];
				i++;
			}
			
			if (!buffer.compare(("--" + boundary + "--\r")))
			{
				is_content = true;
				is_boundary = false;
			}
			
			if (!buffer.compare(("--" + boundary + "\r")))
			{
				is_boundary = true;
			}
			
			if (is_boundary)
			{
				if (!buffer.compare(0, 31, "Content-Disposition: form-data;"))
				{
					size_t	start = buffer.find("filename=\"");

					if (start != std::string::npos)
					{
						size_t end = buffer.find("\"", start + 10);
						
						if (end != std::string::npos)
							filename = buffer.substr(start + 10, end);
					}
				}

				else if (!buffer.compare(0, 1, "\r") && !filename.empty())
				{
					is_boundary = false;
					is_content = true;
				}

			}
			else if (is_content)
			{
				if (!buffer.compare(("--" + boundary + "\r")))
				{
					is_boundary = true;
				}

				else if (!buffer.compare(("--" + boundary + "--\r")))
				{
					new_body.erase(new_body.end() - 1);
					break ;
				}

				else
					new_body += (buffer + "\n");
			}

		}
	}

	body.clear();
	return (new_body);
}

void	Response::setCgiState(int state)
{
	_cgi = state;
}