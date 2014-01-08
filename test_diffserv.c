/* Test survival of diffserv marked bits

   Might be able to assemble this with sendmsg.

   get values from dscp.c's ipqos structure (currently anonymous, need fix)

   extern ipqos_t ipqos;

   for(int i=0; ipqos[i].name != NULL && i < 255, i++) {
	settos(socket,ipqos[i].value);
	send_packet();
   }
   recv_confirmation();
  
*/ 



