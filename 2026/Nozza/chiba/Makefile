CC     = gcc
CFLAGS = -Wall -g -O3 -pthread
LDLIBS = -lm -lrt -pthread
EXEC   = chiba
OBJS   = main.o collector.o ring_buffer.o exporter.o

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

main.o:        main.c ring_buffer.h collector.h  exporter.h
collector.o:   collector.h ring_buffer.h ring_buffer.c
ring_buffer.o: ring_buffer.c ring_buffer.h
exporter.o:    exporter.h ring_buffer.h ring_buffer.c

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(EXEC)

.PHONY: all clean 