/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lsalin <lsalin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/15 12:06:51 by lsalin            #+#    #+#             */
/*   Updated: 2023/04/27 11:49:02 by lsalin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Webserv.hpp"
#include "ServerManager.hpp"

// ne fait rien mais est nécessaire pour éviter que le programme ne s'arrête
// si SIGPIPE reçu
void    sigpipeHandle(int sig)
{
	if (sig) {}
}

int	main(int argc, char **argv) 
{
	// Logger::setState(OFF);
	if (argc == 1 || argc == 2)
	{
		try 
		{
			std::string		config;
			ConfigParser	cluster;
			ServerManager 	master;
			
			signal(SIGPIPE, sigpipeHandle);

			// si 1 seul argument --> config = configs/default.conf"
			// sinon fichier précisé
			config = (argc == 1 ? "configs/default.conf" : argv[1]);
			
			cluster.createCluster(config);
			// cluster.print(); // (pour checker)
			master.setupServers(cluster.getServers());
			master.runServers();
		}
		catch (std::exception &e) {
			std::cerr << e.what() << std::endl;
			return (1);
		}
	}
	
	else 
	{
		Logger::logMsg(RED, CONSOLE_OUTPUT, "Error: wrong arguments");
		return (1);
	}

	return (0);
}