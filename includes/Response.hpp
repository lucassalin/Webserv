/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lsalin <lsalin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/07 10:51:57 by lsalin            #+#    #+#             */
/*   Updated: 2023/04/18 16:20:53 by lsalin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "Webserv.hpp"
#include "HttpRequest.hpp"

class Response
{
	public:
		static Mime		_mime;
		Response();
		Response(HttpRequest&);
		~Response();

		std::string		getRes();
		size_t			getLen() const;
		int				getCode() const;

		void			setRequest(HttpRequest &);
		void			setServer(ServerConfig &);

		void			buildResponse();
		void			clear();
		void			handleCgi(HttpRequest&);
		void			cutRes(size_t);
		int				getCgiState();
		void			setCgiState(int);
		void			setErrorResponse(short code);

		CgiHandler		_cgi_obj;

		std::string		removeBoundary(std::string &body, std::string &boundary);
		std::string		_response_content;

		HttpRequest		request;

	private:
		ServerConfig			_server;
		std::string				_target_file;	// fichier demandé par le client
		std::vector<uint8_t>	_body;
		size_t					_body_length;
		std::string				_response_body;
		std::string				_location;		// emplacement correspondant a la requête
		short					_code;			// code de reponse
		char					*_res;			// contient la reponse

		int						_cgi;			// = 1 --> la reponse a la requete doit etre generee par un script CGI
		int						_cgi_fd[2];
		size_t					_cgi_response_length;
		bool					_auto_index;

		int		buildBody();
		size_t	file_size();
		void	setStatusLine();
		void	setHeaders();
		void	setServerDefaultErrorPages();
		int		readFile();
		void	contentType();
		void	contentLength();
		void	connection();
		void	server();
		void	location();
		void	date();
		int		handleTarget();
		void	buildErrorBody();
		bool	reqError();
		int		handleCgi(std::string &);
		int		handleCgiTemp(std::string &);
};

#endif