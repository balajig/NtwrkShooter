LIB = libutil.a
OBJS = log.o 
CFLAGS = -g -Wall

all : $(LIB)

$(LIB) : $(OBJS)
	$(AR) r $@ $(OBJS)

clean :
	rm -f $(LIB) $(OBJS)

install :
