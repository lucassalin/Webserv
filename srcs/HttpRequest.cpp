/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lsalin <lsalin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/05 16:49:48 by lsalin            #+#    #+#             */
/*   Updated: 2023/04/11 11:36:29 by lsalin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpRequest.hpp"

HttpRequest::HttpRequest()
{
	// "::" pour accéder au champs de l'enum

	_method_str[::GET] = "GET";
	_method_str[::POST] = "POST";
	_method_str[::DELETE] = "DELETE";
	_method_str[::PUT] = "PUT";
	_method_str[::HEAD] = "HEAD";

	_path = "";
	_query = "";
	_fragment = "";
	_body_str = "";
	
	_body_length = 0;
	_storage = "";
	_key_storage = "";
	_boundary = "";

	_error_code = 0;
	_chunk_length = 0;
	_method = NONE;
	_method_index = 1;
	_state = Request_Line;

	_fields_done_flag = false;
	_body_flag = false;
	_body_done_flag = false;
	_chunked_flag = false;
	_multiform_flag = false;
}

HttpRequest::~HttpRequest() {}

/**
 @brief Check si faille "path Traversal" 
 		Caractères autorisés selon la RFC:
 
 		- Alphanumeriques	: A-Z a-z 0-9
 		- Non réservés		: - _ . ~
 		- Réservés			: * ' ( ) ; : @ & = + $ , / ? % # [ ]

 @return true si oui (le path contient des ":"), sinon false
 */

bool	checkUriPos(std::string path)
{
	std::string	tmp(path);
	char		*res = strtok((char*)tmp.c_str(), "/");
	int			pos = 0;

	while (res != NULL)
	{
		if (!strcmp(res, ".."))
			pos--;
		else
			pos++;
		if (pos < 0)
			return (1);
		res = strtok(NULL, "/");
	}
	return (0);
}

/**
 @brief Check si les caractères dans l'URI sont autorises, 
 		Caractères autorisés selon la RFC:
 
 		- Alphanumeriques	: A-Z a-z 0-9
 		- Non réservés		: - _ . ~
 		- Réservés			: * ' ( ) ; : @ & = + $ , / ? % # [ ]

 @return true si le caractère est autorisé, sinon false
 */

bool	allowedCharURI(uint8_t ch)
{
	if ((ch >= '#' && ch <= ';') || (ch >= '?' && ch <= '[') || (ch >= 'a' && ch <= 'z') ||
	   ch == '!' || ch == '=' || ch == ']' || ch == '_' || ch == '~')
		return (true);
	return (false);
}

/**
 @brief Check si les caractères d'un header field sont autorises, 
 		Caractères autorisés selon la RFC:
 
 		"!" / "#" / "$" / "%" / "&" / "'"
		"*" / "+" / "-" / "." / "^" / "_"
		"`" / "|" / "~" / 0-9 / A-Z / a-z

 @return true si le caractère est autorisé, sinon false
 */

bool	isToken(uint8_t ch)
{
	if (ch == '!' || (ch >= '#' && ch <= '\'') || ch == '*'|| ch == '+' || ch == '-'  || ch == '.' ||
	   (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z') || (ch >= '^' && ch <= '`') ||
	   (ch >= 'a' && ch <= 'z') || ch == '|')
		return (true);
	return (false);
}

// Supprime les espaces en debut et fin de string
void	trimStr(std::string &str)
{
	static const char	*spaces = " \t";

	str.erase(0, str.find_first_not_of(spaces));
	str.erase(str.find_last_not_of(spaces) + 1);
}

// Convertit en minuscule
void	toLower(std::string &str)
{
	for (size_t i = 0; i < str.length(); ++i)
		str[i] = std::tolower(str[i]);
}

// Parse les datas HTTP recues par le serveur
// Est call a chaque fois qu'un nouveau morceau de datas est recu
// Et est responsable de m.a.j l'etat du parseur des headers

void	HttpRequest::feed(char *data, size_t size)
{
	u_int8_t					character;
	static std::stringstream	s;

	for (size_t i = 0; i < size; ++i)
	{
		character = data[i];

		// Execute un certain bloc de code en fonction du code d'etat
		switch (_state)
		{

		// GET /hello.txt HTTP/1.1

		case Request_Line:
		{
			if (character == 'G')
				_method = GET;
			
			// "P" = PUT ou POST
			else if (character == 'P')
			{
				_state = Request_Line_Post_Put;
				break;
			}

			else if (character == 'D')
				_method = DELETE;

			else if (character == 'H')
				_method = HEAD;

			// Sinon erreur 501 ("Not implemented")
			else
			{
				_error_code = 501;
				std::cout << "Method Error Request_Line and Character is = " << character << std::endl;
				return;
			}
			
			// Si la methode est correctement reconnue
			// On passe au traitement de la 2e partie
			
			_state = Request_Line_Method;
			break;
		}

		// POST ou PUT

		case Request_Line_Post_Put:
		{
			if (character == 'O')
				_method = POST;

			else if (character == 'U')
				_method = PUT;

			else
			{
				_error_code = 501;
				std::cout << "Method Error Request_Line and Character is = " << character << std::endl;
				return;
			}

			_method_index++;
			_state = Request_Line_Method;

			break;
		}

		case Request_Line_Method:
		{
			if (character == _method_str[_method][_method_index])
				_method_index++;

			else
			{
				_error_code = 501;
				std::cout << "Method Error Request_Line and Character is = " << character << std::endl;
				return;
			}

			// Check si le nombre de caracteres de la methode correspond
			// a ce qui est attendu
			// Espace apres le nom de methode donc on passe a cet etat

			if ((size_t)_method_index == _method_str[_method].length())
				_state = Request_Line_First_Space;
			break;
		}

		case Request_Line_First_Space:
		{
			if (character != ' ')
			{
				_error_code = 400;
				std::cout << "Bad Character (Request_Line_First_Space)" << std::endl;
				return;
			}

			_state = Request_Line_URI_Path_Slash;
			continue;
		}

		// GET /hello.txt HTTP/1.1

		case Request_Line_URI_Path_Slash:
		{
			if (character == '/')
			{
				_state = Request_Line_URI_Path;
				_storage.clear();
			}
			else
			{
				_error_code = 400;
				std::cout << "Bad Character (Request_Line_URI_Path_Slash)" << std::endl;
				return;
			}
			break;
		}

		// https://www.example.com/search?q=example#results

		case Request_Line_URI_Path:
		{
			if (character == ' ')
			{
				_state = Request_Line_Ver;
				_path.append(_storage);
				_storage.clear();
				continue;
			}

			// Query permettent d'add des parametres supplementaires (optionnel)
			// L'element est separe par sa valeur avec un "="

			else if (character == '?')
			{
				_state = Request_Line_URI_Query;
				_path.append(_storage);
				_storage.clear();
				continue;
			}

			// Fragment permettent de pointer vers 
			// une partie specifique de la ressource

			else if (character == '#')
			{
				_state = Request_Line_URI_Fragment;
				_path.append(_storage);
				_storage.clear();
				continue;
			}

			else if (!allowedCharURI(character))
			{
				_error_code = 400;
				std::cout << "Bad Character (Request_Line_URI_Path)" << std::endl;
				return;
			}

			else if (i > MAX_URI_LENGTH)
			{
				_error_code = 414;
				std::cout << "URI Too Long (Request_Line_URI_Path)" << std::endl;
				return;
			}

			break;
		}

		case Request_Line_URI_Query:
		{
			if (character == ' ')
			{
				_state = Request_Line_Ver;
				_query.append(_storage);
				_storage.clear();
				continue;
			}

			else if (character == '#')
			{
				_state = Request_Line_URI_Fragment;
				_query.append(_storage);
				_storage.clear();
				continue;
			}

			else if (!allowedCharURI(character))
			{
				_error_code = 400;
				std::cout << "Bad Character (Request_Line_URI_Query)" << std::endl;
				return;
			}

			else if (i > MAX_URI_LENGTH)
			{
				_error_code = 414;
				std::cout << "URI Too Long (Request_Line_URI_Path)" << std::endl;
				return;
			}

			break;
		}

		case Request_Line_URI_Fragment:
		{
			if (character == ' ')
			{
				_state = Request_Line_Ver;
				_fragment.append(_storage);
				_storage.clear();
				continue;
			}

			else if (!allowedCharURI(character))
			{
				_error_code = 400;
				std::cout << "Bad Character (Request_Line_URI_Fragment)" << std::endl;
				return;
			}

			else if (i > MAX_URI_LENGTH)
			{
				_error_code = 414;
				std::cout << "URI Too Long (Request_Line_URI_Path)" << std::endl;
				return;
			}

			break;
		}

		// HTTP/1.1

		case Request_Line_Ver:
		{
			if (checkUriPos(_path))
			{
				_error_code = 400;
				std::cout << "Request URI ERROR: goes before root !!" << std::endl;
				return;
			}

			if (character != 'H')
			{
				_error_code = 400;
				std::cout << "Bad Character (Request_Line_Ver)" << std::endl;
				return;
			}

			_state = Request_Line_HT;
			break;
		}

		case Request_Line_HT:
		{
			if (character != 'T')
			{
				_error_code = 400;
				std::cout << "Bad Character (Request_Line_HT)" << std::endl;
				return;
			}

			_state = Request_Line_HTT;
			break;
		}

		case Request_Line_HTT:
		{
			if (character != 'T')
			{
				_error_code = 400;
				std::cout << "Bad Character (Request_Line_HTT)" << std::endl;
				return;
			}

			_state = Request_Line_HTTP;
			break;
		}

		case Request_Line_HTTP:
		{
			if (character != 'P')
			{
				_error_code = 400;
				std::cout << "Bad Character (Request_Line_HTTP)" << std::endl;
				return;
			}

			_state = Request_Line_HTTP_Slash;
			break;
		}

		// HTTP/1.1

		case Request_Line_HTTP_Slash:
		{
			if (character != '/')
			{
				_error_code = 400;
				std::cout << "Bad Character (Request_Line_HTTP_Slash)" << std::endl;
				return;
			}

			_state = Request_Line_Major;
			break;
		}

		case Request_Line_Major:
		{
			if (!isdigit(character))
			{
				_error_code = 400;
				std::cout << "Bad Character (Request_Line_Major)" << std::endl;
				return;
			}

			_ver_major = character;
			_state = Request_Line_Dot;

			break;
		}

		case Request_Line_Dot:
		{
			if (character != '.')
			{
				_error_code = 400;
				std::cout << "Bad Character (Request_Line_Dot)" << std::endl;
				return;
			}

			_state = Request_Line_Minor;
			break;
		}

		case Request_Line_Minor:
		{
			if (!isdigit(character))
			{
				_error_code = 400;
				std::cout << "Bad Character (Request_Line_Minor)" << std::endl;
				return;
			}

			_ver_minor = character;
			_state = Request_Line_CR;

			break;
		}

		case Request_Line_CR:
		{
			if (character != '\r')
			{
				_error_code = 400;
				std::cout << "Bad Character (Request_Line_CR)" << std::endl;
				return;
			}

			_state = Request_Line_LF;
			break;
		}

		case Request_Line_LF:
		{
			if (character != '\n')
			{
				_error_code = 400;
				std::cout << "Bad Character (Request_Line_LF)" << std::endl;
				return;
			}

			_state = Field_Name_Start;
			_storage.clear();

			continue;
		}

		// Host: localhost:8080
		// User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)

		case Field_Name_Start:
		{
			if (character == '\r')
				_state = Fields_End;

			else if (isToken(character))
				_state = Field_Name;

			else
			{
				_error_code = 400;
				std::cout << "Bad Character (Field_Name_Start)" << std::endl;
				return;
			}

			break;
		}

		case Fields_End:
		{
			// Fin des headers

			if (character == '\n')
			{
				_storage.clear();
				_fields_done_flag = true;
				_handle_headers();

				// Si un body est present --> chunk ?

				if (_body_flag == 1)
				{
					if (_chunked_flag == true)
						_state = Chunked_Length_Begin;

					else
					{
						_state = Message_Body;
					}
				}

				// Si pas de body --> parsing completed

				else
				{
					_state = Parsing_Done;
				}

				continue;
			}

			else
			{
				_error_code = 400;
				std::cout << "Bad Character (Fields_End)" << std::endl;
				return;
			}

			break;
		}

		// Host: localhost:8080

		case Field_Name:
		{
			if (character == ':')
			{
				_key_storage = _storage;
				_storage.clear();
				_state = Field_Value;
				continue;
			}
			else if (!isToken(character))
			{
				_error_code = 400;
				std::cout << "Bad Character (Field_Name)" << std::endl;
				return;
			}
			break;
			// if (!character allowed)
			//  error;
		}

		case Field_Value:
		{
			if (character == '\r')
			{
				setHeader(_key_storage, _storage);
				_key_storage.clear();
				_storage.clear();
				_state = Field_Value_End;
				continue;
			}
			break;
		}

		// '\r' est suivi de '\n' mais on les traite
		// dans deux case différents

		case Field_Value_End:
		{
			if (character == '\n')
			{
				_state = Field_Name_Start;
				continue;
			}
			else
			{
				_error_code = 400;
				std::cout << "Bad Character (Field_Value_End)" << std::endl;
				return;
			}
			break;
		}

		// Host: www.example.com
		// Transfer-Encoding: chunked

		// 25
		// This is the data in the first chunk
			
		// 1C
		// and this is the second one
		// 0

		case Chunked_Length_Begin:
		{
			if (isxdigit(character) == 0)
			{
				_error_code = 400;
				std::cout << "Bad Character (Chunked_Length_Begin)" << std::endl;
				return;
			}
			
			s.str(""); // on vide le contenu du flux de s pour s'assurer qu'il ne contient pas de précédentes valeurs
			s.clear(); // clear() pour reinit les indicateurs d'erreurs
			s << character; // on add le caractère lu depuis la requête à notre string
			
			// s >> std::hex configure la stringstream pour qu'elle traite 
			// le texte suivant comme un nombre hexa
			// s >> _chunk_length extrait l'hexa de la stringstream et le stocke dans _chunk_length

			s >> std::hex >> _chunk_length; 

			if (_chunk_length == 0)
				_state = Chunked_Length_CR;
			else
				_state = Chunked_Length; // on continue à lire

			continue;
		}

		// La longueur du chunk est encodée en hexa, donc chaque caractère
		// est encodé en hexa + rajouté au total de la longueur du chunk

		case Chunked_Length:
		{
			if (isxdigit(character) != 0)
			{
				int temp_len = 0;
				s.str("");
				s.clear();
				s << character;
				s >> std::hex >> temp_len;
				_chunk_length *= 16; // on x16 le nombre hexa pour faire de la place aux bits de poids faible de temp_len
				_chunk_length += temp_len;
			}

			else if (character == '\r')
				_state = Chunked_Length_LF;

			// Le caractère n'est ni un hexa ni CR --> fin du chunk
			// le prochain caractère est le \n suivant
			// --> on ignore tous les chars jusqu'à ce \n

			else
				_state = Chunked_Ignore;

			continue;
		}

		case Chunked_Length_CR:
		{
			if (character == '\r')
				_state = Chunked_Length_LF;

			else
			{
				_error_code = 400;
				std::cout << "Bad Character (Chunked_Length_CR)" << std::endl;
				return;
			}

			continue;
		}

		case Chunked_Length_LF:
		{
			if (character == '\n')
			{
				if (_chunk_length == 0)
					_state = Chunked_End_CR;
				else
					_state = Chunked_Data;
			}

			else
			{
				_error_code = 400;
				std::cout << "Bad Character (Chunked_Length_LF)" << std::endl;
				return;
			}

			continue;
		}

		case Chunked_Ignore:
		{
			if (character == '\r')
				_state = Chunked_Length_LF;
			continue;
		}

		// On ajoute le contenu de chaque chunk au body jusqu'à la fin du chunk
		// Décremente _chunk_length jusqu'à la fin du chunk (== 0)

		case Chunked_Data:
		{
			_body.push_back(character);
			--_chunk_length;
			
			if (_chunk_length == 0)
				_state = Chunked_Data_CR;

			continue;
		}

		case Chunked_Data_CR:
		{
			if (character == '\r')
				_state = Chunked_Data_LF;

			else
			{
				_error_code = 400;
				std::cout << "Bad Character (Chunked_Data_CR)" << std::endl;
				return;
			}

			continue;
		}

		case Chunked_Data_LF:
		{
			if (character == '\n')
				_state = Chunked_Length_Begin;

			else
			{
				_error_code = 400;
				std::cout << "Bad Character (Chunked_Data_LF)" << std::endl;
				return;
			}
			continue;
		}

		case Chunked_End_CR:
		{
			if (character != '\r')
			{
				_error_code = 400;
				std::cout << "Bad Character (Chunked_End_CR)" << std::endl;
				return;
			}
			
			_state = Chunked_End_LF;
			continue;
		}

		case Chunked_End_LF:
		{
			if (character != '\n')
			{
				_error_code = 400;
				std::cout << "Bad Character (Chunked_End_LF)" << std::endl;
				return;
			}
			
			_body_done_flag = true;
			_state = Parsing_Done;

			continue;
		}

		// Cas où pas de chunk, donc direct body

		case Message_Body:
		{
			// Si la taille du body est < taille totale du body
			// C'est qu'on doit add le caractère actuellement lu au body

			if (_body.size() < _body_length)
				_body.push_back(character);

			if (_body.size() == _body_length)
			{
				_body_done_flag = true;
				_state = Parsing_Done;
			}
			break;
		}

		case Parsing_Done:
		{
			return;
		}

		}
		_storage += character;
	}

	// Si parsing terminé, on add le contenu du body à body_str
	// Permet de la traiter + facilement par la suite

	if (_state == Parsing_Done)
	{
		_body_str.append((char *)_body.data(), _body.size());
	}
}

bool	HttpRequest::parsingCompleted()
{
	return (_state == Parsing_Done);
}

HttpMethod	&HttpRequest::getMethod()
{
	return (_method);
}

std::string &HttpRequest::getPath()
{
	return (_path);
}

std::string	&HttpRequest::getQuery()
{
	return (_query);
}

std::string	&HttpRequest::getFragment()
{
	return (_fragment);
}

std::string	HttpRequest::getHeader(std::string const &name)
{
	return (_request_headers[name]);
}

const std::map<std::string, std::string>	&HttpRequest::getHeaders() const
{
	return (this->_request_headers);
}

std::string	HttpRequest::getMethodStr()
{
	return (_method_str[_method]);
}

std::string	&HttpRequest::getBody()
{
	return (_body_str);
}

std::string     HttpRequest::getServerName()
{
	return (this->_server_name);
}

bool	HttpRequest::getMultiformFlag()
{
	return (this->_multiform_flag);
}

std::string	&HttpRequest::getBoundary()
{
	return (this->_boundary);
}

// Remplace le contenu de body_str par le body actuel

void	HttpRequest::setBody(std::string body)
{
	_body.clear();
	_body.insert(_body.begin(), body.begin(), body.end());
	_body_str = body;
}

void	HttpRequest::setMethod(HttpMethod & method)
{
	_method = method;
}

void	HttpRequest::setHeader(std::string &name, std::string &value)
{
	trimStr(value);
	toLower(name);
	_request_headers[name] = value;
}

void	HttpRequest::setMaxBodySize(size_t size)
{
	_max_body_size = size;
}

// Affiche le contenu complet de la requête

void	HttpRequest::printMessage()
{
	std::cout << _method_str[_method] + " " + _path + "?" + _query + "#" + _fragment
			  + " " + "HTTP/" << _ver_major  << "." << _ver_minor << std::endl;

	// User-Agent:Mozilla/5.0
	for (std::map<std::string, std::string>::iterator it = _request_headers.begin(); it != _request_headers.end(); ++it)
	{
		std::cout << it->first + ":" + it->second << std::endl;
	}

	// [72, 101, 108, 108, 111] --> "hello"
	for (std::vector<u_int8_t>::iterator it = _body.begin(); it != _body.end(); ++it)
	{
		std::cout << *it;
	}

	std::cout << std::endl << "END OF BODY" << std::endl;
	std::cout << "BODY FLAG =" << _body_flag << "  _BOD_done_flag= " << _body_done_flag << "FEIDLS FLAG = " << _fields_done_flag
			  																								<< std::endl;
}

// Parcourt les headers de la requête pour extraire les choses importantes

void	HttpRequest::_handle_headers()
{
	std::stringstream ss;

	// Si Content-Length --> body forcément
	if (_request_headers.count("content-length"))
	{
		_body_flag = true;
		// Extrait "content-length" du header et la stocke dans le flux
		// pour ensuite la convertir en entier et la stocker dans _body_length
		ss << _request_headers["content-length"];
		ss >> _body_length;
	}

	if ( _request_headers.count("transfer-encoding"))
	{
		if (_request_headers["transfer-encoding"].find_first_of("chunked") != std::string::npos)
			_chunked_flag = true;
		_body_flag = true;
	}

	// Host: www.example.com

	if (_request_headers.count("host"))
	{
		size_t pos = _request_headers["host"].find_first_of(':');
		_server_name = _request_headers["host"].substr(0, pos);
	}

	// Content-Type: multipart/form-data; boundary=---------------------------12345abcdef

	if (_request_headers.count("content-type")
		&& _request_headers["content-type"].find("multipart/form-data") != std::string::npos)
	{
		size_t pos = _request_headers["content-type"].find("boundary=", 0);
		
		if (pos != std::string::npos)
			this->_boundary = _request_headers["content-type"].substr(pos + 9, _request_headers["content-type"].size());
		
		this->_multiform_flag = true;
	}
}

short	HttpRequest::errorCode()
{
	return (this->_error_code);
}

// Réinitialise les variables pour pouvoir recevoir une nouvelle requête

void HttpRequest::clear()
{
	_path.clear();
	_error_code = 0;
	_query.clear();
	_fragment.clear();
	_method = NONE;
	_method_index = 1;
	_state = Request_Line;
	_body_length = 0;
	_chunk_length = 0x0;
	_storage.clear();
	_body_str = "";
	_key_storage.clear();
	_request_headers.clear();
	_server_name.clear();
	_body.clear();
	_boundary.clear();
	_fields_done_flag = false;
	_body_flag = false;
	_body_done_flag = false;
	_complete_flag = false;
	_chunked_flag = false;
	_multiform_flag = false;
}

// --------------------------------------------------------------------------------------------------//

/**
 @brief Vérifie si le header "Connection" est present dans la requete, et si sa valeur est "close"
		Si c'est le cas, la connexion doit etre fermee apres la reponse
		
 @example Connection: keep-alive
		  Connection: close

 @return false si la connexion doit etre fermee, sinon true pour la maintenir
*/

bool	HttpRequest::keepAlive()
{
	if (_request_headers.count("connection"))
	{
		if (_request_headers["connection"].find("close", 0) != std::string::npos)
			return (false);
	}
	return (true);
}

// Coupe une partie du body en fonction du nombre de bytes passes en parametre
// Est utile si on ne veut pas envoyer la requete entiere
// Par exemple une grande requete qu'on veut envoyer en plusieurs parties

void	HttpRequest::cutReqBody(int bytes)
{
	_body_str = _body_str.substr(bytes);
}
