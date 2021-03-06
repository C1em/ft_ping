# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: coremart <coremart@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2021/10/23 19:44:59 by coremart          #+#    #+#              #
#    Updated: 2021/10/26 16:03:13 by coremart         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

## OS ##
UNAME_S := $(shell uname -s)

##COMPILATION ##
NAME = ft_ping
ASANFLAGS = -fsanitize=address -fno-omit-frame-pointer -Wno-format-security \
			-fsanitize=undefined
CFLAGS = -g -Werror -Wall -Wextra -pedantic-errors

ifeq ($(UNAME_S), Linux)
CFLAGS += -std=gnu99
endif
ifeq ($(UNAME_S),Darwin)
CFLAGS += -std=c99
endif

DFLAGS = -MT $@ -MMD -MP -MF $(DDIR)/$*.d
AFLAGS =
ASAN =

## INCLUDES ##
LIB = getopt
LIBH = $(LIB)/include
HDIR = include
LIBA = $(LIB)/getopt.a

## SOURCES ##
SDIR = src
_SRCS = ft_ping.c
SRCS = $(patsubst %,$(SDIR)/%,$(_SRCS))

## OBJECTS ##
ODIR = obj
_OBJS = $(_SRCS:.c=.o)
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

## DEPENDENCIES ##
DDIR = dep
_DEPS = $(_OBJS:.o=.d)
DEPS = $(patsubst %,$(DDIR)/%,$(_DEPS))

### RULES ###

all: $(NAME)

$(NAME): $(OBJS)
	@if [ "$(AFLAGS)" = "" ];\
	then\
		make -j 8 -C $(LIB);\
	else\
		make -j 8 -C $(LIB) asan;\
	fi
	${CC} -o $(NAME) $(LIBA) $(OBJS) $(CFLAGS) $(AFLAGS)
	@if [ "$(UNAME_S)" = "Linux" ];\
	then\
		sudo setcap cap_net_raw+ep $(NAME);\
	fi

$(ODIR)/%.o: $(SDIR)/%.c
	${CC} $(CFLAGS) $(DFLAGS) -o $@ -c $< -I $(HDIR) $(AFLAGS)

-include $(DEPS)

clean:
	@make -j 8 -C $(LIB) clean
	@rm -f $(OBJS)

fclean: clean
	@make -j 8 -C $(LIB) fclean
	@rm -f $(NAME)
	@rm -rf $(NAME).dSYM

re: fclean all

asan: AFLAGS = $(ASANFLAGS)
asan: all

reasan: AFLAGS = $(ASANFLAGS)
reasan: re

.PRECIOUS: $(DDIR)/%.d
.PHONY: all clean fclean re $(NAME)
