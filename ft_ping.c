/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_ping.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/08/12 18:38:26 by coremart          #+#    #+#             */
/*   Updated: 2021/08/25 13:09:28 by coremart         ###   ########.fr       */
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

#include <stdio.h>

#define SO_TRAFFIC_CLASS	0x1086
#define PROCESS_ID			0
#define DEST_IP				"8.8.8.8"

#ifdef __linux__
	#define IS_LINUX (1)
#else
	#define IS_LINUX (0)
#endif

#if BYTE_ORDER == BIG_ENDIAN

#define PING_HTONS(n) (n)
#define PING_NTOHS(n) (n)
#define PING_HTONL(n) (n)
#define PING_NTOHL(n) (n)

#else

#define PING_HTONS(n) (((((unsigned short)(n) & 0xFF)) << 8) | (((unsigned short)(n) & 0xFF00) >> 8))
#define PING_NTOHS(n) (((((unsigned short)(n) & 0xFF)) << 8) | (((unsigned short)(n) & 0xFF00) >> 8))

#define PING_HTONL(n) (((((unsigned long)(n) & 0xFF)) << 24) | \
				((((unsigned long)(n) & 0xFF00)) << 8) | \
				((((unsigned long)(n) & 0xFF0000)) >> 8) | \
				((((unsigned long)(n) & 0xFF000000)) >> 24))

#define PING_NTOHL(n) (((((unsigned long)(n) & 0xFF)) << 24) | \
				((((unsigned long)(n) & 0xFF00)) << 8) | \
				((((unsigned long)(n) & 0xFF0000)) >> 8) | \
				((((unsigned long)(n) & 0xFF000000)) >> 24))
#endif


typedef struct ipheader {

#if __BYTE_ORDER == __LITTLE_ENDIAN
	unsigned int ihl:4;			// Internet Header Length: value between 5 and 15 depending of Options (always 5 for icmp)
	unsigned int version:4;		// Version: 4 for ipv4
#elif __BYTE_ORDER == __BIG_ENDIAN
	unsigned int version:4;
	unsigned int ihl:4;
#endif

	unsigned char tos;			// Type of Service: packet priority (0 for ping)
	unsigned short int len;		// Total Length: total size of packet
	unsigned short int id;		// Identifiaction: for defragmentation (don't use)
	unsigned short int off;		// Flags + Offset: for defragmentation (don't use)
	unsigned char ttl;			// Time to Live
	unsigned char p;			// Protocol: (ICMP)
	unsigned short int sum;		// Header checksum
	unsigned int src;			// Source address
	unsigned int dst;			// Destination address
} __attribute__((packed)) iphdr;

typedef struct icmpheader {

	u_char icmp_type;				// type of message
	u_char icmp_code;				// type sub code
	u_short icmp_cksum;				// ones complement checksum of struct
} __attribute__((packed)) icmphdr;

unsigned short in_cksum(void *ptr, unsigned int len) {

	unsigned short *u_int16_ptr = (unsigned short *)ptr;
	unsigned short checksum = 0;

	u_int16_ptr[1] = 0; // set the checksum to 0 in the packet

	for (int i = 0; i < len; i++)
		checksum += u_int16_ptr[i];

	return (~checksum);
}

// struct ipheader*	build_iphdr() {

// 	struct ipheader *ip_header = (struct ipheader*)malloc(sizeof(struct ipheader));
// 	ip_header->version = 4;
// 	ip_header->ihl = 5;
// 	ip_header->tos = 0;
// 	ip_header->ttl = 64;
// 	ip_header->p = IPPROTO_ICMP;
// 	ip_header->id = 0;
// 	ip_header->off = 0;
// 	inet_pton(AF_INET, "127.0.0.1", &(ip_header->src));
// 	inet_pton(AF_INET, DEST_IP, &(ip_header->dst));

// 	ip_header->len = 0; // TODO
// 	ip_header->sum = in_cksum(ip_header, 10);
// }

static void pr_pack(char *buf, int cc, struct sockaddr_in *from, int tc) {

	u_char *cp;
	struct icmp *icp;
	struct ip *ip;
	int hlen;
	int recv_len;

	/* Check the IP header */
	ip = (struct ip *)buf;
	hlen = ip->ip_hl << 2;
	recv_len = cc;
	if (cc < hlen + ICMP_MINLEN) {
		return;
	}

	/* Now the ICMP part */
	cc -= hlen;
	icp = (struct icmp *)(buf + hlen);
	if (icp->icmp_type == ICMP_ECHOREPLY) {
		if (icp->icmp_id != PROCESS_ID)
			return;			/* 'Twas not our ECHO */
		printf("icmp_type: %u\n", icp->icmp_type);
		printf("icmp_code: %u\n", icp->icmp_code);
		printf("icmp_dun.id_data: %p\n", (void*)&icp->icmp_dun.id_data);
		printf("&buf[hlen + ICMP_MINLEN]: %p\n", (void*)&buf[hlen + ICMP_MINLEN]);
		printf("&buf[tot_len]: %p\n", (void*)&buf[cc]);
	} else {

		printf("not a echoreply\n");
		exit(1);
	}
}


int		main(void) {

	struct icmp *pkt = (struct icmp*)calloc(1, sizeof(struct icmp));
	int s; // socket
	struct sockaddr_in whereto;
	struct msghdr msg;
	struct iovec iov[1];
	unsigned char recv_packet[IP_MAXPACKET] __attribute__((aligned(4)));
	struct sockaddr_in from;

	whereto.sin_family = AF_INET;
	inet_pton(AF_INET, DEST_IP, &(whereto.sin_addr));


	pkt->icmp_type = ICMP_ECHO;
	pkt->icmp_code = 0;
	pkt->icmp_cksum = 0;

	// TODO: put UNIX process ID
	pkt->icmp_hun.ih_idseq.icd_id = PROCESS_ID;
	// TODO: increment this
	pkt->icmp_hun.ih_idseq.icd_seq = 0;

	pkt->icmp_cksum = in_cksum(pkt, 4);

	uid_t uid = getuid();
	if (uid != 0 && !IS_LINUX)
		s = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
	else
		s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

	printf("socket: %d\n", s);
	if (s < 0) {

		printf("%s\n", strerror(errno));
		exit(1);
	}



	int hold = IP_MAXPACKET + 128;
	(void)setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&hold, sizeof(hold));
	if (getuid() == 0)
		(void)setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *)&hold, sizeof(hold));


	ssize_t sent = sendto(s, (char*)pkt, sizeof(struct icmp), 0, (struct sockaddr*)&whereto, sizeof(whereto));
	printf("try to send: %zu\nsent: %zd\n", sizeof(struct icmp), sent);
	if (sent < 0) {

		printf("%s\n", strerror(errno));
		exit(1);
	}

	iov[0].iov_base = recv_packet;
	iov[0].iov_len = sizeof(recv_packet);
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	msg.msg_name = (caddr_t)&from;
	msg.msg_namelen = sizeof(from);
	ssize_t recv = recvmsg(s, &msg, 0);
	printf("recv: %zd\n", recv);
	if (recv < 0) {

		printf("%s\n", strerror(errno));
		exit(1);
	} else if (recv == 0) {

		printf("recvmsg len 0, Connection closed");
		exit(1);
	}

	int tc = -1;
	struct cmsghdr *cmsg;
	for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {

				if (cmsg->cmsg_level == SOL_SOCKET &&
					cmsg->cmsg_type == SO_TRAFFIC_CLASS &&
					cmsg->cmsg_len == CMSG_LEN(sizeof(int))) {
					/* Copy to avoid alignment problems: */
					memcpy(&tc, CMSG_DATA(cmsg), sizeof(tc));
				}
		}
	pr_pack((char *)msg.msg_iov->iov_base, recv, &from, tc);

	return (0);
}
