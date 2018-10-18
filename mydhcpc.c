#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "mydhcp.h"

void put_dhcph(struct dhcph *, uint8_t, uint8_t, uint16_t,
			   in_addr_t, in_addr_t);
void print_dhcph(struct dhcph *);
void skt_init_str(struct sockaddr_in *, char []);
void skt_init(struct sockaddr_in *, in_addr_t);
int mysendto(int, struct dhcph *, struct sockaddr_in *);
int myrecvfrom(int, struct dhcph *, struct sockaddr_in *);

struct in_addr addr;
struct in_addr netmask;
uint16_t ttl;

int alrmflag = 0;
int hupflag = 0;
void alrm_func()
{
	alrmflag++;
}
void hup_func()
{
	hupflag++;
}

int main(int argc, char *argv[])
{
	int s;
	socklen_t srvlen, fromlen;
	struct sockaddr_in srvskt, from;
	struct dhcph header;
	struct itimerval value;
	
	if (argc != 2) {
		fprintf(stderr, "Usage: ./mydhcpc IP\n");
		exit(1);
	}
	printf("-- server IP: %s:%d --\n", argv[1], SRVPORT);
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}
	skt_init_str(&srvskt, argv[1]);

	// DISCOVER 送信
	put_dhcph(&header, 1, 0, 0, 0, 0);
	mysendto(s, &header, &srvskt);
	printf("## DISCOVER sent ##\n");
	print_dhcph(&header);
	printf("## status changed: INIT -> WAIT_OFFER ##\n");
	
	// OFFER 受信待ち
	myrecvfrom(s, &header, &from);
	// REQUEST 送信
	put_dhcph(&header, 3, 2, header.ttl, header.address, header.netmask);
	mysendto(s, &header, &srvskt);
	printf("## REQUEST sent ##\n");
	print_dhcph(&header);
	printf("## status changed: WAIT_OFFER -> WAIT_ACK ##\n");
	
	// ACK 受信待ち
	myrecvfrom(s, &header, &from);
	ttl = header.ttl;
	addr.s_addr = header.address;
	netmask.s_addr = header.netmask;
	printf("## status changed: WAIT_ACK -> IN_USE ##\n");

	signal(SIGALRM, alrm_func);
	signal(SIGHUP, hup_func);
	value.it_value.tv_usec = value.it_interval.tv_usec = 0;
	value.it_value.tv_sec = value.it_interval.tv_sec = ttl / 2;
	if (setitimer(ITIMER_REAL, &value, NULL) != 0) {
		fprintf(stderr, "timer error\n");
		exit(1);
	}
	for (;;) {
		pause();
		if (alrmflag > 0) {
			alrmflag = 0;
			// REQUEST 送信
			put_dhcph(&header, 3, 3, ttl, addr.s_addr, addr.s_addr);
			mysendto(s, &header, &srvskt);
			printf("## REQUEST sent ##\n");
			print_dhcph(&header);
			printf("## status changed: IN_USE -> WAIT_EXT_ACK ##\n");
			
			// ACK 受信待ち
			myrecvfrom(s, &header, &from);
			printf("## status changed: WAIT_EXT_ACK -> IN_USE ##\n");
		} else if (hupflag > 0) {
			hupflag = 0;
			printf("-- SIGHUP received\n");
			break;
		}
	}
	
	// RELEASE 送信
	put_dhcph(&header, 5, 0, 0, addr.s_addr, 0);
	mysendto(s, &header, &srvskt);
	printf("## RELEASE sent ##\n");
	print_dhcph(&header);
	printf("## status changed: IN_USE -> EXIT ##\n");

	close(s);
}
