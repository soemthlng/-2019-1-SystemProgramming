OBJS = sinmple_ls.c
CC = gcc
EXEC  = simple_ls

all: $(OBJS)
	$(CC) -o $(EXEC) $^
clean:
	rm -rf $(EXEX)
