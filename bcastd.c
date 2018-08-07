/*
 * bcastd (version 1.0a)
 * Written by ben@cooper.compsol.fidonet.org (Ben Elliston) on 5-May-94 
 * Updated for the 21st century on 7-Aug-2018.
 *
 * This daemon will broadcast a custom set of IP numbers to a machine which is
 * known to publicise its own routing table.
 *
 * Usage: bcastd <gateway> <ip1> <ip2> <ip3> <ip4> .. <ipn>
 *
 * <gateway> is the IP number of the machine which will publicise its routing
 * table via true routed.
 *
 * <ip1> .. <ipn> is the variable list of IP addresses which will be sent via
 * RIP packets to <gateway> every 30 seconds.  (This value can be adjusted in
 * the TIMER_RATE define statement below if need be).
 *
 * <gateway> and <ip#> address fields must be in the usual IP address form:
 *
 * a.b.c.d    (for example: 138.79.20.70)
 *
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <linux/route.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <netdb.h>

#define	RIPVERSION	1

struct netinfo
{
  struct sockaddr rip_dst;	/* destination net/host */
  int rip_metric;		/* cost of route */
};

struct rip
{
  u_char	rip_cmd;		/* request/response */
  u_char	rip_vers;		/* protocol version # */
  u_char	rip_res1[2];		/* pad to 32-bit boundary */
  union
  {
    struct	netinfo ru_nets[1];	/* variable length... */
    char	ru_tracefile[1];	/* ditto .. */
  } ripun;

#define	rip_nets	ripun.ru_nets
#define	rip_tracefile	ripun.ru_tracefile
};
 
#define	RIPCMD_REQUEST		1	/* want info */
#define	RIPCMD_RESPONSE		2	/* responding to request */
#define	RIPCMD_TRACEON		3	/* turn tracing on */
#define	RIPCMD_TRACEOFF		4	/* turn it off */
#define	RIPCMD_MAX		5

#define	HOPCNT_INFINITY		16	/* per XNS */
#define	MAXPACKETSIZE		512	/* max broadcast size */

#define	TIMER_RATE		30	/* broadcast every 30 seconds */

void getpacket(int fd);

int
main(int ac, char *av[])
{
    int fd, i;
    struct servent *sp;
    struct sockaddr_in addr;  
    struct sockaddr_in to;
    struct sockaddr_in lonet;

    memset(&addr, 0, sizeof(addr));
    memset(&to, 0, sizeof(to));
    memset(&lonet, 0, sizeof(lonet));

    sp = getservbyname("router", "udp");

    if (sp == NULL) 
    {
      perror("getservbyname");
      exit(1);
    }

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
    {
      perror("socket");
      exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = sp->s_port;

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) 
    {
      perror("bind");
      exit(1);
    }

    lonet.sin_family = htons(AF_INET);
    lonet.sin_port = 0; 

    to.sin_family = AF_INET;
    to.sin_port = sp->s_port;
    to.sin_addr.s_addr = inet_addr(av[1]);

    for (;;) 
    {
      struct rip rip;
      for (i = 2; i < ac; i++)
      {
        lonet.sin_addr.s_addr = inet_addr(av[i]);
        bzero(&rip, sizeof(rip));
        rip.rip_cmd = RIPCMD_RESPONSE;
        rip.rip_vers = RIPVERSION;
        rip.rip_nets[0].rip_dst = *(struct sockaddr *)&lonet;
        rip.rip_nets[0].rip_metric = htonl(2);
        if (sendto(fd, (void *)&rip, sizeof(rip), 0, (void *)&to, sizeof(to)) < 0)
          perror("sendto");
      }
      sleep(30);
    }
    return(0);
}
