/*
 * This file is Copyright (c) 2010 by the GPSD project
 * BSD terms apply: see the file COPYING in the distribution root for details.
 */
#include <string.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/ip.h>

socket_t netlib_connectsock(int af, const char *host, const char *service,
			    const char *protocol)
{
    struct protoent *ppe;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int ret, type, proto, one = 1;
    socket_t s;
    bool bind_me;

    ppe = getprotobyname(protocol);
    if (strcmp(protocol, "udp") == 0) {
	type = SOCK_DGRAM;
	proto = (ppe) ? ppe->p_proto : IPPROTO_UDP;
    } else {
	type = SOCK_STREAM;
	proto = (ppe) ? ppe->p_proto : IPPROTO_TCP;
    }
    bind_me = (type == SOCK_DGRAM);

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = af;
    hints.ai_socktype = type;
    hints.ai_protocol = proto;
    if (bind_me)
	hints.ai_flags = AI_PASSIVE;
    if ((ret = getaddrinfo(host, service, &hints, &result))) {
	return NL_NOHOST;
    }
    for (rp = result; rp != NULL; rp = rp->ai_next) {
	ret = NL_NOCONNECT;
	if ((s = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) < 0)
	    ret = NL_NOSOCK;
	else if (setsockopt
		 (s, SOL_SOCKET, SO_REUSEADDR, (char *)&one,
		  sizeof(one)) == -1) {
	    ret = NL_NOSOCKOPT;
	} else {
	    if (bind_me) {
		if (bind(s, rp->ai_addr, rp->ai_addrlen) == 0) {
		    ret = 0;
		    break;
		}
	    } else {
		if (connect(s, rp->ai_addr, rp->ai_addrlen) == 0) {
		    ret = 0;
		    break;
		}
	    }
	}

	if (!BAD_SOCKET(s)) {
	    (void)close(s);
	}
    }
    freeaddrinfo(result);
    if (ret)
	return ret;

    int opt = IPTOS_LOWDELAY;
    (void)setsockopt(s, IPPROTO_IP, IP_TOS, &opt, sizeof(opt));
    (void)setsockopt(s, IPPROTO_IPV6, IPV6_TCLASS, &opt, sizeof(opt));
    if (type == SOCK_STREAM)
	setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char *)&one, sizeof(one));
    /* set socket to noblocking */
    (void)fcntl(s, F_SETFL, fcntl(s, F_GETFL) | O_NONBLOCK);

    return s;
}

/*@+mustfreefresh +usedef@*/

const char /*@observer@*/ *netlib_errstr(const int err)
{
    switch (err) {
    case NL_NOSERVICE:
	return "can't get service entry";
    case NL_NOHOST:
	return "can't get host entry";
    case NL_NOPROTO:
	return "can't get protocol entry";
    case NL_NOSOCK:
	return "can't create socket";
    case NL_NOSOCKOPT:
	return "error SETSOCKOPT SO_REUSEADDR";
    case NL_NOCONNECT:
	return "can't connect to host/port pair";
    default:
	return "unknown error";
    }
}

char *netlib_sock2ip(socket_t fd)
/* retrieve the IP address corresponding to a socket */
{
    sockaddr_t fsin;
    socklen_t alen = (socklen_t) sizeof(fsin);
    /*@i1@*/ static char ip[INET6_ADDRSTRLEN];
    int r;

    r = getpeername(fd, &(fsin.sa), &alen);
    /*@ -branchstate -unrecog +boolint @*/
    if (r == 0) {
	switch (fsin.sa.sa_family) {
	case AF_INET:
	    r = !inet_ntop(fsin.sa_in.sin_family, &(fsin.sa_in.sin_addr),
			   ip, sizeof(ip));
	    break;
	case AF_INET6:
	    r = !inet_ntop((int)fsin.sa_in6.sin6_family, &(fsin.sa_in6.sin6_addr),
			   ip, sizeof(ip));
	    break;
	default:
	    (void)strlcpy(ip, "<unknown AF>", sizeof(ip));
	    return ip;
	}
    }
    if (r != 0) {
	(void)strlcpy(ip, "<unknown>", sizeof(ip));
    }
    return ip;
}
