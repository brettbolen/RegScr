#####################################################################
#
# Register Script Makefile
#
#
#####################################################################
CC_PREFIX = arm-none-linux-gnueabi
CFLAGS = -Wall -Werror -g

TEST_DIR = ../test

local:  regscr

all: regscr

regscr : regscr.cc
	g++ -o $@ $<

arm_regscr : regscr.cc
	$(CC_PREFIX)-g++ -o $@ $<

clean:
	- rm -f *.o regscr
	- rm  *~ scripts/*~

install:  regscr
	sudo cp regscr     $(TEST_DIR)
