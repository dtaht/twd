/* Test survival of all tos marked bits

   Might be able to assemble this with sendmsg.

   for(int i = 0; i < 64; i++) {
	settos(socket,i<<2);
	send_packet();
   }
   recv_confirmation();
  
*/ 



