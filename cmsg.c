/* Some useful cmsg headers and parsing stuff.

IP_RECVOPTS
IP_RECVTTL
IP_TTL

IPV6_RECVPKTINFO (since Linux 2.6.14)
              Set delivery of the IPV6_PKTINFO control message on incoming
              datagrams.  Such control messages contain a struct
              in6_pktinfo, as per RFC 3542.  Only allowed for SOCK_DGRAM or
              SOCK_RAW sockets.  Argument is a pointer to a boolean value in
              an integer.

IPV6_DSTOPTS, IPV6_HOPOPTS, IPV6_FLOWINFO,
       IPV6_HOPLIMIT
              Set delivery of control messages for incoming datagrams con-
              taining extension headers from the received packet.  IPV6_RTH-
              DR delivers the routing header, IPV6_AUTHHDR delivers the au-
              thentication header, IPV6_DSTOPTS delivers the destination op-
              tions, IPV6_HOPOPTS delivers the hop options, IPV6_FLOWINFO
              delivers an integer containing the flow ID, IPV6_HOPLIMIT de-
              livers an integer containing the hop count of the packet.  The
              control messages have the same type as the socket option.  All
              these header options can also be set for outgoing packets by
              putting the appropriate control message into the control buf-
              fer of sendmsg(2).  Only allowed for SOCK_DGRAM or SOCK_RAW
              sockets.  Argument is a pointer to a boolean value.

SO_TIMESTAMPNS
SO_TIMESTAMP

hardware support

SO_TIMESTAMPING:

http://os1a.cs.columbia.edu/lxr/source/Documentation/networking/timestamping.txt?a=x86
*/

/* I strongly suspect this will need tweaking to work this way */

int parse_header(packet_t *p)
{
           struct msghdr msgh;
           struct cmsghdr *cmsg;
           int *ttlptr;
           int *tosptr;
           int received_ttl = -1;
	   int recieved_tos = -1;

           /* Receive auxiliary data in msgh */
           for (cmsg = CMSG_FIRSTHDR(&msgh); cmsg != NULL;
                   cmsg = CMSG_NXTHDR(&msgh,cmsg)) {
               if (cmsg->cmsg_level == IPPROTO_IP
                       && cmsg->cmsg_type == IP_TTL) {
                   ttlptr = (int *) CMSG_DATA(cmsg);
                   received_ttl = *ttlptr;
                   break;
               }
               if (cmsg->cmsg_level == IPPROTO_IPV6
                       && cmsg->cmsg_type == IPV6_SOMETHING) {
                   ttlptr = (int *) CMSG_DATA(cmsg);
                   received_ttl = *ttlptr;
                   break;
               }
               if (cmsg->cmsg_level == IPPROTO_IP
                       && cmsg->cmsg_type == IP_TOS) {
                   tosptr = (int *) CMSG_DATA(cmsg);
                   received_tos = *tosptr;
                   break;
               }
               if (cmsg->cmsg_level == IPPROTO_IPV6
                       && cmsg->cmsg_type == IPV6_TCLASS) {
                   tosptr = (int *) CMSG_DATA(cmsg);
                   received_tos = *tosptr;
                   break;
               }
	       // timestamp?
           }
           if (cmsg == NULL) {
               /*
                * Error: IP_TTL not enabled or small buffer
                * or I/O error.
                */
           }
}
