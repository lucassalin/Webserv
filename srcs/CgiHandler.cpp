/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiHandler.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lsalin <lsalin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/07 11:01:04 by lsalin            #+#    #+#             */
/*   Updated: 2023/04/08 14:47:34 by lsalin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CgiHandler.hpp"

// Constructeur par defaut
CgiHandler::CgiHandler()
{
	this->_cgi_pid = -1;
	this->_exit_status = 0;
	this->_cgi_path = "";
	this->_ch_env = NULL;
	this->_argv = NULL;
}

CgiHandler::CgiHandler(std::string path)
{
	this->_cgi_pid = -1;
	this->_exit_status = 0;
	this->_cgi_path = path;
	this->_ch_env = NULL;
	this->_argv = NULL;
}

// Destructeur --> free
CgiHandler::~CgiHandler()
{

	if (this->_ch_env)
	{
		for (int i = 0; this->_ch_env[i]; i++)
			free(this->_ch_env[i]);
		free(this->_ch_env);
	}

	if (this->_argv)
	{
		for (int i = 0; this->_argv[i]; i++)
			free(_argv[i]);
		free(_argv);
	}

	this->_env.clear();
}

// Constructeur de copie
CgiHandler::CgiHandler(const CgiHandler &other)
{
		this->_env = other._env;
		this->_ch_env = other._ch_env;
		this->_argv = other._argv;
		this->_cgi_path = other._cgi_path;
		this->_cgi_pid = other._cgi_pid;
		this->_exit_status = other._exit_status;
}

CgiHandler &CgiHandler::operator=(const CgiHandler &rhs)
{
	if (this != &rhs)
	{
		this->_env = rhs._env;
		this->_ch_env = rhs._ch_env;
		this->_argv = rhs._argv;
		this->_cgi_path = rhs._cgi_path;
		this->_cgi_pid = rhs._cgi_pid;
		this->_exit_status = rhs._exit_status;
	}
	return (*this);
}

void	CgiHandler::setCgiPid(pid_t cgi_pid)
{
	this->_cgi_pid = cgi_pid;
}

void	CgiHandler::setCgiPath(const std::string &cgi_path)
{
	this->_cgi_path = cgi_path;
}

const std::map<std::string, std::string>	&CgiHandler::getEnv() const
{
	return (this->_env);
}

const pid_t	&CgiHandler::getCgiPid() const
{
	return (this->_cgi_pid);
}

const std::string	&CgiHandler::getCgiPath() const
{
	return (this->_cgi_path);
}

/**
 @brief Initialise les variables d'environnement nécessaires 
 		à l'exécution du script CGI situees dans cgi-bin 

 @param req : reference sur un objet HttpRequest
 @param it_loc : iterateur vers un objet Location
 */

void	CgiHandler::initEnvCgi(HttpRequest& req, const std::vector<Location>::iterator it_loc)
{
	std::string	cgi_exec = ("cgi-bin/" + it_loc->getCgiPath()[0]).c_str(); // cgi-bin/script.cgi
	char		*cwd = getcwd(NULL, 0); // /homes/lsalin/serveur/cgi-bin/script.cgi

	// Cas du path qui n'est pas absolu

	// _cgi_path	= script.cgi
	// cwd			= /usr/local/www
	// tmp			= /usr/local/www/
	// path absolu	= /usr/local/www/script.cgi

	if (_cgi_path[0] != '/')
	{
		std::string	tmp(cwd);
		tmp.append("/");

		if(_cgi_path.length() > 0)
			_cgi_path.insert(0, tmp);
	}

	// POST = datas dans le body
	// Notre script CGI doit etre capable de les recuperer via l'env
	// On en init deux : CONTENT_LENGTH (longueur body) et CONTENT_TYPE (type du contenu du body)

	if (req.getMethod() == POST)
	{
		std::stringstream	out;
		out << req.getBody().length();

		// Creation de deux variables d'env
		this->_env["CONTENT_LENGTH"] = out.str();
		this->_env["CONTENT_TYPE"] = req.getHeader("content-type");
	}

	this->_env["GATEWAY_INTERFACE"] = std::string("CGI/1.1"); // ve de l'interface CGI compatible avec serveur
	this->_env["SCRIPT_NAME"] = cgi_exec;
	this->_env["SCRIPT_FILENAME"] = this->_cgi_path;
	this->_env["PATH_INFO"] = this->_cgi_path;
	this->_env["PATH_TRANSLATED"] = this->_cgi_path;
	this->_env["REQUEST_URI"] = this->_cgi_path;		// URL complet de la requete
	this->_env["SERVER_NAME"] = req.getHeader("host");
	this->_env["SERVER_PORT"] ="8002";
	this->_env["REQUEST_METHOD"] = req.getMethodStr();
	this->_env["SERVER_PROTOCOL"] = "HTTP/1.1";
	this->_env["REDIRECT_STATUS"] = "200";
	this->_env["SERVER_SOFTWARE"] = "AMANIX";

	std::map<std::string, std::string> request_headers = req.getHeaders();

	// Parcourt tous les headers de la requete
	// Et les ajoute aux variables d'environnement du script

	for(std::map<std::string, std::string>::iterator it = request_headers.begin();
		it != request_headers.end(); ++it)
	{
		std::string name = it->first;
		// nom du header en majuscule car syntaxe variable d'env
		std::transform(name.begin(), name.end(), name.begin(), ::toupper);
		// si name = user-agent, key = HTTP_USER-AGENT (convention)
		std::string key = "HTTP_" + name;
		_env[key] = it->second; // ajoute la valeur du header aux variables d'env
	}

	// calloc car init la zone memoire a 0
	this->_ch_env = (char **)calloc(sizeof(char *), this->_env.size() + 1);
	std::map<std::string, std::string>::const_iterator	it = this->_env.begin();

	// Creer un tableau de strings contenant les variables d'env au format "name=valeur"
	// "CONTENT_LENGTH=10" ; "GATEWAY_INTERFACE=CGI/1.1" ...

	for (int i = 0; it != this->_env.end(); it++, i++)
	{
		std::string tmp = it->first + "=" + it->second;
		this->_ch_env[i] = strdup(tmp.c_str());
	}

	// argv[0] = path d'acces au fichier executable du script
	// argv[1] = path d'acces au script
	// argv[2] = fin de la liste donc NULL

	this->_argv = (char **)malloc(sizeof(char *) * 3);
	this->_argv[0] = strdup(cgi_exec.c_str());
	this->_argv[1] = strdup(this->_cgi_path.c_str());
	this->_argv[2] = NULL;
}

// Initialise l'env pour les scripts CGI situes dans les emplacements specifies par les blocs
// "location" dans le fichier de config

void	CgiHandler::initEnv(HttpRequest& req, const std::vector<Location>::iterator it_loc)
{
	int			poz;		// stocke la position de la sous-chaine "cgi-bin/"" dans _cgi_path
	std::string extension;	// stocke l'extension du fichier de config CGI (.py)
	std::string ext_path;	// path du fichier de config CGI

	extension = this->_cgi_path.substr(this->_cgi_path.find("."));
	// Recherche dans la map un element correspondant a la key "extension"
	std::map<std::string, std::string>::iterator it_path = it_loc->_ext_path.find(extension);
	
	// si l'extension n'est pas presente dans la map _ext_path
	// --> le fichier CGI associe n'est pas autorise
	if (it_path == it_loc->_ext_path.end())
		return;

	ext_path = it_loc->_ext_path[extension];

	this->_env["AUTH_TYPE"] = "Basic"; // Basic = authentification via username + mdp
	this->_env["CONTENT_LENGTH"] = req.getHeader("content-length");
	this->_env["CONTENT_TYPE"] = req.getHeader("content-type");
	this->_env["GATEWAY_INTERFACE"] = "CGI/1.1";
	poz = findStart(this->_cgi_path, "cgi-bin/");
	this->_env["SCRIPT_NAME"] = this->_cgi_path;
	this->_env["SCRIPT_FILENAME"] = ((poz < 0 || (size_t)(poz + 8) > this->_cgi_path.size()) ? "" : this->_cgi_path.substr(poz + 8, this->_cgi_path.size())); // check dif cases after put right parametr from the response
	this->_env["PATH_INFO"] = getPathInfo(req.getPath(), it_loc->getCgiExtension());
	this->_env["PATH_TRANSLATED"] = it_loc->getRootLocation() + (this->_env["PATH_INFO"] == "" ? "/" : this->_env["PATH_INFO"]);
	this->_env["QUERY_STRING"] = decode(req.getQuery());
	this->_env["REMOTE_ADDR"] = req.getHeader("host");
	poz = findStart(req.getHeader("host"), ":");
	this->_env["SERVER_NAME"] = (poz > 0 ? req.getHeader("host").substr(0, poz) : "");
	this->_env["SERVER_PORT"] = (poz > 0 ? req.getHeader("host").substr(poz + 1, req.getHeader("host").size()) : "");
	this->_env["REQUEST_METHOD"] = req.getMethodStr();
	this->_env["HTTP_COOKIE"] = req.getHeader("cookie");
	this->_env["DOCUMENT_ROOT"] = it_loc->getRootLocation();
	this->_env["REQUEST_URI"] = req.getPath() + req.getQuery();
	this->_env["SERVER_PROTOCOL"] = "HTTP/1.1";
	this->_env["REDIRECT_STATUS"] = "200";
	this->_env["SERVER_SOFTWARE"] = "AMANIX";

	this->_ch_env = (char **)calloc(sizeof(char *), this->_env.size() + 1);
	std::map<std::string, std::string>::const_iterator it = this->_env.begin();

	for (int i = 0; it != this->_env.end(); it++, i++)
	{
		std::string tmp = it->first + "=" + it->second;
		this->_ch_env[i] = strdup(tmp.c_str());
	}

	this->_argv = (char **)malloc(sizeof(char *) * 3);
	this->_argv[0] = strdup(ext_path.c_str());
	this->_argv[1] = strdup(this->_cgi_path.c_str());
	this->_argv[2] = NULL;
}

// Execute le script CGI

void	CgiHandler::execute(short &error_code)
{
	// 500 = Internal Server Error
	if (this->_argv[0] == NULL || this->_argv[1] == NULL)
	{
		error_code = 500;
		return;
	}

	if (pipe(pipe_in) < 0)
	{
		Logger::logMsg(RED, CONSOLE_OUTPUT, "pipe() failed");

		error_code = 500;
		return;
	}

	// processus deja créé donc on le close si fail
	if (pipe(pipe_out) < 0)
	{
		Logger::logMsg(RED, CONSOLE_OUTPUT, "pipe() failed");

		close(pipe_in[0]);
		close(pipe_in[1]);

		error_code = 500;
		return ;
	}

	this->_cgi_pid = fork();

	if (this->_cgi_pid == 0)
	{
		dup2(pipe_in[0], STDIN_FILENO);
		dup2(pipe_out[1], STDOUT_FILENO);

		close(pipe_in[0]);
		close(pipe_in[1]);
		close(pipe_out[0]);
		close(pipe_out[1]);

		this->_exit_status = execve(this->_argv[0], this->_argv, this->_ch_env);
		exit(this->_exit_status);
	}

	else if (this->_cgi_pid > 0) {}

	else
	{
		std::cout << "Fork failed" << std::endl;
		error_code = 500;
	}
}

/**
	@brief Recherche la position du début d'une sous-chaîne (delim) dans path

	@return la position de la premiere occurence de delim dans path si trouve
			Sinon (-1)
*/

int	CgiHandler::findStart(const std::string path, const std::string delim)
{
	if (path.empty())
		return (-1);

	size_t poz = path.find(delim);

	if (poz != std::string::npos)
		return (poz);
	else
		return (-1);
}

/**
	@brief Remplace les caractères encodes en % dans l'URL de la requête par leur equivalent ASCII
		   Ils representent les caracteres speciaux non autorises dans une URL
		   %XY où XY est le code hexa du caractere

	@example /search?q=hello%20world&lang=en%2Cfr%2Cde --> /search?q=hello world&lang=en,fr,de
			 32 (espace)= equivalent decimal de 20
*/

std::string	CgiHandler::decode(std::string &path)
{
	size_t	token = path.find("%");

	while (token != std::string::npos)
	{
		if (path.length() < token + 2) // encodage non valide
			break;

		char	decimal = fromHexToDec(path.substr(token + 1, 2));
		path.replace(token, 3, toString(decimal));
		token = path.find("%");
	}

	return (path);
}

// TODO

std::string CgiHandler::getPathInfo(std::string& path, std::vector<std::string> extensions)
{
	std::string	tmp;
	size_t		start, end;

	for (std::vector<std::string>::iterator it_ext = extensions.begin(); it_ext != extensions.end(); it_ext++)
	{
		start = path.find(*it_ext);

		if (start != std::string::npos)
			break;
	}
	
	if (start == std::string::npos)
		return "";

	if (start + 3 >= path.size())
		return "";

	tmp = path.substr(start + 3, path.size());

	if (!tmp[0] || tmp[0] != '/')
		return "";
	end = tmp.find("?");

	return (end == std::string::npos ? tmp : tmp.substr(0, end));
}

void	CgiHandler::clear()
{
	this->_cgi_pid = -1;
	this->_exit_status = 0;
	this->_cgi_path = "";
	this->_ch_env = NULL;
	this->_argv = NULL;
	this->_env.clear();
}
