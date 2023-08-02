# Webserv

🇫🇷

Le projet Webserv consiste à écrire un serveur HTTP en C++98, qui sera capable de communiquer avec un navigateur web et d'effectuer plusieurs tâches courantes d'un serveur web, comme l'envoi de pages web aux clients, la prise en charge des requêtes `GET`, `POST` et `DELETE`, la possibilité d'écouter sur plusieurs ports, la gestion des fichiers de configuration, etc.

Ce projet est très formateur car il permet de comprendre en profondeur comment fonctionne un serveur web et comment les données sont échangées entre un client et un serveur.

De nombreuses notions importantes de programmation seront étudiées à travers ce projet comme les __sockets__, la __programmation multiplexée__ via l'utilisation de `select()`, ou encore l'implémentation d'un __CGI__.

__Comment tester le programme__ ?

* Exécuter la commande make à la racine du répertoire.
* Exécutez `./webserv`. Votre serveur est maintenant prêt à accepter des connexions de la part de `http://localhost:8002`.
* Tapez `http://localhost:8002` dans votre navigateur et visitez notre site web !
* Gardez un œil sur le terminal pour observer les requêtes entre le client et le serveur.

🇺🇸

The Webserv project consists of writing an HTTP server in C++98, which will be able to communicate with a web browser and perform several common tasks of a web server, such as sending web pages to clients, supports `GET`, `POST` and `DELETE` requests, ability to listen on multiple ports, management of configuration files, etc.

This project is very educational because it allows you to understand in depth how a web server works and how data is exchanged between a client and a server.

Many important notions of programming will be studied through this project such as __sockets__, __multiplexed programming__ via the use of `select()`, or the implementation of a __CGI__.

__How to test the program__?

* Execute the `make` command at the root of the directory.
* Run `./webserv`. Your server is now ready to accept connections from `http://localhost:8002`.
* Type `http://localhost:8002` in your browser and visit our website!
* Keep an eye on the terminal to observe requests between client and server.

![Rating](rating.png)
