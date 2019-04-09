OBJS = ls2.c
CC = gcc
EXEC  = test

all: $(OBJS)
	$(CC) -o $(EXEC) $^
clean:
	rm -rf $(EXEC)
