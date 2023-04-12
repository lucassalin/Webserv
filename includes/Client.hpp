/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lsalin <lsalin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/10 14:11:05 by lsalin            #+#    #+#             */
/*   Updated: 2023/04/12 14:16:15 by lsalin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
# define CLIENT_HPP

#include "Webserv.hpp"
#include "HttpRequest.hpp"
#include "Response.hpp"

// Stocke toutes les informations relatives au client (socket, adresses...)
// Chaque client a un objet du serveur auquel il est connecté

class Client
{
	public:
		Client();
		Client(const Client &other);
		Client(ServerConfig &);
		Client	&operator=(const Client & rhs);
		~Client();

		const int					&getSocket() const;
		const struct sockaddr_in	&getAddress() const;
		const HttpRequest			&getRequest() const;
		const time_t				&getLastTime() const;

		void						setSocket(int &);
		void						setAddress(sockaddr_in &);
		void						setServer(ServerConfig &);
		void						buildResponse();
		void						updateTime();

		void						clearClient();
		Response					response;
		HttpRequest					request;
		ServerConfig				server; // configuration du serveur utilisée par le client

	private:
		int							_client_socket;
		struct sockaddr_in			_client_address;
		time_t						_last_msg_time; // dernière fois que le serveur a reçu un msg provenant du client
};

#endif