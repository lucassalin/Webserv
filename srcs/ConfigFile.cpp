/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigFile.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lsalin <lsalin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/07 14:34:29 by lsalin            #+#    #+#             */
/*   Updated: 2023/04/27 11:55:52 by lsalin           ###   ########.fr       */
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

	// Utilisation du "ET logique" (&) pour tester si le bit en question est activé ou non
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
	if (getTypePath(index) == 1 && checkFile(index, 4) == 0) // Mode 4 = lecture
		return (0);

	if (getTypePath(path + index) == 1 && checkFile(path + index, 4) == 0)
		return (0);

	return (-1);
}

// Lit le contenu d'un fichier specifie par le path
// Renvoie le contenu du fichier : vide si path vide/echec ouverture/autre erreur

std::string	ConfigFile::readFile(std::string path)
{
	if (path.empty() || path.length() == 0)
		return (NULL);

	std::ifstream	config_file(path.c_str());

	if (!config_file || !config_file.is_open())
		return (NULL);

	std::stringstream	stream_binding;
	stream_binding << config_file.rdbuf();

	return (stream_binding.str());
}

std::string	ConfigFile::getPath()
{
	return (this->_path);
}

int	ConfigFile::getSize()
{
	return (this->_size);
}