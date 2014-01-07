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


// misc other stuff

struct DATA_to_SEND pkt;
struct msghdr msg; 
struct iovec iov[1];  
memset(&msg, '\0', sizeof(msg));
msg.msg_iov = iov;
msg.msg_iovlen = 1;
iov[0].iov_base = (char *) &pkt;
iov[0].iov_len = sizeof(pkt);
nRet = sendmsg(udpSocket, &msg,0); 

Receiver Side (assumption DATA_To_SEND has a parameter named "seq"):

struct DATA_to_SEND pkt;
seqNum = ((struct DATA_to_SEND *) iov[0].iov_base)->seq;


// for tos

unsigned char set = 0x03;
if(setsockopt(udpSocket, IPPROTO_IP, IP_RECVTOS, &set,sizeof(set))<0) 
{
    printf("cannot set recvtos\n");
} 
else
{
        printf("socket set to recvtos\n");
// and get it

struct PC_Pkt pkt;
 int *ecnptr;
 unsigned char received_ecn;

 struct msghdr msg; 
 struct iovec iov[1];  
 memset(&msg, '\0', sizeof(msg));
 msg.msg_iov = iov; 
 msg.msg_iovlen = 1;
 iov[0].iov_base = (char *) &pkt;
 iov[0].iov_len = sizeof(pkt);

 int cmsg_size = sizeof(struct cmsghdr)+sizeof(received_ecn);
 char buf[CMSG_SPACE(sizeof(received_ecn))];
 msg.msg_control = buf;
 msg.msg_controllen = sizeof(buf); 

 nRet = recvmsg(udpSocket, &msg, 0);

 if (nRet > 0) {
struct cmsghdr *cmsg;
for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL;
cmsg = CMSG_NXTHDR(&msg,cmsg)) {
         if ((cmsg->cmsg_level == IPPROTO_IP) && 
         (cmsg->cmsg_type == IP_TOS) && (cmsg->cmsg_len) ){
                ecnptr = (int *) CMSG_DATA(cmsg);
        received_ecn = *ecnptr;
        int isecn =  ((received_ecn & INET_ECN_MASK) == INET_ECN_CE);

                printf("received_ecn = %i and %d, is ECN CE marked = %d \n", ecnptr, received_ecn, isecn); 

                 break;
    }
     }
}
