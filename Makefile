SUBDIRS = lib src
CFLAGS = -g -Wall
BIN_DIR = bin

all install: 
	set -e ; for dir in $(SUBDIRS); do $(MAKE) -C $$dir $@; done
clean:
	rm -rf *~
	set -e ; for dir in $(SUBDIRS); do $(MAKE) -C $$dir $@; done
