/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lsalin <lsalin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/10 14:11:00 by lsalin            #+#    #+#             */
/*   Updated: 2023/04/20 12:17:13 by lsalin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include "Webserv.hpp"

class ServerConfig;

// Lit et traite le fichier de config pour définir les paramètres
// de chaque serveurs web créés

class ConfigParser
{
	private:
		std::vector<ServerConfig>	_servers;		// chaque _servers représente une configuration du serveur
		std::vector<std::string>	_server_config;	// contient des strings représentant le contenu du fichier de config
		size_t						_nb_server;		// nombre de serveurs configurés

	public:

		ConfigParser();
		~ConfigParser();

		int 						createCluster(const std::string &config_file);
		void						splitServers(std::string &content);
		void						removeComments(std::string &content);
		void						removeWhiteSpace(std::string &content);
		size_t						findStartServer(size_t start, std::string &content);
		size_t						findEndServer(size_t start, std::string &content);
		void						createServer(std::string &config, ServerConfig &server);
		void						checkServers();
		std::vector<ServerConfig>	getServers();
		int							stringCompare(std::string str1, std::string str2, size_t pos);
		int 						print();

		public:
		class ErrorException : public std::exception
		{
			private:
				std::string _message;

			public:
				ErrorException(std::string message) throw()
				{
					_message = "CONFIG PARSER ERROR: " + message;
				}
				virtual const char* what() const throw()
				{
					return (_message.c_str());
				}

				virtual ~ErrorException() throw() {}
		};
};

#endif