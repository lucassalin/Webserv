/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerConfig.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lsalin <lsalin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/05 15:22:41 by lsalin            #+#    #+#             */
/*   Updated: 2023/04/08 15:38:49 by lsalin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include "Webserv.hpp"

static std::string	serverParametrs[] = {"server_name", "listen", "root", "index", "allow_methods", "client_body_buffer_size"};

class Location;

// Stocke et gére la configuration du serveur web

class ServerConfig
{
	private:
		// nᵒ de port sur lequel le serveur ecoute les connexions entrantes
		// uint16_t car les nᵒ de ports valides vont de 0 à 65535
		uint16_t						_port;
		in_addr_t						_host; // adresse IP de la machine sur laquelle le serveur s'execute
		std::string						_server_name;
		std::string						_root; // repertoire racine du serveur
		unsigned long					_client_max_body_size;
		std::string						_index; // nom du fichier index par defaut
		bool							_autoindex; 
		std::map<short, std::string>	_error_pages; // stocke les pages d'erreur pour chaque code d'etat HTTP
		std::vector<Location> 			_locations; // configurations de l'emplacement associees au serveur
		struct sockaddr_in 				_server_address; // stocke les infos de l'adresse du serveur
		int								_listen_fd; // fd pour la socket d'ecoute du serveur

	public:
		ServerConfig();
		~ServerConfig();
		ServerConfig(const ServerConfig &other);
		ServerConfig &operator=(const ServerConfig & rhs);

		void initErrorPages(void);

		void setServerName(std::string server_name);
		void setHost(std::string parametr);
		void setRoot(std::string root);
		void setFd(int);
		void setPort(std::string parametr);
		void setClientMaxBodySize(std::string parametr);
		void setErrorPages(std::vector<std::string> &parametr);
		void setIndex(std::string index);
		void setLocation(std::string nameLocation, std::vector<std::string> parametr);
		void setAutoindex(std::string autoindex);

		bool isValidHost(std::string host) const;
		bool isValidErrorPages();
		int isValidLocation(Location &location) const;

		const std::string &getServerName();
		const uint16_t &getPort();
		const in_addr_t &getHost();
		const size_t &getClientMaxBodySize();
		const std::vector<Location> &getLocations();
		const std::string &getRoot();
		const std::map<short, std::string> &getErrorPages();
		const std::string &getIndex();
		const bool &getAutoindex();
		const std::string &getPathErrorPage(short key);
		const std::vector<Location>::iterator getLocationKey(std::string key);

		static void checkToken(std::string &parametr);
		bool checkLocaitons() const;

		public:
		class ErrorException : public std::exception
		{
			private:
				std::string _message;
			public:
				ErrorException(std::string message) throw()
				{
					_message = "SERVER CONFIG ERROR: " + message;
				}
				virtual const char* what() const throw()
				{
					return (_message.c_str());
				}
				virtual ~ErrorException() throw() {}
		};

		void	setupServer();
		int		getFd();
};

#endif