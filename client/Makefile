SOURCE = remoteClient.c 
OBJS = remoteClient.o 
HEADER = remoteClient.h
OUT = remoteClient
CC = gcc
FLAGS = -c

all: $(OBJS)
	$(CC) $(OBJS) -o $(OUT)

remoteClient.o : remoteClient.c
	$(CC) $(FLAGS) remoteClient.c

build : $(SOURCE)
	$(CC) -o $(OUT) $(SOURCE)

clean : 
	rm -f $(OBJS) $(OUT)

rebuild : clean build


