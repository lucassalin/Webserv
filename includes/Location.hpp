/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Location.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lsalin <lsalin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/05 15:24:47 by lsalin            #+#    #+#             */
/*   Updated: 2023/04/17 15:19:00 by lsalin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LOCATION_HPP
#define LOCATION_HPP

#include "Webserv.hpp"

// Gere les configs specifiques d'un emplacement dans notre serveur

class Location
{
	private:
		std::string					_path; // chemin relatif de l'emplacement
		std::string					_root; // chemin absolu de l'emplacement
		bool						_autoindex;
		std::string					_index; // nom du fichier à utiliser comme page d'accueil pour cet emplacement
		std::vector<short>			_methods;
		std::string					_return; // redirection a appliquer pour cet emplacement
		std::string					_alias;
		std::vector<std::string>	_cgi_path; // vecteur contenant les chemins des scripts CGI pour cet emplacement
		std::vector<std::string>	_cgi_ext; // vecteur contenant les extensions de fichiers pour les scripts CGI de cet emplacement
		unsigned long				_client_max_body_size; // Taille max des body des requetes acceptees pour cet emplacement

	public:
		std::map<std::string, std::string>	_ext_path;		// key = extension du fichier ; value = path du fichier

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