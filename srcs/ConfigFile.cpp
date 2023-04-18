/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigFile.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lsalin <lsalin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/07 14:34:29 by lsalin            #+#    #+#             */
/*   Updated: 2023/04/18 12:27:38 by lsalin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigFile.hpp"

ConfigFile::ConfigFile() : _size(0) {}

ConfigFile::ConfigFile(std::string const path) : _path(path), _size(0) {}

ConfigFile::~ConfigFile() {}

/**
	@brief Détermine le type du chemin fourni en paramètre
	@brief stat : fonction qui remplit la structure stat avec les infos sur un dossier

	@return (1) si c'est un fichier, (2) si c'est un dossier et (3) si c'est ni l'un ni l'autre
	@return (-1) si échec
 */

int	ConfigFile::getTypePath(std::string const path)
{
	struct stat	buffer;
	int			result;

	result = stat(path.c_str(), &buffer);

	// Utilisation du "ET logique" (&) pour tester 
	// si le bit en question est activé ou non

	if (result == 0)
	{
		if (buffer.st_mode & S_IFREG)
			return (1);
		else if (buffer.st_mode & S_IFDIR)
			return (2);
		else
			return (3);
	}
	else
		return (-1);
}

// Retourne (0) si le fichier existe et que l'user a les permissions nécessaires 
// en fonction d'un certain mode
// Sinon (-1)

int	ConfigFile::checkFile(std::string const path, int mode)
{
	return (access(path.c_str(), mode));
}

/**
	@brief Check un fichier existe et si l'user a les permissions nécessaires pour le lire
	
 	@param path : chemin du fichier
	@param index : nom du fichier

	@return (0) si le fichier existe et que l'user a les permissions, (-1) sinon
*/

int	ConfigFile::isFileExistAndReadable(std::string const path, std::string const index)
{
	// Si index est un fichier et a les permissions de lecture (mode 4)
	if (getTypePath(index) == 1 && checkFile(index, 4) == 0)
		return (0);

	// path + index = chemin complet du fichier par concaténation
	if (getTypePath(path + index) == 1 && checkFile(path + index, 4) == 0)
		return (0);

	return (-1);
}

std::string	ConfigFile::getPath()
{
	return (this->_path);
}

int	ConfigFile::getSize()
{
	return (this->_size);
}