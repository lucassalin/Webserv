/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerManager.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lsalin <lsalin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/05 15:09:07 by lsalin            #+#    #+#             */
/*   Updated: 2023/04/12 09:34:22 by lsalin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVERMANAGER_HPP
#define SERVERMANAGER_HPP

#include "Webserv.hpp"
#include "Client.hpp"
#include "Response.hpp"

/**
 @brief Responsable du fonctionnement principal du/des serveur(s) web
 		1) Gére les connexions TCP et la communication serveur-clients
		2) Configure le serveur via le fichier de config
		3) Gére les requêtes entrantes/sortantes pour chaque client connecté
 */

class ServerManager
{
	public:
		ServerManager();
		~ServerManager();
		void	setupServers(std::vector<ServerConfig>);
		void	runServers();

	private:
		std::vector<ServerConfig>	_servers;
		std::map<int, ServerConfig>	_servers_map;	// associe un fd à la config de son serveur correspondant
		std::map<int, Client>		_clients_map;	// associe un fd à un objet Client
		fd_set						_recv_fd_pool;	// ensemble de fd pour les sockets en attente de lecture
		fd_set						_write_fd_pool;	// ensemble de fd pour les sockets en attente d'écriture
		int							_biggest_fd;	// nombre max de fd à surveiller par select()

		void	acceptNewConnection(ServerConfig &);
		void	checkTimeout();
		void	initializeSets();
		void	readRequest(const int &, Client &);
		void	handleReqBody(Client &);
		void	sendResponse(const int &, Client &);
		void	sendCgiBody(Client &, CgiHandler &);
		void	readCgiResponse(Client &, CgiHandler &);
		void	closeConnection(const int);
		void	assignServer(Client &);
		void	addToSet(const int , fd_set &);
		void	removeFromSet(const int , fd_set &);
};

#endif