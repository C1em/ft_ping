# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: coremart <coremart@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2021/10/20 15:16:13 by coremart          #+#    #+#              #
#    Updated: 2021/10/20 15:19:40 by coremart         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

## COMPILATION ##
NAME = getopt.a
ASANFLAGS = -fsanitize=address -fno-omit-frame-pointer -Wno-format-security -fsanitize=undefined
CFLAGS = -g -Wall -Wextra -Werror -pedantic-errors -std=c99
DFLAGS = -MT $@ -MMD -MP -MF $(DDIR)/$*.d
ASAN =

## INCLUDE ##
HDIR = include

## SOURCES ##
SDIR = src
_SRCS = getopt.c
SRCS = $(patsubst %,$(SDIR)/%,$(_SRCS))

## OBJECTS ##
ODIR = obj
_OBJS = $(_SRCS:.c=.o)
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

## DEPENDENCIES ##
DDIR = dep
_DEPS = $(_OBJS:.o=.d)
DEPS = $(patsubst %,$(DDIR)/%,$(_DEPS))

## RULES ##

all: $(NAME)

$(NAME): $(OBJS)
	ar rcs $(NAME) $(OBJS)

$(ODIR)/%.o: $(SDIR)/%.c
	gcc $(CFLAGS) $(DFLAGS) -o $@ -c $< -I $(HDIR) $(ASAN)

-include $(DEPS)

clean:
	@rm -f $(OBJS)

fclean: clean
	@rm -f $(NAME)

re: fclean all

asan: ASAN = $(ASANFLAGS)
asan: all

reasan: ASAN = $(ASANFLAGS)
reasan: re

.PRECIOUS: $(DDIR)/%.d
.PHONY: all clean fclean re $(NAME)
