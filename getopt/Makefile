# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: coremart <coremart@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2021/10/20 15:16:13 by coremart          #+#    #+#              #
#    Updated: 2021/10/26 14:32:09 by coremart         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

## OS ##
UNAME_S := $(shell uname -s)

## COMPILATION ##
NAME = getopt.a
ASANFLAGS = -fsanitize=address -fno-omit-frame-pointer -Wno-format-security -fsanitize=undefined
CFLAGS = -g -Wall -Wextra -Werror -pedantic-errors

ifeq ($(UNAME_S),Linux)
CFLAGS += -std=gnu99
endif
ifeq ($(UNAME_S),Darwin)
CFLAGS += -std=c99
endif

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
