OBJS = if.o ntwrk_main.o rt_info_nl.o ping.o ping_common.o resolve.o
BIN = nshooter
INCLUDES =../lib 
CFLAGS ?= -g -Wall -I$(INCLUDES) 
LIBS= ../lib/libutil.a

all : $(BIN)

$(BIN) : $(OBJS)
	$(CC) $(CFLAGS) -o $(BIN) $(OBJS) $(LIBS)

clean : 
	rm -f $(BIN) $(OBJS) *~

install : $(BIN)
	install -d $(DESTDIR)$(BIN_DIR)
	install -s $(BIN) $(DESTDIR)$(BIN_DIR)
