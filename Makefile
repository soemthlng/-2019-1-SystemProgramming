OBJS = Advanced_ls.c
CC = gcc
EXEC  = spls_advanced

all: $(OBJS)
	$(CC) -o $(EXEC) $^
clean:
	rm -rf $(EXEC)
