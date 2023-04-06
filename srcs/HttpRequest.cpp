/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lsalin <lsalin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/05 16:49:48 by lsalin            #+#    #+#             */
/*   Updated: 2023/04/06 10:38:52 by lsalin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/HttpRequest.hpp"

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

//-------------------------------------------------------------------------------------//

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

		case Request_Line:
		{
			if (character == 'G')
				_method = GET;
			else if (character == 'P')
			{
				_state = Request_Line_Post_Put;
				break;
			}
			else if (character == 'D')
				_method = DELETE;
			else if (character == 'H')
				_method = HEAD;
			else
			{
				_error_code = 501;
				std::cout << "Method Error Request_Line and Character is = " << character << std::endl;
				return;
			}
			_state = Request_Line_Method;
			break;
		}

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
		case Request_Line_URI_Path:
		{
			if (character == ' ')
			{
				_state = Request_Line_Ver;
				_path.append(_storage);
				_storage.clear();
				continue;
			}
			else if (character == '?')
			{
				_state = Request_Line_URI_Query;
				_path.append(_storage);
				_storage.clear();
				continue;
			}
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
			if (character == '\n')
			{
				_storage.clear();
				_fields_done_flag = true;
				_handle_headers();
				// if no body then parsing is completed.
				if (_body_flag == 1)
				{
					if (_chunked_flag == true)
						_state = Chunked_Length_Begin;
					else
					{
						_state = Message_Body;
					}
				}
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
		case Chunked_Length_Begin:
		{
			if (isxdigit(character) == 0)
			{
				_error_code = 400;
				std::cout << "Bad Character (Chunked_Length_Begin)" << std::endl;
				return;
			}
			s.str("");
			s.clear();
			s << character;
			s >> std::hex >> _chunk_length;
			if (_chunk_length == 0)
				_state = Chunked_Length_CR;
			else
				_state = Chunked_Length;
			continue;
		}
		case Chunked_Length:
		{
			if (isxdigit(character) != 0)
			{
				int temp_len = 0;
				s.str("");
				s.clear();
				s << character;
				s >> std::hex >> temp_len;
				_chunk_length *= 16;
				_chunk_length += temp_len;
			}
			else if (character == '\r')
				_state = Chunked_Length_LF;
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
		case Message_Body:
		{
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
		} // end of swtich
		_storage += character;
	}
	if (_state == Parsing_Done)
	{
		_body_str.append((char *)_body.data(), _body.size());
	}
}