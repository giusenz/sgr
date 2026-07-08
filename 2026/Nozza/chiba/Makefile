CC     = gcc
CFLAGS = -Wall -g -O3 -pthread
LDLIBS = -lm -lrt -pthread -lcurl
EXEC   = chiba
OBJS   = main.o wrappers.o collector.o ring_buffer.o exporter.o 

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

main.o:        main.c exporter.h wrappers.h ring_buffer.h collector.h 
wrappers.o:    wrappers.c wrappers.h
collector.o:   collector.c collector.h ring_buffer.h 
ring_buffer.o: ring_buffer.c ring_buffer.h wrappers.h
exporter.o:    exporter.c exporter.h ring_buffer.h

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(EXEC)

.PHONY: all clean 