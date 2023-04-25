/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Webserv.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lsalin <lsalin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/08 15:16:42 by lsalin            #+#    #+#             */
/*   Updated: 2023/04/25 14:05:08 by lsalin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#ifndef WEBSERV_HPP
# define WEBSERV_HPP

# include <iostream>
// # include <fstream>
# include <fcntl.h>
# include <cstring>
# include <string> 
# include <unistd.h>
# include <dirent.h>
# include <sstream>
// # include <bits/stdc++.h>
# include <cstdlib>
# include <fstream>
# include <sstream>
# include <cctype>
# include <ctime>
# include <cstdarg>

/* STL Containers */
# include <map>
# include <set>
# include <vector>
# include <algorithm>
# include <iterator>
# include <list>

/* System */
# include <sys/types.h>
# include <sys/wait.h>
# include <sys/stat.h>
# include <sys/time.h>
# include <unistd.h>
// # include <machine/types.h>
# include <signal.h>

/* Network */
# include <sys/socket.h>
# include <netinet/in.h>
# include <sys/select.h>
# include <arpa/inet.h>

# include "ConfigParser.hpp"
# include "ConfigFile.hpp"
# include "ServerConfig.hpp"
# include "Location.hpp"
# include "HttpRequest.hpp"
# include "CgiHandler.hpp"
# include "Mime.hpp"
# include "Logger.hpp"

#define CONNECTION_TIMEOUT 60 // Nombre de secondes au bout desquelles le client se fait déco si aucunes données n'ont été envoyées
#ifdef TESTER
	#define MESSAGE_BUFFER 40000 
#else
	#define MESSAGE_BUFFER 40000
#endif

#define MAX_URI_LENGTH 4096
#define MAX_CONTENT_LENGTH 30000000

template <typename T>
std::string toString(const T val)
{
	std::stringstream stream;
	stream << val;
	return stream.str();
}

/* Utils.c */

std::string		statusCodeString(short);
std::string		getErrorPage(short);
int				buildHtmlIndex(std::string &, std::vector<uint8_t> &, size_t &);
int				ft_stoi(std::string str);
unsigned int	fromHexToDec(const std::string& nb);

#endif