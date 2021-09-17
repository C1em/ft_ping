/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_ping.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/08/12 18:38:26 by coremart          #+#    #+#             */
/*   Updated: 2021/09/17 22:27:48 by coremart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <sys/socket.h> // sendto() socket() CMSG_FIRSTHDR() CMSG_NXTHDR()
#include <netinet/in.h> // IPPROTO_ICMP
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <stdlib.h> // malloc() free()
#include <unistd.h> // getuid()
#include <errno.h>
#include <string.h>
#include <netinet/if_ether.h> // struct ether_header
#include <unistd.h> // getpid()
#include <stdio.h>
#include <netdb.h> //  getaddrinfo()
#include <sys/time.h>
#include <signal.h> // signal() SIGALARM
#include "ft_ping.h"

struct ping g_ping;

void	print_mem(void* ptr, unsigned int len) {

	for (int i = 0; i < len; i++) {

		printf("0x%x ", ((unsigned char*)ptr)[i]);
	}
	printf("\n");
}

unsigned short in_cksum(unsigned short *ptr, unsigned int len) {

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
	ip_hdr->ip_tos = 0;
	ip_hdr->ip_ttl = 64;
	ip_hdr->ip_p = IPPROTO_ICMP;
	ip_hdr->ip_id = htons((unsigned short)getpid());

	ip_hdr->ip_off = 0;
	ip_hdr->ip_src.s_addr = INADDR_ANY;

	if (inet_pton(AF_INET, DEST_IP, &(ip_hdr->ip_dst)) <= 0) {

		printf("ip_dst\n");
		exit(1);
	}

	// on Linux ip_len is filled by the kernel
	if (!IS_LINUX)
		ip_hdr->ip_len = (unsigned short)(sizeof(struct ip) + ICMP_MINLEN + TV_LEN);

	// done by kernel
	// ip_hdr->ip_sum = htons(in_cksum((unsigned short*)ip_hdr, 20));
	return (ip_hdr);
}

void	print_ip_hdr(struct ip* ip_hdr) {

	printf("_____________________\n");
	printf("ip_hl: %u\n", ip_hdr->ip_hl);
	printf("ip_v: %u\n", ip_hdr->ip_v);
	printf("ip_tos: %u\n", ip_hdr->ip_tos);
	if (IS_LINUX)
		printf("ip_len: %u\n", ntohs(ip_hdr->ip_len));
	else // on FreeBSD and Macos ip_len is in host byte order
		printf("ip_len: %u\n", ip_hdr->ip_len);
	printf("ip_id: %u\n", ntohs(ip_hdr->ip_id));
	printf("ip_off: %u\n", ip_hdr->ip_off);
	printf("ip_ttl: %u\n", ip_hdr->ip_ttl);
	printf("ip_p: %u\n", ip_hdr->ip_p);
	printf("ip_sum: %u\n", ntohs(ip_hdr->ip_sum));

	char addr[16];
	inet_ntop(AF_INET, &ip_hdr->ip_src, addr, sizeof(addr));
	printf("ip_src: %s\n", addr);
	inet_ntop(AF_INET, &ip_hdr->ip_dst, addr, sizeof(addr));
	printf("ip_dst: %s\n", addr);
	printf("_____________________\n");
}

void	print_icmp_hdr(struct ip* ip_hdr) {


	struct icmp* icmp_hdr = (struct icmp*)(ip_hdr + 1);
	printf("_____________________\n");
	printf("icmp_type: %u\n", icmp_hdr->icmp_type);
	printf("icmp_type: %u\n", icmp_hdr->icmp_code);
	printf("icmp_hun.ih_idseq.icd_id: %u\n", ntohs(icmp_hdr->icmp_hun.ih_idseq.icd_id));
	printf("icmp_hun.ih_idseq.icd_seq: %u\n", icmp_hdr->icmp_hun.ih_idseq.icd_seq);
	printf("icmp_cksum: %u\n", icmp_hdr->icmp_cksum);

	struct tv32 sent_tv32;
	struct timeval now;
	(void)gettimeofday(&now, NULL);
	struct tv32 cur_tv32;
	memcpy((void*)&sent_tv32, &((char*)icmp_hdr)[ICMP_MINLEN], TV_LEN);

	cur_tv32.tv32_sec = (u_int32_t)now.tv_sec - ntohl(sent_tv32.tv32_sec);
	cur_tv32.tv32_usec = (u_int32_t)now.tv_usec - ntohl(sent_tv32.tv32_usec);
	double triptime = ((double)cur_tv32.tv32_sec) * 1000.0 + ((double)cur_tv32.tv32_usec) / 1000.0;

	printf("icmp tv32_sec: %u\n", ntohl(sent_tv32.tv32_sec));
	printf("icmp tv32_usec: %u\n", ntohl(sent_tv32.tv32_usec));
	printf("travel time: %.3f ms\n", triptime);
	printf("_____________________\n");
}

struct ip	*build_icmphdr(struct ip* ip_hdr) {

	struct icmp	*icmp_hdr = (struct icmp*)(ip_hdr + 1);
	struct timeval now;
	struct tv32 tv32;

	icmp_hdr->icmp_type = ICMP_ECHO;
	icmp_hdr->icmp_code = 0;
	icmp_hdr->icmp_cksum = 0;

	icmp_hdr->icmp_hun.ih_idseq.icd_id = htons((unsigned short)getpid());
	icmp_hdr->icmp_hun.ih_idseq.icd_seq = g_ping.npackets;
	g_ping.npackets++;

	(void)gettimeofday(&now, NULL);

	tv32.tv32_sec = htonl(now.tv_sec);
	tv32.tv32_usec = htonl(now.tv_usec);
	memcpy((void*)&((char*)icmp_hdr)[ICMP_MINLEN], (void*)&tv32, TV_LEN);

	icmp_hdr->icmp_cksum = in_cksum((unsigned short*)icmp_hdr, ICMP_MINLEN + TV_LEN);
	return (ip_hdr);
}

void check_packet(char *buf, int cc) {

	u_char *cp;
	struct icmp *icp;
	struct ip *ip;
	int hlen;
	int recv_len;

	if (!IS_LINUX)
		((struct ip*)buf)->ip_len += ((struct ip*)buf)->ip_hl << 2;

	// Check the IP header
	ip = (struct ip *)buf;
	hlen = ip->ip_hl << 2;
	recv_len = cc;
	if (cc < hlen + ICMP_MINLEN) {

		printf("received packet too small ?\n");
		exit(1);
	}

	// Now the ICMP part
	icp = (struct icmp *)(buf + hlen);
	if (icp->icmp_type == ICMP_ECHOREPLY) {

		if (ntohs(icp->icmp_id) != (unsigned short)getpid()) {

			printf("not our packet\n");
			exit(1);
		}
		char ip_src[16];
		inet_ntop(AF_INET, &ip->ip_src, ip_src, sizeof(ip_src));

		struct tv32 sent_tv32;
		struct timeval now;
		(void)gettimeofday(&now, NULL);
		struct tv32 cur_tv32;
		memcpy((void*)&sent_tv32, &((char*)icp)[ICMP_MINLEN], TV_LEN);

		cur_tv32.tv32_sec = (u_int32_t)now.tv_sec - ntohl(sent_tv32.tv32_sec);
		cur_tv32.tv32_usec = (u_int32_t)now.tv_usec - ntohl(sent_tv32.tv32_usec);
		double triptime = ((double)cur_tv32.tv32_sec) * 1000.0 + ((double)cur_tv32.tv32_usec) / 1000.0;

		printf(
			"%u bytes from %s: icmp_seq=%u ttl=%u time=%f ms\n",
			(IS_LINUX) ? ntohs(ip->ip_len) : ip->ip_len,
			ip_src,
			icp->icmp_hun.ih_idseq.icd_seq,
			ip->ip_ttl,
			triptime
		);

	} else {

		// drop ICMP_ECHO
		if (icp->icmp_type == ICMP_ECHO) {

			printf("recv ICMP_ECHO\n");
			return ;
		}

		printf("not a echoreply type: %u\n", icp->icmp_type);
		exit(1);
	}
}

int		create_socket(void) {

	int s;

	uid_t uid = getuid();
	if (uid != 0 && !IS_LINUX)
		s = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
	else
		s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

	if (s < 0) {

		printf("%s\n", strerror(errno));
		exit(1);
	}

	int hold = 1;
	// tell the socket to not add the ip header because we provide our own
	setsockopt(s, IPPROTO_IP, IP_HDRINCL, (char *)&hold, sizeof(hold));

	// int hold = IP_MAXPACKET + 128;
	// (void)setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&hold, sizeof(hold));
	// if (getuid() == 0)
	// 	(void)setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *)&hold, sizeof(hold));

	return (s);
}

struct sockaddr_in	build_dest_addr(char* str_dest) {

	struct sockaddr_in dest = (struct sockaddr_in){0};

	dest.sin_family = AF_INET;
	inet_pton(AF_INET, str_dest, &(dest.sin_addr));
	memset(dest.sin_zero, 0, sizeof(dest.sin_zero));

	return (dest);
}

struct ip	*create_ping_packet(void) {

	struct ip *pkt = (struct ip*)calloc(1, sizeof(struct ip) + ICMP_MINLEN + TV_LEN);

	pkt = build_iphdr(pkt);
	pkt = build_icmphdr(pkt);

	// print_ip_hdr(pkt);
	// print_icmp_hdr(pkt);

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

	icmp_hdr->icmp_hun.ih_idseq.icd_seq = g_ping.npackets;
	g_ping.npackets++;

	(void)gettimeofday(&now, NULL);

	tv32.tv32_sec = htonl(now.tv_sec);
	tv32.tv32_usec = htonl(now.tv_usec);
	memcpy((void*)&((char*)icmp_hdr)[ICMP_MINLEN], (void*)&tv32, TV_LEN);

	icmp_hdr->icmp_cksum = 0;
	icmp_hdr->icmp_cksum = in_cksum((unsigned short*)icmp_hdr, ICMP_MINLEN + TV_LEN);
	return (pkt);
}

// void	ping_loop(void) {

// 	int s = create_socket();
// 	struct sockaddr_in dest_addr = build_dest_addr(DEST_IP);
// 	struct ip *pkt = create_ping_packet();

// 	// First ping
// 	ssize_t sent = sendto(s, (char*)pkt, sizeof(struct ip) + ICMP_MINLEN + TV_LEN, 0, (struct sockaddr*)&dest_addr, sizeof(struct sockaddr_in));
// 	if (sent < 0) {

// 		printf("errno %d: %s\n", errno, strerror(errno));
// 		exit(1);
// 	}
// 	printf("PING %s (%s): %zd data bytes\n", DEST_IP, DEST_IP, sent);
// 	struct msghdr* msg = create_msg_receiver();
// 	ssize_t recv;

// 	while(1) {

// 		printf("here1\n");
// 		recv = recvmsg(s, msg, 0);
// 		if (recv < 0) {

// 			printf("%s\n", strerror(errno));
// 			exit(1);
// 		} else if (recv == 0) {

// 			printf("recvmsg len 0, Connection closed\n");
// 			exit(1);
// 		}

// 		printf("here2\n");
// 		check_packet((char *)msg->msg_iov->iov_base, recv);

// 		pkt = update_packet(pkt);
// 		sent = sendto(s, (char*)pkt, sizeof(struct ip) + ICMP_MINLEN + TV_LEN, 0, (struct sockaddr*)&dest_addr, sizeof(struct sockaddr_in));
// 		if (sent < 0) {

// 			printf("errno %d: %s\n", errno, strerror(errno));
// 			exit(1);
// 		}
// 		printf("here3\n");
// 	}
// }

void	pinger(int signum) {

	g_ping.pkt = update_packet(g_ping.pkt);
	struct sockaddr_in dest_addr = build_dest_addr(DEST_IP);
	ssize_t sent = sendto(g_ping.s, (char*)g_ping.pkt, sizeof(struct ip) + ICMP_MINLEN + TV_LEN, 0, (struct sockaddr*)&dest_addr, sizeof(struct sockaddr_in));
	if (sent < 0) {

		printf("errno %d: %s\n", errno, strerror(errno));
		exit(1);
	}
	alarm(1);
}

int		main(void) {

	g_ping.s = create_socket();
	struct sockaddr_in dest_addr = build_dest_addr(DEST_IP);
	g_ping.pkt = create_ping_packet();

	// First ping
	ssize_t sent = sendto(g_ping.s, (char*)g_ping.pkt, sizeof(struct ip) + ICMP_MINLEN + TV_LEN, 0, (struct sockaddr*)&dest_addr, sizeof(struct sockaddr_in));
	if (sent < 0) {

		printf("errno %d: %s\n", errno, strerror(errno));
		exit(1);
	}
	printf("PING %s (%s): %zd data bytes\n", DEST_IP, DEST_IP, sent);
	struct msghdr* msg = create_msg_receiver();

	ssize_t recv = recvmsg(g_ping.s, msg, 0);
	if (recv < 0) {

		printf("%s\n", strerror(errno));
		exit(1);
	} else if (recv == 0) {

		printf("recvmsg len 0, Connection closed\n");
		exit(1);
	}

	check_packet((char *)msg->msg_iov->iov_base, recv);

	signal(SIGALRM, pinger);
	alarm(1);

	while (1) {

		recv = recvmsg(g_ping.s, msg, 0);
		if (recv < 0) {

			printf("%s\n", strerror(errno));
			exit(1);
		} else if (recv == 0) {

			printf("recvmsg len 0, Connection closed\n");
			exit(1);
		}

		check_packet((char *)msg->msg_iov->iov_base, recv);
	}
	return (0);
}
