#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>

#include "mydhcp.h"

void read_config(char *);
void insert_ip(struct ip_list *);
struct ip_list *remove_ip();
struct client *search_client(struct in_addr);
struct client *new_client(struct in_addr, struct in_addr, struct in_addr);
void put_dhcph(struct dhcph *, uint8_t, uint8_t, uint16_t,
			   in_addr_t, in_addr_t);
void print_dhcph(struct dhcph *);
void skt_init_str(struct sockaddr_in *, char []);
void skt_init(struct sockaddr_in *, in_addr_t);
int mysendto(int, struct dhcph *, struct sockaddr_in *);
int myrecvfrom(int, struct dhcph *, struct sockaddr_in *);

int ttl;
struct ip_list ip_head;
struct client client_list;

int main(int argc, char *argv[])
{
	int s;
	fd_set rdfds;
	struct sockaddr_in myskt, cliskt, from;
	struct dhcph header;
	struct ip_list *ip;
	struct client *cl;
	struct timeval timeout;
	
	ip_head.fp = ip_head.bp = &ip_head;
	client_list.fp = client_list.bp = &client_list;

	if (argc != 2) {
		fprintf(stderr, "Usage: ./mydhcps config-file\n");
		exit(1);
	}
	read_config(argv[1]);
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}
	skt_init(&myskt, htonl(INADDR_ANY));
	if (bind(s, (struct sockaddr *)&myskt, sizeof myskt) < 0){
		perror("bind");
		exit(1);
	}

	// DISCOVER 受信待ち
	printf("-- wait for event --\n");
	myrecvfrom(s, &header, &from);
	// OFFER 送信
	if ((ip = remove_ip()) == NULL) {
		put_dhcph(&header, 2, 1, 0, 0, 0);
	} else {
		cl = new_client(from.sin_addr, ip->address, ip->netmask);
		printf("-- IP address allocated: %s, ", inet_ntoa(ip->address));
		printf("%s\n", inet_ntoa(ip->netmask));
		put_dhcph(&header, 2, 0, cl->ttl, cl->addr.s_addr, cl->netmask.s_addr);
	}
	mysendto(s, &header, &from);
	printf("## OFFER sent ##\n");
	print_dhcph(&header);
	printf("## status changed: INIT -> WAIT_REQ ##\n");
	
	// REQUEST 受信待ち
	myrecvfrom(s, &header, &from);
	// ACK 送信
	put_dhcph(&header, 4, 0, cl->ttl, cl->addr.s_addr, cl->netmask.s_addr);
	mysendto(s, &header, &from);
	printf("## ACK sent ##\n");
	print_dhcph(&header);
	printf("## status changed: WAIT_REQ -> IN_USE ##\n");

	printf("\n-- wait for event --\n");
	for (;;) {
		FD_ZERO(&rdfds);
		FD_SET(s, &rdfds);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		if (select(s+1, &rdfds, NULL, NULL, &timeout) == 0) {
			if (--(cl->ttlcounter) == 0) {
				break;
			}
			printf("-- client IP: %s, TTL %d --\n",
				   inet_ntoa(cl->id), cl->ttlcounter);
			continue;
		}
		if (FD_ISSET(s, &rdfds)) {
			// REQUEST 受信待ち
			myrecvfrom(s, &header, &from);
			// ACK 送信
			put_dhcph(&header, 4, 0, ttl, cl->addr.s_addr, cl->netmask.s_addr);
			mysendto(s, &header, &from);
			printf("## ACK sent ##\n");
			print_dhcph(&header);
			cl->ttlcounter = ttl;
			printf("\n-- wait for event --\n");
		}
	}
	if ((ip = (struct ip_list *)malloc(sizeof(struct ip_list))) == NULL) {
		fprintf(stderr, "malloc error\n");
		exit(1);
	}
	ip->address.s_addr = cl->addr.s_addr;
	ip->netmask.s_addr = cl->netmask.s_addr;
	insert_ip(ip);
	printf("\n-- IP address released: %s, ", inet_ntoa(cl->addr));
	printf("%s\n", inet_ntoa(cl->netmask));
	printf("\n-- wait for event --\n");
	
	// RELEASE 受信待ち
	myrecvfrom(s, &header, &from);
	printf("## status changed: IN_USE -> TERMINATE ##\n");
	
	close(s);
}

void read_config(char *filename)
{
	FILE *fp;
	char lbuf[80], address[80], netmask[80];
	struct ip_list *p;
	
	if ((fp = fopen(filename, "r")) == NULL) {
        fprintf(stderr, "file open error: %s\n", filename);
        exit(1);
    }
	fgets(lbuf, sizeof lbuf, fp);
	ttl = atoi(lbuf);
	printf("-- config file: TTL %d --\n", ttl);
	
    while (fgets(lbuf, sizeof lbuf, fp) != NULL) {
		if (sscanf(lbuf, "%s %s", address, netmask) == EOF) {
			fprintf(stderr, "cofig-file syntax error: %s\n", filename);
			exit(1);
		}
		if ((p = (struct ip_list *)malloc(sizeof(struct ip_list))) == NULL) {
			fprintf(stderr, "malloc error\n");
			exit(1);
		}
		printf("-- config file: IP %s, netmask %s --\n", address, netmask);
		if (inet_aton(address, &p->address) < 0 ||
			inet_aton(netmask, &p->netmask) < 0) {
			perror("inet_aton");
			exit(1);
		}
		insert_ip(p);
    }
    fclose(fp);
	printf("\n");
}

void insert_ip(struct ip_list *p)
{
	p->fp = &ip_head;
	p->bp = ip_head.bp;
	ip_head.bp->fp = p;
	ip_head.bp = p;
}

struct ip_list *remove_ip()
{
	struct ip_list *p;

	if (ip_head.fp == &ip_head) {
		return NULL;
	}
	p = ip_head.fp;
	ip_head.fp->fp->bp = &ip_head;
	ip_head.fp = ip_head.fp->fp;

	return p;
}

struct client *search_client(struct in_addr addr)
{
	struct client *p;

	for (p = client_list.fp; p != &client_list; p = p->fp) {
		if (p->addr.s_addr == addr.s_addr) {
			return p;
		}
	}
	return NULL;
}

void insert_client(struct client *p)
{
	p->fp = &client_list;
	p->bp = client_list.bp;
	client_list.bp->fp = p;
	client_list.bp = p;
}

struct client *new_client(struct in_addr id, struct in_addr addr,
						  struct in_addr netmask)
{
	struct client *p;

	if ((p = (struct client *)malloc(sizeof(struct client))) == NULL) {
		fprintf(stderr, "malloc error\n");
		exit(1);
	}
	p->stat = INIT;
	p->ttlcounter = p->ttl = ttl;
	p->id = id;
	p->addr = addr;
	p->netmask = netmask;

	insert_client(p);
	return p;
}
