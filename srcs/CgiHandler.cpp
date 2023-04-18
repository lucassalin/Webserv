/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiHandler.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lsalin <lsalin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/07 11:01:04 by lsalin            #+#    #+#             */
<<<<<<< HEAD
/*   Updated: 2023/04/18 11:49:10 by lsalin           ###   ########.fr       */
=======
/*   Updated: 2023/04/17 21:17:04 by lsalin           ###   ########.fr       */
>>>>>>> 3a4cecf9757b6727f1efd4dc305e65e4c583d375
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

// Init les variables d'env pour le script CGI
// Utilise les informations de la requete HTTP et le path du script

void	CgiHandler::initEnvCgi(HttpRequest& req, const std::vector<Location>::iterator it_loc)
{
	std::string	cgi_exec = ("cgi-bin/" + it_loc->getCgiPath()[0]).c_str();
	char		*cwd = getcwd(NULL, 0);

	// cgi_exec 	= cgi-bin/script.cgi
	// _cgi_path	= script.cgi
	// cwd			= /usr/local/www
	// tmp			= /usr/local/www/
	// path absolu	= /usr/local/www/script.cgi

	// Si le path n'est pas absolu --> on le rends absolu

	if (_cgi_path[0] != '/')
	{
		std::string	tmp(cwd);
		tmp.append("/");

		if (_cgi_path.length() > 0)
			_cgi_path.insert(0, tmp);
	}

	// POST = donnees contenues dans le body
	// Donc ajout des deux variables d'env adequates : CONTENT_LENGTH et CONTENT_TYPE

	if (req.getMethod() == POST)
	{
		std::stringstream	out;
		out << req.getBody().length();

		this->_env["CONTENT_LENGTH"] = out.str();
		this->_env["CONTENT_TYPE"] = req.getHeader("content-type");
	}

	this->_env["GATEWAY_INTERFACE"] = std::string("CGI/1.1"); // version cgi
	this->_env["SCRIPT_NAME"] = cgi_exec;
	this->_env["SCRIPT_FILENAME"] = this->_cgi_path;
	this->_env["PATH_INFO"] = this->_cgi_path;
	this->_env["PATH_TRANSLATED"] = this->_cgi_path;
	this->_env["REQUEST_URI"] = this->_cgi_path;
	this->_env["SERVER_NAME"] = req.getHeader("host");
	this->_env["SERVER_PORT"] ="8002";
	this->_env["REQUEST_METHOD"] = req.getMethodStr();
	this->_env["SERVER_PROTOCOL"] = "HTTP/1.1";
	this->_env["REDIRECT_STATUS"] = "200";
	this->_env["SERVER_SOFTWARE"] = "42Paris";

	std::map<std::string, std::string> request_headers = req.getHeaders();

	// Parcourt les headers de la requete et les add au variables d'env du script
	for (std::map<std::string, std::string>::iterator it = request_headers.begin(); it != request_headers.end(); ++it)
	{
		std::string name = it->first;
		std::transform(name.begin(), name.end(), name.begin(), ::toupper); // headername en MAJ (syntaxe env)
		std::string key = "HTTP_" + name; // name = User-agent, key = HTTP_USER-AGENT (convention)
		_env[key] = it->second; // ajoute la valeur du header aux variables d'env
	}

	this->_ch_env = (char **)calloc(sizeof(char *), this->_env.size() + 1);
	std::map<std::string, std::string>::const_iterator	it = this->_env.begin();

	// _ch_env = tableau de strings "env_name=value"
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

// Initialise les variables d'env du script 
// en fonction de l'extension du fichier de config !

void	CgiHandler::initEnv(HttpRequest& req, const std::vector<Location>::iterator it_loc)
{
	int			poz;		// position de la sous-chaine "cgi-bin/"" dans _cgi_path
	std::string extension;	// extension du fichier de config CGI (.py)
	std::string ext_path;	// path du fichier de config CGI

	extension = this->_cgi_path.substr(this->_cgi_path.find("."));
	// Recherche dans la map un element correspondant a la key "extension"
	std::map<std::string, std::string>::iterator it_path = it_loc->_ext_path.find(extension);

	// si l'extension n'est pas presente dans la map _ext_path
	if (it_path == it_loc->_ext_path.end())
		return;

	ext_path = it_loc->_ext_path[extension];

	this->_env["AUTH_TYPE"] = "Basic"; // Basic = authentification via username + mdp
	this->_env["CONTENT_LENGTH"] = req.getHeader("content-length");
	this->_env["CONTENT_TYPE"] = req.getHeader("content-type");
	this->_env["GATEWAY_INTERFACE"] = "CGI/1.1";
	poz = findStart(this->_cgi_path, "cgi-bin/");
	this->_env["SCRIPT_NAME"] = this->_cgi_path;
	this->_env["SCRIPT_FILENAME"] = ((poz < 0 || (size_t)(poz + 8) > this->_cgi_path.size()) ? "" : this->_cgi_path.substr(poz + 8, this->_cgi_path.size()));
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
	this->_env["SERVER_SOFTWARE"] = "42Paris";

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