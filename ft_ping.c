/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_ping.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: coremart <coremart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/08/12 18:38:26 by coremart          #+#    #+#             */
/*   Updated: 2021/08/28 22:02:07 by coremart         ###   ########.fr       */
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

#include <assert.h>
#define SO_TRAFFIC_CLASS	0x1086
#define DEST_IP				"127.0.0.1"

#ifdef __linux__
	#define IS_LINUX (1)
#else
	#define IS_LINUX (0)
#endif

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

	ip_hdr->ip_len = (unsigned short)(sizeof(struct ip) + ICMP_MINLEN);

	// done by kernel
	// ip_hdr->ip_sum = htons(in_cksum((unsigned short*)ip_hdr, 20));
	return (ip_hdr);
}

void	print_ip_hdr(struct ip* ip_hdr) {

	printf("_____________________\n");
	printf("ip_hl: %u\n", ip_hdr->ip_hl);
	printf("ip_v: %u\n", ip_hdr->ip_v);
	printf("ip_tos: %u\n", ip_hdr->ip_tos);
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
	printf("icmp_cksum: %u\n", ntohs(icmp_hdr->icmp_cksum));
	printf("_____________________\n");
}

struct ip	*build_icmphdr(struct ip* ip_hdr) {

	struct icmp	*icmp_hdr = (struct icmp*)(ip_hdr + 1);
	icmp_hdr->icmp_type = ICMP_ECHO;
	icmp_hdr->icmp_code = 0;
	icmp_hdr->icmp_cksum = 0;

	icmp_hdr->icmp_hun.ih_idseq.icd_id = htons((unsigned short)getpid());
	// TODO: increment this
	icmp_hdr->icmp_hun.ih_idseq.icd_seq = 0;

	icmp_hdr->icmp_cksum = htons(in_cksum((unsigned short*)icmp_hdr, 8));
	return (ip_hdr);
}

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

		printf("received packet too small ?\n");
		exit(1);
	}

	/* Now the ICMP part */
	icp = (struct icmp *)(buf + hlen);
	if (icp->icmp_type == ICMP_ECHOREPLY) {

		if (ntohs(icp->icmp_id) != (unsigned short)getpid()) {

			printf("not our packet\n");
			exit(1);
		}

		printf("icmp_type: %u\n", icp->icmp_type);
		printf("icmp_code: %u\n", icp->icmp_code);
		printf("icmp_dun.id_data: %p\n", (void*)&icp->icmp_dun.id_data);
		printf("&buf[hlen + ICMP_MINLEN]: %p\n", (void*)&buf[hlen + ICMP_MINLEN]);
		printf("&buf[tot_len]: %p\n", (void*)&buf[cc]);
		printf("data size: %zu\n", (size_t)&buf[cc] - (size_t)&buf[hlen + ICMP_MINLEN]);
		printf("data size: %zu\n", (size_t)&buf[cc] - (size_t)&buf[2 * hlen + ICMP_MINLEN]);

		print_ip_hdr((struct ip*)&buf[hlen + ICMP_MINLEN]);

	} else {

		printf("not a echoreply\n");
		exit(1);
	}
}

int		main(void) {

	unsigned char recv_packet[IP_MAXPACKET] __attribute__((aligned(4)));
	struct ip *pkt = (struct ip*)calloc(1, sizeof(struct ip) + ICMP_MINLEN);
	struct sockaddr_in whereto = (struct sockaddr_in){0};
	struct sockaddr_in from = (struct sockaddr_in){0};
	struct msghdr msg;
	struct iovec iov[1];
	int s; // socket


	whereto.sin_family = AF_INET;
	inet_pton(AF_INET, DEST_IP, &(whereto.sin_addr));
	memset(whereto.sin_zero, 0, sizeof(whereto.sin_zero));

	pkt = build_iphdr(pkt);
	pkt = build_icmphdr(pkt);

	uid_t uid = getuid();
	if (uid != 0 && !IS_LINUX)
		s = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
	else
		s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);

	printf("socket: %d\n", s);
	if (s < 0) {

		printf("%s\n", strerror(errno));
		exit(1);
	}



	// int hold = IP_MAXPACKET + 128;
	// (void)setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&hold, sizeof(hold));
	// if (getuid() == 0)
	// 	(void)setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *)&hold, sizeof(hold));
	int hold = 1;
	setsockopt(s, IPPROTO_IP, IP_HDRINCL, (char *)&hold, sizeof(hold));

	print_ip_hdr(pkt);
	print_icmp_hdr(pkt);
	ssize_t sent = sendto(s, (char*)pkt, sizeof(struct ip) + ICMP_MINLEN, 0, (struct sockaddr*)&whereto, sizeof(struct sockaddr_in));
	printf("try to send: %zu\nsent: %zd\n", sizeof(struct ip) + ICMP_MINLEN, sent);
	if (sent < 0) {

		printf("errno %d: %s\n", errno, strerror(errno));
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

		printf("recvmsg len 0, Connection closed\n");
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
