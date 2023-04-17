/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Location.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lsalin <lsalin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/05 15:24:47 by lsalin            #+#    #+#             */
/*   Updated: 2023/04/17 14:44:51 by lsalin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LOCATION_HPP
#define LOCATION_HPP

#include "Webserv.hpp"

// Définit une configuration pour chaque ressource pouvant être demandée par un client

class Location
{
	private:
		std::string					_path;		// chemin relatif
		std::string					_root;		// chemin absolu
		bool						_autoindex;	// auto-indexation activé ou non ?
		std::string					_index;		// nom du fichier à utiliser comme page d'accueil pour la location en question
		std::vector<short>			_methods;	// GET + POST + DELETE + PUT + HEAD
		std::string					_return;	// remplacement où renvoyer la requête pour cette location
		std::string					_alias;
		std::vector<std::string>	_cgi_path;
		std::vector<std::string>	_cgi_ext;	// extensions de fichiers pour les scripts CGI de cette location
		unsigned long				_client_max_body_size;

	public:
		std::map<std::string, std::string>	_ext_path; // key = extension de fichier ; value = path de ce fichier

		Location();
		Location(const Location &other);
		Location &operator=(const Location &rhs);
		~Location();

		void	setPath(std::string parametr);
		void	setRootLocation(std::string parametr);
		void	setMethods(std::vector<std::string> methods);
		void	setAutoindex(std::string parametr);
		void	setIndexLocation(std::string parametr);
		void	setReturn(std::string parametr);
		void	setAlias(std::string parametr);
		void	setCgiPath(std::vector<std::string> path);
		void	setCgiExtension(std::vector<std::string> extension);
		void	setMaxBodySize(std::string parametr);
		void	setMaxBodySize(unsigned long parametr);

		const std::string							&getPath() const;
		const std::string							&getRootLocation() const;
		const std::vector<short>					&getMethods() const;
		const bool									&getAutoindex() const;
		const std::string							&getIndexLocation() const;
		const std::string							&getReturn() const;
		const std::string							&getAlias() const;
		const std::vector<std::string>				&getCgiPath() const;
		const std::vector<std::string>				&getCgiExtension() const;
		const std::map<std::string, std::string>	&getExtensionPath() const;
		const unsigned long							&getMaxBodySize() const;

		std::string	getPrintMethods() const;
};

#endif