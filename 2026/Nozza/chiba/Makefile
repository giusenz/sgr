CC     = gcc
CFLAGS = -Wall -g -O3 -pthread
LDLIBS = -lm -lrt -pthread
EXEC   = chiba
OBJS   = main.o collector.o

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

main.o:       
collector.o: c

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(EXEC)

.PHONY: all clean 