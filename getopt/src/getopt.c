/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   getopt.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/10/05 16:29:11 by coremart          #+#    #+#             */
/*   Updated: 2021/10/24 19:36:08 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>

char *optarg = NULL;
int optind = 1;
int optopt = '?';
int opterr = 1;

static int			_getopt_strcmp(char *s1, char *s2) {

	int i = 0;

	while (s1[i] == s2[i] && s1[i])
		i++;

	return ((unsigned char)s1[i] - (unsigned char)s2[i]);
}

static inline bool	is_nonopt(char **argv, int index) {

	return (argv[index][0] != '-' || argv[index][1] == '\0');
}

char				*my_index(const char *str, int chr) {

	while (*str) {

		if (*str == chr)
			return ((char*)str);
		str++;
	}
	return (NULL);
}

void				exchange(char **argv, int *first_nonopt, int *last_nonopt) {

	int bottom = *first_nonopt;
	int middle = *last_nonopt;
	int top = optind;
	char *tem;

	// Exchange the shorter segment with the far end of the longer segment.
	// That puts the shorter segment into the right place.
	// It leaves the longer segment in the right place overall,
	// but it consists of two parts that need to be swapped next.

	while (top > middle && middle > bottom) {

		if (top - middle > middle - bottom) {

			// Bottom segment is the short one
			int len = middle - bottom;

			// Swap it with the top part of the top segment
			for (int i = 0; i < len; i++) {

				tem = argv[bottom + i];
				argv[bottom + i] = argv[top - (middle - bottom) + i];
				argv[top - (middle - bottom) + i] = tem;
			}
			// Exclude the moved bottom segment from further swapping
			top -= len;
		}
		else {

			// Top segment is the short one
			int len = top - middle;

			// Swap it with the bottom part of the bottom segment
			for (int i = 0; i < len; i++) {

				tem = argv[bottom + i];
				argv[bottom + i] = argv[middle + i];
				argv[middle + i] = tem;
			}
			// Exclude the moved top segment from further swapping
			bottom += len;
		}
	}

	// Update records for the slots the non-options now occupy
	*first_nonopt += (optind - *last_nonopt);
	*last_nonopt = optind;
}

int			getopt(int argc, char *const argv[], const char *optstring) {

	static char *nextchar = NULL;
	static int first_nonopt;
	static int last_nonopt;
	static int __getopt_initialized = 0;

	optarg = NULL;

	// init
	if (optind == 0 || !__getopt_initialized) {

		if (optind == 0)
			optind = 1;	// Don't scan ARGV[0], the program name.
		first_nonopt = last_nonopt = optind;
		__getopt_initialized = 1;
	}

	if (nextchar == NULL || *nextchar == '\0') {

		// if non option have been skipped exchange
		// last_nonopt != optind means if it's not the first time we iter or parsing is over
		if (first_nonopt != last_nonopt && last_nonopt != optind)
			exchange((char**)argv, &first_nonopt, &last_nonopt);
		else if (last_nonopt != optind)
			first_nonopt = optind;

		while (optind < argc && is_nonopt((char**)argv, optind))
			optind++;
		last_nonopt = optind;


		// '--' stop parsing
		if (optind != argc && _getopt_strcmp(argv[optind], "--") == 0) {

			optind++;

			// exchange treating '--' as a option
			if (first_nonopt != last_nonopt && last_nonopt != optind)
				exchange((char**)argv, &first_nonopt, &last_nonopt);
			else if (first_nonopt == last_nonopt)
				first_nonopt = optind;

			last_nonopt = argc;
			optind = argc;
		}

		// if parsing is over
		if (optind == argc) {

			if (first_nonopt != last_nonopt)
				optind = first_nonopt;
			return (-1);
		}

		// skip initial '-'
		nextchar = argv[optind] + 1;
	}

	char c = *nextchar++;
	char *temp = my_index(optstring, c);

	if (*nextchar == '\0')
		optind++;

	if (temp == NULL || c == ':') {

		if (opterr)
			fprintf(stderr, "%s: invalid option -- %c\n", argv[0], c);

		optopt = c;
		return ('?');
	}

	// handle option argument
	if (temp[1] == ':') {

		if (temp[2] == ':') {

			// This is an option that accepts an argument optionally
			if (*nextchar != '\0') {

				optarg = nextchar;
				optind++;
			}
			else
				optarg = NULL;

			nextchar = NULL;
		}
		else {

			// This is an option that requires an argument
			if (*nextchar != '\0') {

				optarg = nextchar;
				optind++;
			}
			else if (optind == argc) {

				if (opterr)
					fprintf(stderr, "%s: option requires an argument -- %c\n", argv[0], c);

				optopt = c;
				if (optstring[0] == ':')
					c = ':';
				else
					c = '?';
			}
			else
				optarg = argv[optind++];

			nextchar = NULL;
		}
	}

	return (c);
}
