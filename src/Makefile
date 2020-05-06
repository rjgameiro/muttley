
CTL_NAME =		muttley
UTL_NAME =		kexutil

KEX_NAME =		muttley.kex
KEX_CTRL =		_muttley_ctrl

BUILD_ARCH =	64

CC =			xlc
LD =			ld

DEBUG = 		-g
CFLAGS =		-q$(BUILD_ARCH) -qlanglvl=ansi -qinfo=pro
CTL_LDFLAGS =   -brtl -bI:$(KEX_NAME).exp
KEX_CFLAGS =	-DPOWER -D_KERNEL
KEX_LDFLAGS =	-b$(BUILD_ARCH) -bI:/usr/lib/kernex.exp -lsys -lcsys

all:			$(KEX_NAME) $(CTL_NAME) 

$(CTL_NAME):	$(CTL_NAME).c $(UTL_NAME).o
				@echo "$@"
				$(CC) $(CFLAGS) $(CTL_LDFLAGS) -o $@ $(CTL_NAME).c $(UTL_NAME).o

$(UTL_NAME).o:	$(UTL_NAME).c
				@echo "$@"
				$(CC) $(CFLAGS) -o $@ -c $?

$(KEX_NAME):	$(KEX_NAME).c
				@echo "$@"
				$(CC) $(CFLAGS) $(KEX_CFLAGS) -o $(KEX_NAME).o -qlist -qsource -c $?
				$(LD) $(KEX_LDFLAGS) -o $@ $(KEX_NAME).o -e $(KEX_CTRL) -bE:$(KEX_NAME).exp

clean:
				rm -f *.o $(KEX_NAME) $(KEX_NAME).lst $(CTL_NAME)
