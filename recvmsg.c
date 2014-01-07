/* some basic examples for using recvmsg correctly.
   this is kind of deep magic overall so...
   lifted from : 
   http://stackoverflow.com/questions/2881200/linux-can-recvmsg-be-used-to-receive-the-ip-tos-of-every-incoming-packet
   
*/

int ttl = 60;
if(setsockopt(udpSocket, IPPROTO_IP, IP_RECVTTL, &ttl,sizeof(ttl))<0) 
{
	printf("cannot set recvttl\n");
} 
else
{
	printf("socket set to recvttl\n");
}

struct msghdr msg; 
struct iovec iov[1];  
memset(&msg, '\0', sizeof(msg));
msg.msg_iov = iov;
msg.msg_iovlen = 1;
iov[0].iov_base = (char *) &pkt;
iov[0].iov_len = sizeof(pkt);

int *ttlptr=NULL;
int received_ttl = 0;

int cmsg_size = sizeof(struct cmsghdr)+sizeof(received_ttl); // NOTE: Size of header + size of data
char buf[CMSG_SPACE(sizeof(received_ttl))];
msg.msg_control = buf; // Assign buffer space for control header + header data/value
msg.msg_controllen = sizeof(buf); //just initializing it

nRet = recvmsg(udpSocket, &msg, 0);

if (nRet > 0) {
	struct cmsghdr *cmsg;
	for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg,cmsg)) {

          if ((cmsg->cmsg_level == IPPROTO_IP) && (cmsg->cmsg_type == IP_TTL) &&
          (cmsg->cmsg_len) ){
                ttlptr = (int *) CMSG_DATA(cmsg);
                received_ttl = *ttlptr;
                printf("received_ttl = %i and %d \n", ttlptr, received_ttl); 
               break;
           }
    }
}
