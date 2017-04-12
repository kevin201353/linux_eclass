CFLAGS=`pkg-config --cflags gtk+-3.0 gstreamer-1.0`
LIBS=`pkg-config --libs gtk+-3.0 gstreamer-1.0 libpulse libpulse-simple`

TIME=`date +"%Y-%m-%d %T"`
.INIT:
	@echo $(TIME)
	@echo "char buildtime[32]=\"$(TIME)\";" > buildtime.h

all: .INIT clean player

player:
	gcc -std=c99 -c decoder.c main.c recv_data.c audio_pulse.c  tcp_server.c tcp_client.c $(CFLAGS)
	gcc -std=c99 -o eclass_client decoder.o main.o recv_data.o audio_pulse.o tcp_server.o tcp_client.o -Wall $(LIBS) -lpthread 

clean:  
	rm -rf *.o eclass_client

