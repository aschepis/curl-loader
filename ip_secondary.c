/* 
 *      ip_secondary.c
 *
 *        This program is free software; you can redistribute it and/or
 *        modify it under the terms of the GNU General Public License
 *        as published by the Free Software Foundation; either version
 *        2 of the License, or (at your option) any later version.
 *
 * Authors:    Alexey Kuznetsov, <kuznet@ms2.inr.ac.ru>
 *
 * The ideas and most of the implementation comes from iprouted2
 * project, directory ip, written by Alexey Kuznetsov.
 * Cutted and combined by Robert Iakobashvili, <coroberti@gmail.com>
 */

// must be first include
#include "fdsetsize.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <fnmatch.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

#include <linux/netdevice.h>
#include <linux/if_arp.h>
#include <linux/sockios.h>


#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>


struct rtnl_handle
{
  int	 fd;
  struct sockaddr_nl local;
  struct sockaddr_nl peer;
  __u32 seq;
  __u32 dump;
};

typedef struct
{
  __u8 family;
  __u8 bytelen;
  __s16 bitlen;
  __u32 data[4];
} inet_prefix;

struct idxmap
{
  struct idxmap * next;
  int		index;
  int		type;
  int		alen;
  unsigned	flags;
  unsigned char	addr[8];
  char		name[16];
};

static struct idxmap *idxmap[16];


/*
Interface address.

struct ifaddrmsg
{
unsigned char   ifa_family;
unsigned char   ifa_prefixlen;  // The prefix length 
unsigned char   ifa_flags;      // Flags
unsigned char   ifa_scope;      // See above 
int             ifa_index;      // Link index
};
enum
{
IFA_UNSPEC,
IFA_ADDRESS,
IFA_LOCAL,
IFA_LABEL,
IFA_BROADCAST,
IFA_ANYCAST,
IFA_CACHEINFO
};
struct nlmsghdr
{
__u32           nlmsg_len;      // Length of message including header
__u16           nlmsg_type;     // Message content
__u16           nlmsg_flags;    // Additional flags
__u32           nlmsg_seq;      // Sequence number
__u32           nlmsg_pid;      //Sending process PID
};
*/


static int get_prefix(inet_prefix *dst, char *arg, int family);
static int get_prefix_1(inet_prefix *dst, char *arg, int family);
static int get_integer(int *val, const char *arg, int base);
static int get_addr_1(inet_prefix *addr, const char *name, int family);
static int addattr_l(struct nlmsghdr *n, int maxlen, int type, void *data, int alen);
static int default_scope(inet_prefix *lcl);
static int rtnl_open(struct rtnl_handle *rth, unsigned subscriptions);
static int ll_init_map(struct rtnl_handle *rth);
static int ll_remember_index(struct sockaddr_nl *who, struct nlmsghdr *n, void *arg);
static int rtnl_talk(struct rtnl_handle *rtnl, struct nlmsghdr *n, pid_t peer,
              unsigned groups, struct nlmsghdr *answer,
              int (*junk)(struct sockaddr_nl *,struct nlmsghdr *n, void *),
              void *jarg);
static int rtnl_dump_filter(struct rtnl_handle *rth,
                     int (*filter)(struct sockaddr_nl *, struct nlmsghdr *n, void *),
                     void *arg1,
                     int (*junk)(struct sockaddr_nl *,struct nlmsghdr *n, void *),
                     void *arg2);
static int rtnl_wilddump_request(struct rtnl_handle *rth, int family, int type);
static int parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta, int len);
static int ll_name_to_index(char *name);
static int rtnl_rtscope_a2n(__u32 *id, char *arg);
static void rtnl_rtscope_initialize(void);

typedef struct
{
  struct nlmsghdr 	n;
  struct ifaddrmsg 	ifa;
  char   			buf[256];
} request;


/*******************************************************************************
* Function name - add_secondary_ip_to_device
*
* Description - Adds a secondary IPv4 address to a valid networking device.
* Input -       *device - network device name as linux sees it, like "eth0"
*               *ip_slash_mask - string in the form of ipv4/mask, e.g. "192.168.0.1/24"
* Return Code/Output - On Success - 0, on Error -1
********************************************************************************/
int add_secondary_ip_to_device(const char*const device, 
                               const char*const ip_slash_mask,
                               char* scope) 
{
  request req;

  static struct rtnl_handle rth =
    {
      -1,
      {0, 0, 0, 0}, 
      {0, 0, 0, 0},
      0,
      0
    };
 	
  const char*const d = device; /* e.g. "eth0" */
  inet_prefix lcl, peer;
  int  peer_len = 0, local_len = 0;

  memset(&req, 0, sizeof(req));

  req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
  req.n.nlmsg_flags = NLM_F_REQUEST;
  req.n.nlmsg_type = RTM_NEWADDR;
  req.ifa.ifa_family = AF_UNSPEC;

  get_prefix(&lcl, (char*)ip_slash_mask, req.ifa.ifa_family);
  if (req.ifa.ifa_family == AF_UNSPEC)
    req.ifa.ifa_family = lcl.family;
  addattr_l(&req.n, sizeof(req), IFA_LOCAL, &lcl.data, lcl.bytelen);
  local_len = lcl.bytelen;
   
  if (peer_len == 0 && local_len) 
    {
      peer = lcl;
      addattr_l(&req.n, sizeof(req), IFA_ADDRESS, &lcl.data, lcl.bytelen);
    }

  if (req.ifa.ifa_prefixlen == 0)
    req.ifa.ifa_prefixlen = lcl.bitlen;

  /* do not support broadcast addresses */

  __u32 scope_id = 0;

  if (scope && scope[0])
    {
      if (rtnl_rtscope_a2n(&scope_id, scope)) 
        {
          fprintf (stderr, "%s - error: invalid scope \"%s\".\n", __func__, scope);
          return -1;
        }
      else
        {
          req.ifa.ifa_scope = scope_id;
        }
    }
  else
    {
      req.ifa.ifa_scope = default_scope(&lcl);
    }

  if (rth.fd < 0)
    {
      if (rtnl_open(&rth, 0) < 0)
        return -1;
    }

  ll_init_map(&rth);

  if ((req.ifa.ifa_index = ll_name_to_index((char*)d)) == 0) 
    {
      fprintf (stderr, "%s - Cannot find device \"%s\"\n", __func__, d);
      return -1;
    }

  if (rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL) < 0)
    return -2;

  return 0;
}

static int get_prefix(inet_prefix *dst, char *arg, int family)
{
  if (family == AF_PACKET) 
    {
      fprintf(stderr, 
              "%s - Error: \"%s\" may be inet prefix, but it is not allowed in this context.\n",
              __func__, arg);
      exit(1);
    }
  if (get_prefix_1(dst, arg, family)) 
    {
        fprintf(stderr, 
                "%s - Error: an inet prefix is expected rather than \"%s\".\n", 
                __func__, arg);
      exit(1);
    }
  return 0;
}

static int get_prefix_1(inet_prefix *dst, char *arg, int family)
{
  int err;
  int plen;
  char *slash;

  memset(dst, 0, sizeof(*dst));
  if (strcmp(arg, "default") == 0 || strcmp(arg, "any") == 0 || 
      strcmp(arg, "all") == 0) 
    {
      if (family == AF_DECnet)
        return -1;
      dst->family = family;
      dst->bytelen = 0;
      dst->bitlen = 0;
      return 0;
    }

  slash = strchr(arg, '/');
  if (slash)
    *slash = 0;
  err = get_addr_1(dst, arg, family);
  if (err == 0) 
    {
      switch(dst->family) 
        {
        case AF_INET6:
          dst->bitlen = 128;
          break;
        case AF_DECnet:
          dst->bitlen = 16;
          break;
        default:
        case AF_INET:
          dst->bitlen = 32;
        }
        
      if (slash) 
        {
	   
          if (get_integer(&plen, slash+1, 0) || plen > dst->bitlen)
            {
              err = -1;
              goto done;
            }
          dst->bitlen = plen;
        }
    }
 done:
  if (slash)
    *slash = '/';
  return err;
}

static int get_integer(int *val, const char *arg, int base)
{
  long res;
  char *ptr;

  if (!arg || !*arg)
    return -1;
  res = strtol(arg, &ptr, base);
  if (!ptr || ptr == arg || *ptr || res > INT_MAX || res < INT_MIN)
    return -1;
  *val = res;
  return 0;
}

static int addattr_l(struct nlmsghdr *n, int maxlen, int type, void *data, int alen)
{
  int len = RTA_LENGTH(alen);
  struct rtattr *rta;

  if ((int)NLMSG_ALIGN(n->nlmsg_len) + len > maxlen)
    return -1;
  rta = (struct rtattr*)(((char*)n) + NLMSG_ALIGN(n->nlmsg_len));
  rta->rta_type = type;
  rta->rta_len = len;
  memcpy(RTA_DATA(rta), data, alen);
  n->nlmsg_len = NLMSG_ALIGN(n->nlmsg_len) + len;

  return 0;
}

static int default_scope(inet_prefix *lcl)
{
  if (lcl->family == AF_INET) 
    {		
      if (lcl->bytelen >= 1 && *(__u8*)&lcl->data == 127)
        return RT_SCOPE_HOST;
    }
  return 0;
}

static int rtnl_open(struct rtnl_handle *rth, unsigned subscriptions)
{
  socklen_t addr_len;

  memset(rth, 0, sizeof(rth)) ;

  rth->fd = socket (AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
  if (rth->fd < 0) 
    {
      perror("rtnl_open(): Cannot open netlink socket");
      return -1;
    }

  memset(&rth->local, 0, sizeof(rth->local));
  rth->local.nl_family = AF_NETLINK;
  rth->local.nl_groups = subscriptions;
    
  if (bind(rth->fd, (struct sockaddr*)&rth->local, sizeof(rth->local)) < 0) 
    {
      perror("rtnl_open(): Cannot bind netlink socket");
      return -1;
    }
  addr_len = sizeof(rth->local);
  if (getsockname(rth->fd, (struct sockaddr*)&rth->local, &addr_len) < 0) 
    {
      perror("rtnl_open(): Cannot getsockname");
      return -1;
    }
  if (addr_len != sizeof(rth->local)) 
    {
        fprintf(stderr, "%s - error: Wrong address length %d\n", 
                __func__, addr_len);
        return -1;
    }
  if (rth->local.nl_family != AF_NETLINK) 
    {
      fprintf(stderr, "%s - error: Wrong address family %d\n", 
              __func__, rth->local.nl_family);
      return -1;
    }
  rth->seq = time(NULL);
  return 0;
}

static int ll_init_map(struct rtnl_handle *rth)
{
  if (rtnl_wilddump_request(rth, AF_UNSPEC, RTM_GETLINK) < 0) 
    {
      perror("ll_init_map(): Cannot send dump request");
      exit(1);
    }
  if (rtnl_dump_filter(rth, ll_remember_index, &idxmap, NULL, NULL) < 0) 
    {
        fprintf(stderr, "%s - Dump terminated\n", __func__);
        exit(1);
    }
  return 0;
}

static int rtnl_talk(struct rtnl_handle *rtnl, 
          struct nlmsghdr *n, 
          pid_t peer,
          unsigned groups, 
          struct nlmsghdr *answer,
          int (*junk)(struct sockaddr_nl *, struct nlmsghdr *n, void *),
          void *jarg)
{
  int status;
  unsigned seq;
  struct nlmsghdr *h;
  struct sockaddr_nl nladdr;
  struct iovec iov = { (void*)n, n->nlmsg_len };
  char   buf[8192];
  struct msghdr msg = 
    {
      (void*)&nladdr, 
      sizeof(nladdr),
      &iov,  
      1,
      NULL,	
      0,
      0
    };

  memset(&nladdr, 0, sizeof(nladdr));
  nladdr.nl_family = AF_NETLINK;
  nladdr.nl_pid = peer;
  nladdr.nl_groups = groups;

  n->nlmsg_seq = seq = ++rtnl->seq;
  if (answer == NULL)
    n->nlmsg_flags |= NLM_F_ACK;

  status = sendmsg(rtnl->fd, &msg, 0);

  if (status < 0) 
    {
      perror("rtnl_talk(): Cannot talk to rtnetlink");
      return -1;
    }

  iov.iov_base = buf;

  while (1) 
    {
      iov.iov_len = sizeof(buf);
      status = recvmsg(rtnl->fd, &msg, 0);

      if (status < 0) 
        {
          if (errno == EINTR)
            continue;
          perror("rtnl_talk(): recvmsg OVERRUN");
          continue;
        }
      if (status == 0) 
        {
            fprintf(stderr, "%s - error: EOF on netlink\n", __func__);
          return -1;
        }
      if (msg.msg_namelen != sizeof(nladdr)) 
        {
          fprintf(stderr, "%s - sender address length == %d\n", 
                  __func__, msg.msg_namelen);
          exit(1);
        }
		
      for (h = (struct nlmsghdr*)buf; status >= (int)sizeof(*h); ) 
        {
          int err;
          int len = h->nlmsg_len;
          int l = len - sizeof(*h);

          if (l<0 || len>status) 
            {
              if (msg.msg_flags & MSG_TRUNC) 
                {
                    fprintf(stderr, "%s - Truncated message\n", __func__);
                  return -1;
                }
              fprintf(stderr, "%s - !!!malformed message: len=%d\n", __func__, len);
              exit(1);
            }
          if ((int)nladdr.nl_pid != peer || 
              h->nlmsg_pid != rtnl->local.nl_pid || 
              h->nlmsg_seq != seq) 
            {
              if (junk) 
                {
                  err = junk(&nladdr, h, jarg);
                  if (err < 0)
                    return err;
                }
              continue;
            }

          if (h->nlmsg_type == NLMSG_ERROR) 
            {
              struct nlmsgerr *err = (struct nlmsgerr*)NLMSG_DATA(h);

              if (l < (int)sizeof(struct nlmsgerr)) 
                {
                    fprintf(stderr, "%s - ERROR truncated\n", __func__);
                } 
              else 
                {
                  errno = -err->error;
                  if (errno == 0) 
                    {
                      if (answer)
                        memcpy(answer, h, h->nlmsg_len);
                      return 0;
                    }
                  perror("rtnl_talk(): RTNETLINK answers");
                }
              return -1;
            }
          if (answer) 
            {
              memcpy(answer, h, h->nlmsg_len);
              return 0;
            }

          fprintf(stderr, "%s - Unexpected reply!!!\n", __func__);
          status -= NLMSG_ALIGN(len);
          h = (struct nlmsghdr*)((char*)h + NLMSG_ALIGN(len));
        }
		
      if (msg.msg_flags & MSG_TRUNC) 
        {
            fprintf(stderr, "%s - Message truncated\n", __func__);
          continue;
        }	   
      if (status) 
        {		
            fprintf(stderr, "%s - !!!Remnant of size %d\n", __func__, status);
            exit(1);
        }
    }
}

static int get_addr_1(inet_prefix *addr, const char *name, int family)
{
  const char *cp;
  unsigned char *ap = (unsigned char*)addr->data;
  int i;

  memset(addr, 0, sizeof(*addr));

  if (strcmp(name, "default") == 0 || strcmp(name, "all") == 0 || 
      strcmp(name, "any") == 0)
    {
      if (family == AF_DECnet)
        return -1;
      addr->family = family;
      addr->bytelen = (family == AF_INET6 ? 16 : 4);
      addr->bitlen = -1;
      return 0;
    }

  if (strchr(name, ':')) 
    {
      addr->family = AF_INET6;
      if (family != AF_UNSPEC && family != AF_INET6)
        return -1;
      if (inet_pton(AF_INET6, name, addr->data) <= 0)
        return -1;
      addr->bytelen = 16;
      addr->bitlen = -1;
      return 0;
    }

  if (family == AF_DECnet) 
    {
      return -1;
    }

  addr->family = AF_INET;
  if (family != AF_UNSPEC && family != AF_INET)
    return -1;
  addr->bytelen = 4;
  addr->bitlen = -1;
  for (cp=name, i=0; *cp; cp++) 
    {
      if (*cp <= '9' && *cp >= '0') 
        {
          ap[i] = 10*ap[i] + (*cp-'0');
          continue;
        }
      if (*cp == '.' && ++i <= 3)
        continue;
      return -1;
    }
  return 0;
}

static int ll_remember_index(struct sockaddr_nl *who, struct nlmsghdr *n, void *arg)
{
  int h;
  struct ifinfomsg *ifi = NLMSG_DATA(n);
  struct idxmap *im, **imp;
  struct rtattr *tb[IFLA_MAX+1];

  (void)who;
  (void)arg;

  if (n->nlmsg_type != RTM_NEWLINK)
    return 0;

  if (n->nlmsg_len < NLMSG_LENGTH(sizeof(ifi)))
    return -1;

  memset(tb, 0, sizeof(tb));
  parse_rtattr(tb, IFLA_MAX, IFLA_RTA(ifi), IFLA_PAYLOAD(n));
  if (tb[IFLA_IFNAME] == NULL)
    return 0;

  h = ifi->ifi_index&0xF;

  for (imp=&idxmap[h]; (im=*imp)!=NULL; imp = &im->next)
    if (im->index == ifi->ifi_index)
      break;

  if (im == NULL) 
    {
      im = malloc(sizeof(*im));
      if (im == NULL)
        return 0;
      im->next = *imp;
      im->index = ifi->ifi_index;
      *imp = im;
    }

  im->type = ifi->ifi_type;
  im->flags = ifi->ifi_flags;

  if (tb[IFLA_ADDRESS]) 
    {
      int alen;
      im->alen = alen = RTA_PAYLOAD(tb[IFLA_ADDRESS]);
      if (alen > (int)sizeof(im->addr))
        alen = sizeof(im->addr);
      memcpy(im->addr, RTA_DATA(tb[IFLA_ADDRESS]), alen);
    } 
  else 
    {
      im->alen = 0;
      memset(im->addr, 0, sizeof(im->addr));
    }
  strcpy(im->name, RTA_DATA(tb[IFLA_IFNAME]));
  return 0;
}

static int rtnl_wilddump_request(struct rtnl_handle *rth, int family, int type)
{
  struct 
  {
    struct nlmsghdr nlh;
    struct rtgenmsg g;
  } req;
    
  struct sockaddr_nl nladdr;

  memset(&nladdr, 0, sizeof(nladdr));
  nladdr.nl_family = AF_NETLINK;

  req.nlh.nlmsg_len = sizeof(req);
  req.nlh.nlmsg_type = type;
  req.nlh.nlmsg_flags = NLM_F_ROOT|NLM_F_MATCH|NLM_F_REQUEST;
  req.nlh.nlmsg_pid = 0;
  req.nlh.nlmsg_seq = rth->dump = ++rth->seq;
  req.g.rtgen_family = family;

  return sendto (rth->fd, (void*)&req, 
                 sizeof(req), 
                 0, 
                 (struct sockaddr*)&nladdr, 
                 sizeof(nladdr));
}

static int 
rtnl_dump_filter(struct rtnl_handle *rth,
                 int (*filter)(struct sockaddr_nl *, struct nlmsghdr *n, void *),
                 void *arg1,
                 int (*junk)(struct sockaddr_nl *,struct nlmsghdr *n, void *),
                 void *arg2)
{
  char	buf[8192];
  struct sockaddr_nl nladdr;
  struct iovec iov = { buf, sizeof(buf) };

  while (1) 
    {
      int status;
      struct nlmsghdr *h;
      struct msghdr msg = 
        {
          (void*)&nladdr, sizeof(nladdr),
          &iov,	
          1,
          NULL,	
          0,
          0
        };

      status = recvmsg(rth->fd, &msg, 0);

      if (status < 0) 
        {
          if (errno == EINTR)
            continue;
          perror("rtnl_dump_filter(): OVERRUN");
          continue;
        }
      if (status == 0) 
        {
            fprintf(stderr, "%s - EOF on netlink\n", __func__);
          return -1;
        }
		
      if (msg.msg_namelen != sizeof(nladdr)) 
        {
            fprintf(stderr, "%s - sender address length == %d\n", 
                    __func__, msg.msg_namelen);
          exit(1);
        }

      h = (struct nlmsghdr*)buf;
		
      while (NLMSG_OK(h, (unsigned) status)) 
        {
          int err;
          if (nladdr.nl_pid != 0 || h->nlmsg_pid != rth->local.nl_pid || h->nlmsg_seq != rth->dump) 
            {
              if (junk) 
                {
                  err = junk(&nladdr, h, arg2);
                  if (err < 0)
                    return err;
                }
              goto skip_it;
            }

          if (h->nlmsg_type == NLMSG_DONE)
            return 0;

          if (h->nlmsg_type == NLMSG_ERROR) 
            {
              struct nlmsgerr *err = (struct nlmsgerr*)NLMSG_DATA(h);			
              if (h->nlmsg_len < NLMSG_LENGTH(sizeof(struct nlmsgerr))) 
                {
                    fprintf(stderr, "%s - ERROR truncated\n", __func__);
                } 
              else 
                {
                  errno = -err->error;
                  perror("rtnl_dump_filter(): RTNETLINK answers");
                }
              return -1;
            }
          err = filter(&nladdr, h, arg1);
          if (err < 0)
            return err;

        skip_it:
			
          h = NLMSG_NEXT(h, status);
        }
        
      if (msg.msg_flags & MSG_TRUNC) 
        {
            fprintf(stderr, "%s - Message truncated\n", __func__);
          continue;
        }
      if (status) 
        {
            fprintf(stderr, "%s - !!!Remnant of size %d\n", __func__, status);
          exit(1);
        }
    }
}

static int parse_rtattr(struct rtattr *tb[], 
             int max, 
             struct rtattr *rta, 
             int len)
{
  while (RTA_OK(rta, len)) 
    {
      if (rta->rta_type <= max)
        tb[rta->rta_type] = rta;
      rta = RTA_NEXT(rta,len);
    }
  if (len)
    fprintf(stderr, "%s - !!!Deficit %d, rta_len=%d\n", __func__, len, rta->rta_len);
  return 0;
}

static int 
ll_name_to_index(char *name)
{
  static char ncache[16];
  static int icache;
  struct idxmap *im;
  int i;

  if (name == NULL)
    return 0;
  if (icache && strcmp(name, ncache) == 0)
    return icache;

  for (i=0; i<16; i++) 
    {
      for (im = idxmap[i]; im; im = im->next) 
        {
          if (strcmp(im->name, name) == 0) 
            {
              icache = im->index;
              strcpy(ncache, name);
              return im->index;
            }
        }
    }
  return 0;
}

static char * rtnl_rtscope_tab[256] = {
	"global",
};

static int rtnl_rtscope_init;

static void rtnl_rtscope_initialize(void)
{
	rtnl_rtscope_init = 1;
	rtnl_rtscope_tab[255] = "nowhere";
	rtnl_rtscope_tab[254] = "host";
	rtnl_rtscope_tab[253] = "link";
	rtnl_rtscope_tab[200] = "site";
	//rtnl_tab_initialize("/etc/iproute2/rt_scopes",
	//		    rtnl_rtscope_tab, 256);
}

static int rtnl_rtscope_a2n(__u32 *id, char *arg)
{
	static char *cache = NULL;
	static unsigned long res;
	char *end;
	int i;

	if (cache && strcmp(cache, arg) == 0) {
		*id = res;
		return 0;
	}

	if (!rtnl_rtscope_init)
		rtnl_rtscope_initialize();

	for (i=0; i<256; i++) {
		if (rtnl_rtscope_tab[i] &&
		    strcmp(rtnl_rtscope_tab[i], arg) == 0) {
			cache = rtnl_rtscope_tab[i];
			res = i;
			*id = res;
			return 0;
		}
	}

	res = strtoul(arg, &end, 0);
	if (!end || end == arg || *end || res > 255)
		return -1;
	*id = res;
	return 0;
}



/*******************************************************************************
* Function name - add_secondary_ip_addrs
*
* Description - Adds all secondary IPv4 addresses from array to network interface
* Input -       *interface - network device name as linux sees it, like "eth0"
*               addr_number - number of addresses to add
*               *addresses - array of strings of ipv4 addresses
*               netmask - CIDR notation netmask
* Return Code/Output - On Success - 0, on Error -1
********************************************************************************/
int add_secondary_ip_addrs (const char*const interface, 
                            int addr_number, 
                            const char**const addresses, 
                            int netmask,
                            char* addr_scope)
{
  char ip_slash_mask_buffer[64];
  int j = 0, rval_set_ip = -1;

  for (j = 0; j < addr_number && addresses[j] ; j++)
    {
        fprintf (stderr, "%s - setting secondary IP %s\n", __func__, addresses[j]);

        snprintf (ip_slash_mask_buffer, 
                  sizeof (ip_slash_mask_buffer) -1, 
                  "%s/%d", 
                  addresses[j], netmask);
            
      rval_set_ip = add_secondary_ip_to_device (interface, 
                                                ip_slash_mask_buffer,
                                                addr_scope);

      switch (rval_set_ip)
        {
        case -1:
          fprintf (stderr, 
                   "%s - error: failed with errno %d for ip %s\n", 
                   __func__, errno, ip_slash_mask_buffer);
          return -1;

        case -2:
          fprintf (stderr, 
                   "%s - note: probably, the IP-address \"%s\" already exists.\n", 
                   __func__, ip_slash_mask_buffer);
          break;

        case 0:
        default:
          fprintf (stderr, "%s - successfully added %s IP-address.\n", 
                   __func__,  ip_slash_mask_buffer);
          break;
        }
    }
  return 0;
}

