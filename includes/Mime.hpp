/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Mime.hpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lsalin <lsalin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/05 15:25:01 by lsalin            #+#    #+#             */
/*   Updated: 2023/04/12 14:16:22 by lsalin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MIME_HPP
#define MIME_HPP

#include "Webserv.hpp"

// Fournit une correspondance entre les extensions de fichiers 
// et les types MIME correspondants

class Mime
{
	public:
		Mime();
		std::string	getMimeType(std::string extension);

	private:
		std::map<std::string, std::string>	_mime_types;
		
};

#endif