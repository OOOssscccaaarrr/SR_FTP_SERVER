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
OBJS = utilitaire/csapp.o serveur/get.o
INCLDIR = -I. -Iutilitaire -Iserveur -Iclient

PROGS =  serveur/ftp_serv client/ftp_client



all: $(PROGS)


# Règle générique pour compiler les .o dans leur dossier d'origine
utilitaire/%.o: utilitaire/%.c $(INCLUDE)
	$(CC) $(CFLAGS) $(INCLDIR) -c -o $@ $<

serveur/%.o: serveur/%.c $(INCLUDE)
	$(CC) $(CFLAGS) $(INCLDIR) -c -o $@ $<

client/%.o: client/%.c $(INCLUDE)
	$(CC) $(CFLAGS) $(INCLDIR) -c -o $@ $<
	

# Règle pour lier les exécutables dans leur dossier
serveur/ftp_serv: serveur/ftp_serv.o $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $^ $(LIBS)

client/ftp_client: client/ftp_client.o $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $^ $(LIBS)
	
clean:
	rm -f $(PROGS) utilitaire/*.o serveur/*.o client/*.o
