/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_ping.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/08/31 12:46:55 by coremart          #+#    #+#             */
/*   Updated: 2021/09/26 16:47:45 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_PING_H
# define FT_PING_H

#define DEST_IP				"127.0.0.1"

#ifdef __linux__
	#define IS_LINUX (1)
#else
	#define IS_LINUX (0)
#endif

struct tv32 {
	u_int32_t tv32_sec;
	u_int32_t tv32_usec;
};

# define PING_TIMEOUT_SEC 1

# define TV_LEN (sizeof(struct tv32))

# define IP_MINLEN 20

# define DATA_LEN (64 - ICMP_MINLEN)

# define F_VERBOSE 0x1

struct ping {

	struct sockaddr_in	dest_addr;
	struct tv32			last_ping;
	struct ip			*pkt;
	int					s; // socket
	unsigned int		ntransmitted;
	unsigned int		nreceived;
	unsigned int		nmissedmax;
	unsigned int		options;
	unsigned int		finish_up;
	char				hostname[16];
};

#endif
