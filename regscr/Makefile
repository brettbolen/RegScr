#####################################################################
#
# Register Script Makefile
#
#
#####################################################################
CCC = arm-g++
CFLAGS = -Wall -Werror -g


local:  regscr

all: regscr

regscr : regscr.cc
	g++ -o $@ $<

arm_regscr : regscr.cc
	$(CCC) -o $@ $<

clean:
	- rm -f *.o regscr
	- rm  *~ scripts/*~

install:  regscr
	sudo cp regscr     $(NFS_INSTALL_DIR)/usr/bin/regscr
	sudo cp regscr.dat $(NFS_INSTALL_DIR)/root
	sudo chmod a+x     scripts/*
	sudo cp scripts/*  $(NFS_INSTALL_DIR)/root
	sudo cp regscr.cc  $(NFS_INSTALL_DIR)/root/test
	
