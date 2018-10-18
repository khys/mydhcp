// #define ITIMER_REAL    0
// #define ITIMER_VIRTUAL 1
// #define ITIMER_PROF    2
#define SRVPORT        51230
// #define SRVPORT 51231 (when 51230 is in use)

// client stat
#define INIT           0
#define WAIT_OFFER     1
#define WAIT_ACK       2
#define IN_USE         3
#define WAIT_EXT_ACK   4
#define EXIT           5

// server stat
#define INIT           0
#define WAIT_REQ       1
#define IN_USE         3
#define TERMINATE      4

struct dhcph {
    uint8_t    type;
    uint8_t    code;
    uint16_t   ttl;
    in_addr_t  address;
    in_addr_t  netmask;
};

struct ip_list {
    struct in_addr address;
    struct in_addr netmask;
	struct ip_list *fp;
	struct ip_list *bp;
};

struct client {
	struct client *fp;
	struct client *bp;
	short stat;
	int ttlcounter;
	struct in_addr id;
	struct in_addr addr;
	struct in_addr netmask;
	uint16_t ttl;
};
