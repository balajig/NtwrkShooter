SUBDIRS = lib src
CFLAGS = -g -Wall
BIN_DIR = bin
NTS_BIN = ./src/nshooter
NTS_INSTALL_DIR = /bin/nshooter

all: 
	set -e ; for dir in $(SUBDIRS); do $(MAKE) -C $$dir $@; done
install:
	echo "<---------------------Installing Network Trouble shooter--------------------->"; \
	if test "$(USER)" != "root"; \
	then \
		echo "*************** YOU MUST BE ROOT TO INSTALL NETWORK TROUBLE SHOOTER :( ***************"; \
	else \
		chown root $(NTS_BIN) ; \
		chmod ug+s $(NTS_BIN) ; \
		chmod o-rx $(NTS_BIN) ; \
		install -C $(NTS_BIN) $(NTS_INSTALL_DIR) ; \
		echo "<------------------------ Installed successfully :) -------------------------->"; \
	fi
clean:
	rm -rf *~
	set -e ; for dir in $(SUBDIRS); do $(MAKE) -C $$dir $@; done
