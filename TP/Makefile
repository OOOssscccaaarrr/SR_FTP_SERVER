.PHONY: all, clean

# Disable implicit rules
.SUFFIXES:

VPATH = client serveur utilitaire
# Keep intermediate files
#.PRECIOUS: %.o

CC = gcc
CFLAGS = -Wall -Werror
LDFLAGS =

# Note: -lnsl does not seem to work on Mac OS but will
# probably be necessary on Solaris for linking network-related functions 
#LIBS += -lsocket -lnsl -lrt
LIBS += -lpthread

INCLUDE = csapp.h structures.h
OBJS = csapp.o get.o structures.o
INCLDIR = -I. -Iutilitaire -Iserveur -Iclient

PROGS =  serveur/ftp_serv client/ftp_client


all: $(PROGS)

%.o: %.c $(INCLUDE)
	$(CC) $(CFLAGS) $(INCLDIR) -c -o $@ $<
	
%: %.o $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $^ $(LIBS)
#	$(CC) -o $@ $(LDFLAGS) $(LIBS) $^
	
clean:
	rm -f $(PROGS) *.o
