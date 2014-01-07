
#ifdef TIMERFD
timer
socket
while(select(
#endif

#ifdef USE_SELECT
#include <sys/select.h>

main_loop() {
struct timeval timeout;
fd_set exceptfds,readfds,writefds;
FD_ZERO(readfds);

timeout.tv_sec = 0;
timeout.tv_nsec = 10000;
int serial = 0;

while (serial[rfd] < end && cur_time < end_time) {
	ok = select(rfd,readfds,NULL,NULL,timeout);
	update_time();
	switch(ok) {
		case 0:	send_data(rfd); break;
		case -1: goto end; break;
		case default:   send_data(rfd);
				update_stats(rfd);
	}
	timeout.tv_sec = 0;
	timeout.tv_nsec = 10000;
}

end: 
}

	
#endif

