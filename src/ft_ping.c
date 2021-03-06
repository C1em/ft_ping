/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_ping.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/08/12 18:38:26 by coremart          #+#    #+#             */
/*   Updated: 2021/10/26 17:13:34 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <sys/socket.h> // sendto() socket() CMSG_FIRSTHDR() CMSG_NXTHDR() AF_INET
#include <netinet/ip.h> // struct ip
#include <netinet/ip_icmp.h> // struct icmp
#include <arpa/inet.h> // inet_ntop() inet_ntoa()
#include <stdlib.h> // malloc() free() calloc()
#include <unistd.h> // getuid()
#include <errno.h>
#include <string.h>
#include <netinet/if_ether.h> // struct ether_header
#include <unistd.h> // getpid()
#include <stdio.h> // printf() fprintf()
#include <netdb.h> //  getaddrinfo()
#include <sys/time.h>
#include "ft_ping.h"
#include <signal.h> // signal() SIGINT SIGQUIT
#include <sysexits.h>
#include <stdbool.h>
#include "getopt.h"

volatile struct ping g_ping = {.tmin = 999999999.0};

static double		fabs(double x) {

	if (x < 0.0)
		return (-x);
	return (x);
}

double			ft_ping_sqrt(double x) {

	if (x < 0)
		return (-1.0);
	if (x == 0.0)
		return (0.0);

	long double lx = (long double)x;
	long double xi = x / 2;
	long double new_xi;

	while (fabs(xi * xi - lx) > 0.0000000000001) {

		new_xi = (xi + lx / xi) / 2;

		if (new_xi == xi) // reach max precision
			break ;

		xi = new_xi;
	}
	return (xi);
}

bool	is_space(char c) {

	if (
		c == '\t' ||
		c == '\n' ||
		c == '\r' ||
		c == '\v' ||
		c == '\f' ||
		c == ' '
	)
		return (true);
	return (false);
}

int		finish(void) {

	signal(SIGINT, SIG_IGN);
	signal(SIGALRM, SIG_IGN);
	(void)putchar('\n');
	(void)printf("--- %s ping statistics ---\n", g_ping.hostname);
	(void)printf("%u packets transmitted, ", g_ping.ntransmitted);
	(void)printf("%u packets received, ", g_ping.nreceived);
	if (g_ping.ntransmitted) {
		if (g_ping.nreceived > g_ping.ntransmitted)
			(void)printf("-- somebody's printing up packets!");
		else
			(void)printf(
				"%.1f%% packet loss",
				((g_ping.ntransmitted - g_ping.nreceived) * 100.0) /
				g_ping.ntransmitted
			);
	}
	(void)putchar('\n');
	if (g_ping.nreceived) {

		double n = g_ping.nreceived + g_ping.nrepeats;
		double avg = g_ping.tsum / n;
		double vari = g_ping.tsumsq / n - avg * avg;
		(void)printf(
			"round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n",
			g_ping.tmin,
			avg,
			g_ping.tmax,
			ft_ping_sqrt(vari)
			);
		return (0);
	}
	return (2);
}

void			stopit(int sig __attribute__((unused))) {

	exit(finish());
}

unsigned short	in_cksum(unsigned short *ptr, unsigned int len) {

	unsigned long checksum = 0;
	while (len > 1) {

		checksum += *ptr++;
		len -= sizeof(short);
	}

	//if any bytes left, pad the bytes and add
	if(len > 0)
		checksum += ((*ptr)&htons(0xFF00));

	//Fold checksum to 16 bits: add carrier to result
	while (checksum >> 16)
		checksum = (checksum & 0xffff) + (checksum >> 16);

	checksum = ~checksum;
	return ((unsigned short)checksum);
}

struct ip	*build_iphdr(struct ip* ip_hdr) {

	ip_hdr->ip_v = 4;
	ip_hdr->ip_hl = 5;
	ip_hdr->ip_tos = 64;
	ip_hdr->ip_ttl = 64;
	ip_hdr->ip_p = IPPROTO_ICMP;
	ip_hdr->ip_id = htons((unsigned short)getpid());

	ip_hdr->ip_off = 0;
	ip_hdr->ip_src.s_addr = INADDR_ANY;

	memcpy(
		&ip_hdr->ip_dst,
		(void*)&g_ping.dest_addr.sin_addr.s_addr,
		sizeof(g_ping.dest_addr.sin_addr.s_addr)
	);

	// on Linux ip_len is filled by the kernel
	if (!IS_LINUX)
		ip_hdr->ip_len = (unsigned short)(sizeof(struct ip) + ICMP_MINLEN + DATA_LEN);

	// done by kernel
	// ip_hdr->ip_sum = htons(in_cksum((unsigned short*)ip_hdr, 20));
	return (ip_hdr);
}

void		add_icmp_data(unsigned char *data) {

	for (int i = 0; i < DATA_LEN - (int)TV_LEN; i++) {

		// fill the data with incrementing numbers
		data[i] = (unsigned char)i % 256;
	}
}

struct ip	*build_icmphdr(struct ip* ip_hdr) {

	struct icmp	*icmp_hdr = (struct icmp*)(ip_hdr + 1);
	struct timeval now;
	struct tv32 tv32;

	icmp_hdr->icmp_type = ICMP_ECHO;
	icmp_hdr->icmp_code = 0;
	icmp_hdr->icmp_cksum = 0;

	icmp_hdr->icmp_hun.ih_idseq.icd_id = htons((unsigned short)getpid());
	icmp_hdr->icmp_hun.ih_idseq.icd_seq = htons(g_ping.ntransmitted);
	g_ping.ntransmitted++;

	(void)gettimeofday(&now, NULL);

	tv32.tv32_sec = htonl(now.tv_sec);
	tv32.tv32_usec = htonl(now.tv_usec);
	memcpy((void*)&((char*)icmp_hdr)[ICMP_MINLEN], (void*)&tv32, TV_LEN);

	add_icmp_data((unsigned char*)icmp_hdr->icmp_dun.id_data + TV_LEN);

	icmp_hdr->icmp_cksum = in_cksum((unsigned short*)icmp_hdr, ICMP_MINLEN + DATA_LEN);
	return (ip_hdr);
}

void	pr_iph(struct ip *ip)
{
	u_char	*cp;
	int		hlen;

	hlen = ip->ip_hl << 2;
	cp = (u_char*)ip + 20; // pointer on options

	(void)printf("Vr HL TOS  Len   ID Flg  off TTL Pro  cks      Src      Dst\n");
	(void)printf(
		" %1x  %1x  %02x %04x %04x",
		ip->ip_v, ip->ip_hl, ip->ip_tos, ntohs(ip->ip_len),
		ntohs(ip->ip_id)
	);
	(void)printf(
		"   %1lx %04lx",
		(u_long) (ntohl(ip->ip_off) & 0xe000) >> 13,
		(u_long) ntohl(ip->ip_off) & 0x1fff
	);
	(void)printf(
		"  %02x  %02x %04x",
		ip->ip_ttl,
		ip->ip_p,
		ntohs(ip->ip_sum)
	);
	(void)printf(" %s ", inet_ntoa(*(struct in_addr *)&ip->ip_src.s_addr));
	(void)printf(" %s ", inet_ntoa(*(struct in_addr *)&ip->ip_dst.s_addr));

	// print options
	while (hlen-- > 20) {
		(void)printf("%02x", *cp++);
	}
	(void)putchar('\n');
}

void	pr_retip(struct ip *ip)
{
	u_char	*cp;
	int		hlen;

	pr_iph(ip);
	hlen = ip->ip_hl << 2;
	cp = (u_char*)ip + hlen;

	if (ip->ip_p == 6)
		(void)printf(
			"TCP: from port %u, to port %u (decimal)\n",
			(*cp * 256 + *(cp + 1)), (*(cp + 2) * 256 + *(cp + 3))
		);
	else if (ip->ip_p == 17)
		(void)printf(
			"UDP: from port %u, to port %u (decimal)\n",
			(*cp * 256 + *(cp + 1)), (*(cp + 2) * 256 + *(cp + 3))
		);
}

void	pr_icmph(struct icmp *icp)
{

	switch(icp->icmp_type) {

		case ICMP_ECHOREPLY:
			(void)printf("Echo Reply\n");
			/* XXX ID + Seq + Data */
			break;
		case ICMP_UNREACH:
			switch(icp->icmp_code) {
			case ICMP_UNREACH_NET:
				(void)printf("Destination Net Unreachable\n");
				break;
			case ICMP_UNREACH_HOST:
				(void)printf("Destination Host Unreachable\n");
				break;
			case ICMP_UNREACH_PROTOCOL:
				(void)printf("Destination Protocol Unreachable\n");
				break;
			case ICMP_UNREACH_PORT:
				(void)printf("Destination Port Unreachable\n");
				break;
			case ICMP_UNREACH_NEEDFRAG:
				(void)printf("frag needed and DF set (MTU %d)\n",
						ntohs(icp->icmp_nextmtu));
				break;
			case ICMP_UNREACH_SRCFAIL:
				(void)printf("Source Route Failed\n");
				break;
			case ICMP_UNREACH_FILTER_PROHIB:
				(void)printf("Communication prohibited by filter\n");
				break;
			default:
				(void)printf("Dest Unreachable, Bad Code: %d\n",
					icp->icmp_code);
				break;
			}
			/* Print returned IP header information */
			pr_retip((struct ip *)icp->icmp_data);
			break;
		case ICMP_SOURCEQUENCH:
			(void)printf("Source Quench\n");
			pr_retip((struct ip*)icp->icmp_data);
			break;
		case ICMP_REDIRECT:
			switch(icp->icmp_code) {
			case ICMP_REDIRECT_NET:
				(void)printf("Redirect Network");
				break;
			case ICMP_REDIRECT_HOST:
				(void)printf("Redirect Host");
				break;
			case ICMP_REDIRECT_TOSNET:
				(void)printf("Redirect Type of Service and Network");
				break;
			case ICMP_REDIRECT_TOSHOST:
				(void)printf("Redirect Type of Service and Host");
				break;
			default:
				(void)printf("Redirect, Bad Code: %d", icp->icmp_code);
				break;
			}
			(void)printf("(New addr: %s)\n", inet_ntoa(icp->icmp_gwaddr));
			pr_retip((struct ip *)icp->icmp_data);
			break;
		case ICMP_ECHO:
			(void)printf("Echo Request\n");
			/* XXX ID + Seq + Data */
			break;
		case ICMP_TIMXCEED:
			switch(icp->icmp_code) {
			case ICMP_TIMXCEED_INTRANS:
				(void)printf("Time to live exceeded\n");
				break;
			case ICMP_TIMXCEED_REASS:
				(void)printf("Frag reassembly time exceeded\n");
				break;
			default:
				(void)printf("Time exceeded, Bad Code: %d\n",
					icp->icmp_code);
				break;
			}
			pr_retip((struct ip *)icp->icmp_data);
			break;
		case ICMP_PARAMPROB:
			(void)printf("Parameter problem: pointer = 0x%02x\n",
				icp->icmp_hun.ih_pptr);
			pr_retip((struct ip *)icp->icmp_data);
			break;
		case ICMP_TSTAMP:
			(void)printf("Timestamp\n");
			/* XXX ID + Seq + 3 timestamps */
			break;
		case ICMP_TSTAMPREPLY:
			(void)printf("Timestamp Reply\n");
			/* XXX ID + Seq + 3 timestamps */
			break;
		case ICMP_IREQ:
			(void)printf("Information Request\n");
			/* XXX ID + Seq */
			break;
		case ICMP_IREQREPLY:
			(void)printf("Information Reply\n");
			/* XXX ID + Seq */
			break;
		case ICMP_MASKREQ:
			(void)printf("Address Mask Request\n");
			break;
		case ICMP_MASKREPLY:
			(void)printf("Address Mask Reply\n");
			break;
		case ICMP_ROUTERADVERT:
			(void)printf("Router Advertisement\n");
			break;
		case ICMP_ROUTERSOLICIT:
			(void)printf("Router Solicitation\n");
			break;
		default:
			(void)printf("Bad ICMP type: %d\n", icp->icmp_type);
	}
}

void	check_packet(char *buf, int cc) {

	struct icmp *icp;
	struct ip *ip;
	int hlen;
	char addr[16]; // buffer for inet_ntop()

	if (!IS_LINUX)
		((struct ip*)buf)->ip_len += ((struct ip*)buf)->ip_hl << 2;

	// Check the IP header
	ip = (struct ip *)buf;
	hlen = ip->ip_hl << 2;
	if (cc < hlen + ICMP_MINLEN) {

		if (g_ping.options & F_VERBOSE) {

			inet_ntop(AF_INET, (void*)&g_ping.dest_addr.sin_addr.s_addr, addr, sizeof(addr));
			(void)fprintf(
				stderr,
				"packet too short (%d bytes) from %s",
				cc,
				addr
			);
		}
		return;
	}

	// Check the ICMP header
	icp = (struct icmp *)(buf + hlen);
	cc -= hlen;
	if (icp->icmp_type == ICMP_ECHOREPLY) {

		if (ntohs(icp->icmp_id) != (unsigned short)getpid()) // not our packet
			return;
		g_ping.nreceived++;

		int seq = ntohs(icp->icmp_seq);

		if (TST(seq % MAX_DUP_CHK)) {

			++g_ping.nrepeats;
			--g_ping.nreceived;
		} else
			SET(seq % MAX_DUP_CHK);

		struct tv32 sent_tv32;
		struct timeval now;
		(void)gettimeofday(&now, NULL);
		struct tv32 cur_tv32;
		memcpy((void*)&sent_tv32, &((char*)icp)[ICMP_MINLEN], TV_LEN);

		cur_tv32.tv32_sec = (u_int32_t)now.tv_sec - ntohl(sent_tv32.tv32_sec);
		cur_tv32.tv32_usec = (u_int32_t)now.tv_usec - ntohl(sent_tv32.tv32_usec);
		double triptime = ((double)cur_tv32.tv32_sec) * 1000.0 + ((double)cur_tv32.tv32_usec) / 1000.0;

		g_ping.tsum += triptime;
		g_ping.tsumsq += triptime * triptime;

		if (triptime < g_ping.tmin)
			g_ping.tmin = triptime;
		if (triptime > g_ping.tmax)
			g_ping.tmax = triptime;

		inet_ntop(AF_INET, &ip->ip_src, addr, sizeof(addr));
		(void)printf(
			"%u bytes from %s: icmp_seq=%u ttl=%u time=%.3f ms",
			cc,
			addr,
			ntohs(icp->icmp_hun.ih_idseq.icd_seq),
			ip->ip_ttl,
			triptime
		);

		// check the data
		u_char *cp = (u_char*)&icp->icmp_dun.id_data[TV_LEN];
		u_char *dp = &((u_char*)g_ping.pkt)[sizeof(struct ip) + ICMP_MINLEN + TV_LEN];
		for (int i = 0; i < DATA_LEN - (int)TV_LEN && cc > 0; ++i, ++cp, ++dp, --cc) {

			if (*cp != *dp) {

				(void)printf("\nwrong data byte #%d should be 0x%x but was 0x%x", i, *dp, *cp);
				(void)printf("\ncp:");
				cp = (u_char*)icp->icmp_data;
				for (i = 0; i < DATA_LEN; ++i, ++cp) {

					if ((i % 16) == 0)
						(void)printf("\n\t");
					(void)printf("%2x ", *cp);
				}
				(void)printf("\ndp:");
				cp = &((u_char*)g_ping.pkt)[sizeof(struct ip) + ICMP_MINLEN];
				for (i = 0; i < DATA_LEN; ++i, ++cp) {

					if ((i % 16) == 0)
						(void)printf("\n\t");
					(void)printf("%2x ", *cp);
				}
				break;
			}
		}
	} else { // not an echoreply

		struct ip *oip = (struct ip *)icp->icmp_data;
		struct icmp *oicmp = (struct icmp *)(oip + 1);

		if (
			(g_ping.options & F_VERBOSE && getuid() == 0) ||
			(oip->ip_dst.s_addr == g_ping.dest_addr.sin_addr.s_addr &&
			oip->ip_p == IPPROTO_ICMP &&
			oicmp->icmp_type == ICMP_ECHO &&
			oicmp->icmp_id == htons((unsigned short)getpid()))
		) { // if verbose or is a reply to our packet

			inet_ntop(AF_INET, (void*)&g_ping.dest_addr.sin_addr.s_addr, addr, sizeof(addr));
			(void)printf(
				"%d bytes from %s: ",
				cc,
				addr
			);
			pr_icmph(icp);
		} else
			return ;
	}
	putchar('\n');
}

int		create_socket(void) {

	int s;

	uid_t uid = getuid();
	if (uid != 0 && !IS_LINUX)
		s = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
	else
		s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

	if (s < 0) {

		(void)printf("%s\n", strerror(errno));
		exit(1);
	}

	int hold = 1;
	// Tell the socket to not add the ip header because we provide our own
	setsockopt(s, IPPROTO_IP, IP_HDRINCL, (char*)&hold, sizeof(hold));

	return (s);
}

struct sockaddr_in	build_dest_addr() {

	struct sockaddr_in dest = (struct sockaddr_in){0};

	dest.sin_family = AF_INET;
	memset(dest.sin_zero, 0, sizeof(dest.sin_zero));

	if (inet_pton(AF_INET, g_ping.hostname, &dest.sin_addr) != 1) {

		struct addrinfo hints, *result;
		memset(&hints, 0, sizeof (hints));
		hints.ai_family = AF_INET;

		int error;
		if ((error = getaddrinfo(g_ping.hostname, NULL, &hints, &result)) != 0) {

			(void)printf("ping: cannot resolve %s: Unknown host\n", g_ping.hostname);
			exit(1);
		}

		memcpy(&dest.sin_addr, &result->ai_addr->sa_data[2], sizeof(dest.sin_addr));
	}
	return (dest);
}

struct ip	*create_ping_packet(void) {

	struct ip *pkt = (struct ip*)calloc(1, sizeof(struct ip) + ICMP_MINLEN + DATA_LEN);

	pkt = build_iphdr(pkt);
	pkt = build_icmphdr(pkt);

	return (pkt);
}

struct msghdr	*create_msg_receiver(void) {

	struct msghdr *msg = (struct msghdr*)calloc(1, sizeof(struct msghdr));
	struct iovec *iov = (struct iovec*)calloc(1, sizeof(struct iovec));
	unsigned char *recv_packet = (unsigned char*)calloc(1, IP_MAXPACKET);

	iov->iov_base = recv_packet;
	iov->iov_len = IP_MAXPACKET;
	msg->msg_iov = iov;
	msg->msg_iovlen = 1;

	return (msg);
}

struct ip	*update_packet(struct ip* pkt) {

	struct icmp	*icmp_hdr = (struct icmp*)(pkt + 1);
	struct timeval now;
	struct tv32 tv32;

	icmp_hdr->icmp_hun.ih_idseq.icd_seq = htons(g_ping.ntransmitted);
	g_ping.ntransmitted++;

	(void)gettimeofday(&now, NULL);

	tv32.tv32_sec = htonl(now.tv_sec);
	tv32.tv32_usec = htonl(now.tv_usec);
	memcpy((void*)&((char*)icmp_hdr)[ICMP_MINLEN], (void*)&tv32, TV_LEN);

	icmp_hdr->icmp_cksum = 0;
	icmp_hdr->icmp_cksum = in_cksum((unsigned short*)icmp_hdr, ICMP_MINLEN + DATA_LEN);
	return (pkt);
}

void	pinger(void) {

	g_ping.pkt = update_packet(g_ping.pkt);
	ssize_t sent = sendto(g_ping.s, (char*)g_ping.pkt, sizeof(struct ip) + ICMP_MINLEN + DATA_LEN, 0, (struct sockaddr*)&g_ping.dest_addr, sizeof(struct sockaddr_in));
	if (sent < 0)
		(void)printf("ping: sendto: %s\n", strerror(errno));
	struct timeval now;
	(void)gettimeofday(&now, NULL);
	g_ping.last_ping.tv32_sec = (u_int32_t)now.tv_sec;
	g_ping.last_ping.tv32_usec = (u_int32_t)now.tv_usec;
}

void	usage(void) {

	fprintf(stderr, "usage: ping [-v] host\n");
	exit(EX_USAGE);
}

int		main(int ac, char **av) {

	g_ping.s = create_socket();
	struct timeval now;
	struct timeval timeout;
	int arg;

	while ((arg = getopt(ac, av, "hv")) != -1) {

		switch (arg) {

			case 'h':
				usage();
				break;
			case 'v':
				g_ping.options |= F_VERBOSE;
				break;
			default:
				usage();
		}
	}

	if (ac - optind != 1)
		usage();

	// host is the last argument
	g_ping.hostname = av[optind];

	g_ping.dest_addr = build_dest_addr();
	g_ping.pkt = create_ping_packet();

	signal(SIGINT, stopit);
	signal(SIGQUIT, stopit);

	// First ping
	char	addr[16];
	inet_ntop(AF_INET, (void*)&g_ping.dest_addr.sin_addr.s_addr, addr, sizeof(addr));
	(void)printf("PING %s (%s): %d data bytes\n", g_ping.hostname, addr, DATA_LEN);

	ssize_t sent = sendto(g_ping.s, (char*)g_ping.pkt, sizeof(struct ip) + ICMP_MINLEN + DATA_LEN, 0, (struct sockaddr*)&g_ping.dest_addr, sizeof(struct sockaddr_in));
	if (sent < 0)
		(void)printf("ping: sendto: %s\n", strerror(errno));

	(void)gettimeofday(&now, NULL);
	g_ping.last_ping.tv32_sec = (u_int32_t)now.tv_sec;
	g_ping.last_ping.tv32_usec = (u_int32_t)now.tv_usec;
	struct msghdr* msg = create_msg_receiver();

	ssize_t recv;
	while (1) {

		(void)gettimeofday(&now, NULL);
		timeout.tv_sec = (long)g_ping.last_ping.tv32_sec + PING_TIMEOUT_SEC - now.tv_sec;
		timeout.tv_usec = g_ping.last_ping.tv32_usec - now.tv_usec;
		while (timeout.tv_usec < 0) {
			timeout.tv_usec += 1000000;
			timeout.tv_sec--;
		}
		while (timeout.tv_usec >= 1000000) {
			timeout.tv_usec -= 1000000;
			timeout.tv_sec++;
		}
		if (timeout.tv_sec < 0) {
			timeout.tv_sec = 0;
			timeout.tv_usec = 1;
		}
		setsockopt(g_ping.s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

		recv = recvmsg(g_ping.s, msg, 0);

		if (errno == EAGAIN) { // timeout

			pinger();
			if (g_ping.ntransmitted - g_ping.nreceived - 1 > g_ping.nmissedmax) {

				g_ping.nmissedmax = g_ping.ntransmitted - g_ping.nreceived - 1;
				(void)printf(
					"Request timeout for icmp_seq %u\n",
					g_ping.ntransmitted - 2
				);
			}
			errno = 0;
			continue;
		}
		if (recv < 0) {

			(void)printf("%s\n", strerror(errno));
			exit(1);
		} else if (recv == 0) {

			(void)printf("recvmsg len 0, Connection closed\n");
			exit(1);
		}
		check_packet((char *)msg->msg_iov->iov_base, recv);
	}

	// Makes compiler happy
	return (1);
}
