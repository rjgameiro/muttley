# muttley

Found this code (which I've written for fun in 2010) on a very old backup, getting it on github for posterity.

MUTTLEY is a 'device watch dog' AIX kernel extension.

It performs 'monitoring' i.e. opens a device (or file) and tries to read 512
bytes from it. On failure to read from the device muttley either logs a
message to the console, or 'panics' the server.

MUTTLEY was created has a proof of concept. In fact, IBM HACMP and also
PowerHA (at least 'till version 6.1 SP3) didn't recover from loss of the rootvg.
In a case where all the fibre-channel HBAs on a PowerHA node which is booting
from SAN fail, even with disk heartbeating properly setup - that node will be 
in a sort of a 'zombie' state and won't be able to take any action. Having 
MUTTLEY running and active will cause a kernel panic, bringing the node down.

This is TOTALLY A PROOF OF CONCEPT test against AIX 6.1, useage in production
environments is discouraged and totally at your own risk.

MUTTLEY is very configurable. In short, the default is configured behaviour
will be performed if:
- the number of failed runs, exceeds 'runs' threshold;
- a run is made of a series os 'checks' and it will be considered failed
if there aren't at least 'success' successfull tests.
- a run will be executed every 'run_int' interval	
The default configured behaviour is to JUST LOG TO THE CONSOLE (ONCE).
See the last example to use muttley in active mode (i.e. panic the system).

Some usage examples (must be executed on the same directory where the kernel 
extension is located):

GETTING HELP

	# ./muttley -h

	muttley is a kernel device monitoring and watch dog extension allowing\n"
    	load, status, start, display, stop and unload actions to be performed.\n"
    	The extension monitors a device and (optionally) forces a kernel panic\n"
    	and dump, upon loss of access to that device.\n"
    	The monitored device will usually be a raw hard disk or logical volume."

	usage:
	  muttley -h
    	  muttley load
	  muttley status
	  muttley [-d device] [-c checks] [-s success] [-r runs]
	  muttley [-i run_int] [-b behaviour] [-f] start
	  muttley [-i disp_int] [-t times] display
	  muttley stop
	  muttley unload
	  
	options:
	  -h             displays this help message
	  -d device      path to device to monitor (default /dev/rhd4)
	  -c checks      number of checks to perform on each run (default 3)
	  -s success     number of successful checks per run to consider the
	                 device as available (default 1)
	  -r runs        consecutive failed run threshold to execute the defined
	                 behaviour (default 2)
	  -i run_int     interval between check runs in seconds (default 5)
	  -b behaviour   behaviour on monitoring failure - none, panic
	                 (default none)
	  -f             allow 'device' to be any type of file, if using a file
	                 it must be at least 512 bytes in size (useful for
	                 test purposes, i.e. with a file which is removable)
	  -v disp_int    statistics display interval in seconds (default 2)
	  -t times       number of times the statistics will be display
	                 (default 1)
	arguments:
	  load           loads the kernel extension
	  status         query kernel extension status
	  start          starts monitoring (and executes the defined action on
	                 threshold)
	  display        display monitoring statistics
	  stop           stops monitoring
	  unload         unloads the kernel extension
	
	Notes:
	The muttley command line controller must be executed in the same
        same directory where the 'muttley.kex' kernel extension is located.

	DISCLAIMER
	All sorts of strange and random 'features' may develop in
	your system by the simple though of using this program.

LOAD THE MUTTLEY KERNEL EXTENSION

	# ./muttley load  
	muttley successfully loaded with kmid = 0x50a25000

START THE MUTTLEY KERNEL PROCESS WITH DEFAULTS (see help) 

	# ./muttley start 
	muttley started on '/dev/rhd4'

STOPPING THE MUTTLEY KERNEL PROCESS 

	# ./muttley stop
	stopping muttley... success

UNLOADING MUTTLEY

	# ./muttley unload
	muttley was successfully unloaded

VIEWING STATISTICS

	# ./muttley display
		or
	# ./muttley -v 5 -t 10 display
	
FOR TEST PURPOSES, MUTTLEY CAN BE RUN ON A FILE INSTEAD OF A DEVICE:
NOTE THAT THE FILE MUST BE AT LEAST 512 BYTES IN SIZE.
	
	# ./muttley -d /tmp/muttley.file -f start
	muttley started on '/tmp/muttley.file'

RUNNING MUTTLEY IN ACTIVE MODE (I.E. PANIC ON FAILURE TO MONITOR)

	# ./muttley -b panic start

Anyway... read the help page.

Regards, and hope this is at least usefull has an example.

Ricardo Gameiro
