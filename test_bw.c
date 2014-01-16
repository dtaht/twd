/* Some thoughts on testing bandwidth.

	Since on the receiver side we are hopefully doing
	per packet timestamping right off the device's ringbuffer...
	
	after sending a burst of packets (say, 100 1000 byte packets)
	as fast as possible, by measuring interpacket arrival time,
	you can get a grip on the bandwidth available through the 
	bottleneck link.

	That works on isochronous mediums without packet loss, or
	weird token bucket shaping. We proved we can get a clock
	resolution in userspace accurate to ~300ns on modern x86 hardware
	and with it done in kernelspace vi SOTIMESTAMPNS perhaps 20ns. 

	having control of the hardware we use adding kernel timestamping
	support is not much of a problem, but it would be nice to also
	be able to run out of pure userspace.
	
	a 1500 byte packet takes

	1Mbit: 13ms
	10   : 1.3ms
        100  : .130ms
        1000 : .013ms 
	10000: 1300ns 

        So it seems feasible to get measurements past a gigabit.

	Packet loss makes it a bit harder, so you can compare what
	timestamps you got and their interpacket departure time, 
	smooth, and maybe get a result, but the right answer there
	is to back off on the burst size until you stop getting delay
	and packet loss. See TCP relentless.

	Then there is request/grant structures like cable, which have
	a 2ms-6ms request/grant interval for a bag of bytes. GPON is
	similar.

	then there are packet aggregation types, like wifi, that can
	bundle up a batch of 32k to 64k worth of "stuff" and ship that
	all in a bunch. You should be able to see aggregation and 
	grant/request patterns in the data.

*/
