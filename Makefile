NAME =	webserv

SRCS =	srcs/main.cpp \
		srcs/Utils.cpp \
		srcs/ServerManager.cpp \
		srcs/Response.cpp \
		srcs/Client.cpp \
		srcs/HttpRequest.cpp \
		srcs/ConfigFile.cpp \
		srcs/ConfigParser.cpp \
		srcs/ServerConfig.cpp \
		srcs/Location.cpp \
		srcs/CgiHandler.cpp \
		srcs/Mime.cpp \
		srcs/Logger.cpp

HEADERS	= includes/Webserv.hpp

OBJS = $(SRCS:.cpp=.o)

CXX = c++

CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -Iincludes
CXXFLAGS += -g3 

RM = rm -rf

all:
	@$(MAKE) $(NAME) -j5
$(NAME) : $(OBJS) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(NAME)

re: 	fclean all

.PHONY: all clean fclean re