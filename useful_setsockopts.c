#ifdef WINDOWS

// might as well try

#include <process.h>
#include <ws2tcpip.h>
#include <io.h>

#else  // if WINDOWS

#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#endif

/*
SO_TIMESTAMP
    Enable or disable the receiving of the SO_TIMESTAMP
    control message. The timestamp control message is sent with level
    SOL_SOCKET and the cmsg_data field is a struct timeval indicating
    the reception time of the last packet passed to the user in this
    call. See cmsg(3) for details on control messages. 
*/

struct socket_options {
	int sock;
	int sock_type;
	int rcvbuf;
	int sndbuf;
 	int family;
	int sndbuf;
	int mtu;
	uint8_t ttl;
	uint8_t dscp;
	int ecn;
	bool multicast;
};

typdef socket_options socketOptions_t;

void setup_sockets(socketOptions_t *o)
{
    int sock = o->sock;
    int dscp = o->dscp | ecn;

    if (o->rcvbuf) {
        if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *)&o->rcvbuf, 
                       sizeof(o->rcvbuf)) == SOCKET_ERROR) {
            socket_error(0, 0, "Error setting receive buffer size");
            exit(1);
        }
     }
    if (o->sndbuf) {
     if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *)&o->sndbuf, 
                       sizeof(o->sndbuf)) == SOCKET_ERROR) {
            socket_error(0, 0, "Error setting send buffer size");
            exit(1);
        }
    }
    if (o->family == AF_INET6) {
#ifdef IPV6_MTU_DISCOVER
// not sure why this gets disabled most places I've seen it
        {
            int mtuflag = IP_PMTUDISC_DONT;
            if (setsockopt(sock, IPPROTO_IPV6, IPV6_MTU_DISCOVER,
                           (char *)&o->mtu, sizeof(o->mtu)) == SOCKET_ERROR) {
                socket_error(0, 0, "Error disabling MTU discovery");
                socket_close(sock);
                exit(1);
            }
        }
#endif
#if defined IPV6_TCLASS && !defined WINDOWS
// fixme we care if we can't set these right
	if(o->family == AF_INET6)
        if (setsockopt(sock, IPPROTO_IPV6, IPV6_TCLASS, (char *)&dscp, 
                       sizeof(dscp)) == SOCKET_ERROR) {
            socket_warn(0, 0, "Error setting dscp");
        }
#else
#warn you are missing a header for IPV6_TCLASS
#endif
	if(o->family = AF_INET)
        if (setsockopt(sock, IPPROTO_IP, IP_TOS, (char *)&dscp, 
                       sizeof(dscp)) == SOCKET_ERROR) {
            socket_warn(0, 0, "Error setting dscp");
        }
#ifdef IP_MTU_DISCOVER
        {
            int mtuflag = IP_PMTUDISC_DONT;
            if (setsockopt(sock, IPPROTO_IP, IP_MTU_DISCOVER, (char *)&mtuflag, 
                           sizeof(mtuflag)) == SOCKET_ERROR) {
                socket_error(0, 0, "Error disabling MTU discovery");
                socket_close(sock);
                exit(1);
            }
        }
#endif
#ifdef USE_MULTICAST
if(o->multicast) {
        if (setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, (char *)&ttl, 
                       sizeof(ttl)) == SOCKET_ERROR) {
            socket_error(0, 0, "Error setting ttl");
            socket_close(sock);
            exit(1);
        }
        if (setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_IF,
                (char *)&out_if.ifidx, sizeof(int)) == SOCKET_ERROR) {
            socket_error(0, 0, "Error setting outgoing interface");
            socket_close(sock);
            exit(1);
        }
        if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &l_ttl, 
                       sizeof(l_ttl)) == SOCKET_ERROR) {
            socket_error(0, 0, "Error setting ttl");
            socket_close(sock);
            exit(1);
        }
        if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF,
                       (char *)&out_if.su.sin.sin_addr, 
                       sizeof(out_if.su.sin.sin_addr)) == SOCKET_ERROR) {
            socket_error(0, 0, "Error setting outgoing interface");
            socket_close(sock);
            exit(1);
        }
}
#endif

}
/* new sendmmsg msghdr stuff from the man pages (linux 3.0 only) */

/*
#define _GNU_SOURCE
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

int
main(void)
{
    int sockfd;
    struct sockaddr_in sa;
    struct mmsghdr msg[2];
    struct iovec msg1[2], msg2;
    int retval;

   sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

   sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    sa.sin_port = htons(1234);
    if (connect(sockfd, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
        perror("connect()");
        exit(EXIT_FAILURE);
    }

   memset(msg1, 0, sizeof(msg1));
    msg1[0].iov_base = "one";
    msg1[0].iov_len = 3;
    msg1[1].iov_base = "two";
    msg1[1].iov_len = 3;

   memset(&msg2, 0, sizeof(msg2));
    msg2.iov_base = "three";
    msg2.iov_len = 5;

   memset(msg, 0, sizeof(msg));
    msg[0].msg_hdr.msg_iov = msg1;
    msg[0].msg_hdr.msg_iovlen = 2;

   msg[1].msg_hdr.msg_iov = &msg2;
    msg[1].msg_hdr.msg_iovlen = 1;

   retval = sendmmsg(sockfd, msg, 2, 0);
    if (retval == -1)
        perror("sendmmsg()");
    else
        printf("%d messages sent\n", retval);

   exit(0);
}

// recvmm

#define _GNU_SOURCE
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

int
main(void)
{
#define VLEN 10
#define BUFSIZE 200
#define TIMEOUT 1
    int sockfd, retval, i;
    struct sockaddr_in sa;
    struct mmsghdr msgs[VLEN];
    struct iovec iovecs[VLEN];
    char bufs[VLEN][BUFSIZE+1];
    struct timespec timeout;

   sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

   sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    sa.sin_port = htons(1234);
    if (bind(sockfd, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

   memset(msgs, 0, sizeof(msgs));
    for (i = 0; i < VLEN; i++) {
        iovecs[i].iov_base         = bufs[i];
        iovecs[i].iov_len          = BUFSIZE;
        msgs[i].msg_hdr.msg_iov    = &iovecs[i];
        msgs[i].msg_hdr.msg_iovlen = 1;
    }

   timeout.tv_sec = TIMEOUT;
    timeout.tv_nsec = 0;

   retval = recvmmsg(sockfd, msgs, VLEN, 0, &timeout);
    if (retval == -1) {
        perror("recvmmsg()");
        exit(EXIT_FAILURE);
    }

   printf("%d messages received\n", retval);
    for (i = 0; i < retval; i++) {
        bufs[i][msgs[i].msg_len] = 0;
        printf("%d %s", i+1, bufs[i]);
    }
    exit(EXIT_SUCCESS);
}

*/
