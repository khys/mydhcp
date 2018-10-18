#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mydhcp.h"

void put_dhcph(struct dhcph *p, uint8_t type, uint8_t code, uint16_t ttl,
			   in_addr_t address, in_addr_t netmask)
{
	p->type = type;
	p->code = code;
	p->ttl = ttl;
	p->address = address;
	p->netmask = netmask;
}

void print_dhcph(struct dhcph *p)
{
	char address[80], netmask[80];

	inet_ntop(AF_INET, &p->address, address, sizeof address);
	inet_ntop(AF_INET, &p->netmask, netmask, sizeof netmask);
	
	printf("type %d, code %d, ttl %d, IP %s, netmask %s\n",
		   p->type, p->code, p->ttl, address, netmask);
}

void skt_init_str(struct sockaddr_in *skt, char addr[])
{
	memset(skt, 0, sizeof *skt);
	skt->sin_family = AF_INET;
	skt->sin_port = htons(SRVPORT);
	
	if (inet_aton(addr, &skt->sin_addr) < 0) {
		perror("inet_aton");
		exit(1);
	}
}

void skt_init(struct sockaddr_in *skt, in_addr_t addr)
{
	memset(skt, 0, sizeof *skt);
	skt->sin_family = AF_INET;
	skt->sin_port = htons(SRVPORT);
	skt->sin_addr.s_addr = addr;
}

int mysendto(int s, struct dhcph *header, struct sockaddr_in *skt)
{
	int cnt;
	socklen_t sktlen = sizeof *skt;
	
	if ((cnt = sendto(s, header, sizeof *header, 0,
					  (struct sockaddr *)skt, sktlen)) < 0) {
		perror("sendto");
		exit(1);
	}
	
	return cnt;
}

int myrecvfrom(int s, struct dhcph *header, struct sockaddr_in *skt)
{
	int cnt;
	socklen_t sktlen = sizeof *skt;
	
	if ((cnt = recvfrom(s, header, sizeof *header, 0,
						(struct sockaddr *)skt, &sktlen)) < 0) {
		perror("recvfrom");
		exit(1);
	}
	printf("## message received from %s %d ##\n",
		   inet_ntoa(skt->sin_addr), ntohs(skt->sin_port));
	print_dhcph(header);
	
	return cnt;
}
