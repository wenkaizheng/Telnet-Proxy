all: cproxy sproxy
cproxy: cproxy.o network.o
	gcc -Wall -o cproxy cproxy.o network.o

sproxy: sproxy.o network.o
	gcc -Wall -o sproxy sproxy.o network.o

cproxy.o: cproxy.c
	gcc -Wall -c cproxy.c

sproxy.o: sproxy.c
	gcc -Wall -c sproxy.c
 
network.o: network.h network.c
	gcc -c network.c
.PHONY : clean 
clean : 
	rm -f *.o
	rm -f sproxy
	rm -f cproxy
