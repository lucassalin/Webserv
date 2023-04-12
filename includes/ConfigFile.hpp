/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigFile.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lsalin <lsalin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/07 14:33:47 by lsalin            #+#    #+#             */
/*   Updated: 2023/04/12 14:16:16 by lsalin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIGFILE_HPP
#define CONFIGFILE_HPP

#include "Webserv.hpp"

class ConfigFile 
{
	private:
		std::string		_path;
		size_t			_size;

	public:
		ConfigFile();
		ConfigFile(std::string const path);
		~ConfigFile();

		static int	getTypePath(std::string const path);
		static int	checkFile(std::string const path, int mode);
		std::string	readFile(std::string path);
		static int	isFileExistAndReadable(std::string const path, std::string const index);

		std::string	getPath();
		int			getSize();
};

#endif