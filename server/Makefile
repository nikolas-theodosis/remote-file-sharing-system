SOURCE = dataServer.c lookup.c pooling.c queue.c
OBJS = dataServer.o lookup.o pooling.o queue.o
HEADER = dataServer.h queue.h
OUT = dataServer
CC = gcc
LDFLAGS = -pthread 
FLAGS = -c 

all: $(OBJS)
	$(CC) $(OBJS) -o $(OUT) $(LDFLAGS)

main.o : main.c
	$(CC) $(FLAGS) main.c

dataServer.o : dataServer.c
	$(CC) $(FLAGS) dataServer.c

lookup.o : lookup.c
	$(CC) $(FLAGS) lookup.c

pooling.o : pooling.c
	$(CC) $(FLAGS) pooling.c

queue.o : queue.c
	$(CC) $(FLAGS) queue.c

build : $(SOURCE)
	$(CC) -o $(OUT) $(SOURCE)

clean : 
	rm -f $(OBJS) $(OUT)

rebuild : clean build


