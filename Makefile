# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: anmerten <anmerten@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2024/01/01 00:00:00 by student           #+#    #+#              #
#    Updated: 2026/03/11 16:45:33 by anmerten         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME		= ircserv
CXX			= c++
CXXFLAGS	= -Wall -Wextra -Werror -std=c++98
RM			= rm -f

# Directories
SRCDIR		= src
INCDIR		= include
OBJDIR		= obj

# Source files
SRCS		= $(SRCDIR)/main.cpp \
			  $(SRCDIR)/Server.cpp \
			  $(SRCDIR)/ServerUtils.cpp \
			  $(SRCDIR)/ServerCommands.cpp \
			  $(SRCDIR)/ServerOperatorCommands.cpp \
			  $(SRCDIR)/Client.cpp \
			  $(SRCDIR)/Channel.cpp

# Object files
OBJS		= $(SRCS:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

# Header files
INCS		= -I$(INCDIR)

# Colors
GREEN		= \033[0;32m
RED			= \033[0;31m
RESET		= \033[0m

# Rules
all: $(NAME)

$(NAME): $(OBJS)
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@echo "$(GREEN)✓ $(NAME) compiled successfully!$(RESET)"

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(OBJDIR)
	@$(CXX) $(CXXFLAGS) $(INCS) -c $< -o $@
	@echo "$(GREEN)✓ Compiled: $<$(RESET)"

clean:
	@$(RM) -r $(OBJDIR)
	@echo "$(RED)✗ Object files removed$(RESET)"

fclean: clean
	@$(RM) $(NAME)
	@echo "$(RED)✗ $(NAME) removed$(RESET)"

re: fclean all

.PHONY: all clean fclean re
