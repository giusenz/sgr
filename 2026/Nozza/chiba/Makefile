CC     = gcc
CFLAGS = -Wall -g -O3 -pthread
LDLIBS = -lm -lrt -pthread
EXEC   = chiba
OBJS   = main.o collector.o ring_buffer.o

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

main.o:        main.c ring_buffer.h collector.h
collector.o:   collector.h ring_buffer.h ring_buffer.c
ring_buffer.o: ring_buffer.c ring_buffer.h

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(EXEC)

.PHONY: all clean 