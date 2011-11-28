OBJS = if.o ntwkr_main.o
BIN = ntwkr
INCLUDES =../lib 
CFLAGS ?= -g -Wall -I$(INCLUDES) 
LIBS= ../lib/libutil.a -lpthread -lreadline 

all : $(BIN)

$(BIN) : $(OBJS)
	$(CC) $(CFLAGS) -o $(BIN) $(OBJS) $(LIBS)

clean : 
	rm -f $(BIN) $(OBJS) *~

install : $(BIN)
	install -d $(DESTDIR)$(BIN_DIR)
	install -s $(BIN) $(DESTDIR)$(BIN_DIR)
