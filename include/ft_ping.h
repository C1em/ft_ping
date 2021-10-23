/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_ping.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/08/31 12:46:55 by coremart          #+#    #+#             */
/*   Updated: 2021/10/23 19:53:42 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_PING_H
# define FT_PING_H

#ifdef __linux__
	#define IS_LINUX (1)
#else
	#define IS_LINUX (0)
#endif


// number of received sequence numbers we can keep track of
# define	MAX_DUP_CHK		(8 * 128)

// keep track of seq to check duplicate
char		rcvd_tbl[MAX_DUP_CHK / 8];

# define	A(bit)		rcvd_tbl[(bit) / 8] // identify byte in array
# define	B(bit)		(1 << ((bit) & 0x07)) // identify bit in byte
# define	SET(bit)	(A(bit) |= B(bit))
# define	TST(bit)	(A(bit) & B(bit))


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
	unsigned int		nrepeats;
	double				tsum;
	double				tsumsq;
	double				tmin;
	double				tmax;
	unsigned int		options;
	char				*hostname;
};

#endif
