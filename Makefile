#IDIR =../../../../libmodbus/src
IDIR=../libmodbus/src
LDIR=/usr/local/lib


TARGET=wago
CC=gcc
#CFLAGS=-I$(IDIR) -L$(LDIR) -g -std=gnu99
CFLAGS=-I/usr/local/include -L/usr/local/lib -g 

.PHONY: default all clean check cron

default: $(TARGET)
all: default

SRC_C=wago.c \
     main.c
	 
HDR=wago.h \
    main.h \
    typedefs.h 

LIBS=-lpthread -lmodbus -lmicrohttpd 

#DEPS = $(patsubst %,$(IDIR)/%,$(HDR))
OBJ=$(patsubst %.c,%.o,$(SRC_C))

%.o: %.c $(HDR)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)
	
check:
	@echo '#############################'
	@echo ' SRC_C  = $(SRC_C)           '
	@echo ' OBJ    = $(OBJ)             '
	@echo ' HDR    = $(HDR)             '
	@echo '#############################'
	
cronjobstart:
	crontab -u ${USER} cronjob.txt

cronjobstop:
	crontab -u ${USER} -r

clean:
	rm -f *.o $(TARGET) 
