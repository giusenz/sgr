CC     = gcc
CFLAGS = -Wall -g -O3 -pthread
LDLIBS = -lm -lrt -pthread
EXEC   = chiba
OBJS   = main.o collector.o

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

main.o:      main.c collector.h
collector.o: collector.c collector.h

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(EXEC)

.PHONY: all clean 