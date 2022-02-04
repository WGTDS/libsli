####################
# C Compiler = GCC #
####################
CC=gcc

##################
# Compiler Flags #
##################
CFLAGS=-ansi -Wall -Wextra -pedantic -pedantic-errors

######################
# Optimization Level #
######################
OLEVEL=-O3

#######################
# Extra Optimizations #
#######################
OEXTRA=-fexpensive-optimizations -flto

####################################
# The following are subdirectories #
# of the current work directory.   #
####################################

###############################
# <Compiled Binary Directory> #
###############################
BDIR=bin

########################################
# <Header Include Directory>           #
# The "-I" flags specifies searching   #
# the immediate work directory for the #
# specified subdirectory.              #
########################################
IDIR=-Iinclude

##########################################
# <Compiled, External Library Directory> #
##########################################
LDIR=lib

####################################
# <Compiled Object Code Directory> #
####################################
ODIR=obj

_LIBOBJ=dszpmio.o dszpsmsr.o dszpyay.o dszsyaz.o\
        eszpmio.o eszpsmsr.o eszpyay.o eszsyaz.o\
        process.o search.o utils.o
LIBOBJ=$(patsubst %,$(ODIR)/%,$(_LIBOBJ))

_BINOBJ=lzsz.o
BINOBJ=$(patsubst %,$(ODIR)/%,$(_BINOBJ))

_OBJ=$(_LIBOBJ) $(_BINOBJ)
OBJ=$(patsubst %,$(ODIR)/%,$(_OBJ))

####################################
# Executed by simply typing "make" #
####################################
all: $(LDIR)/libsli.a $(BDIR)/lzsz

$(ODIR)/%.o: src/%.c
	$(CC) $(CFLAGS) $(OLEVEL) $(IDIR) -c -o $@ $<

####################################################
# Can directly call by typing "make lib/libsli.a". #
####################################################
$(LDIR)/libsli.a: $(LIBOBJ)
	ar -rcs $@ $^

################################################
# Can directly call by typing "make bin/lzsz", #
# however, "libsli.a" is a prerequisite.       #
################################################
$(BDIR)/lzsz: $(BINOBJ)
	$(CC) $(CFLAGS) $(OEXTRA) $(OLEVEL) -s\
    -o $(BDIR)/lzsz $(ODIR)/lzsz.o $(LDIR)/libsli.a

.PHONY: clean

clean:
	rm -rf $(BDIR)/* $(LDIR)/* $(ODIR)/* 
