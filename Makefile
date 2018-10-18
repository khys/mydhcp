mydhcpc: mydhcpc.o subr.o
	gcc -o mydhcpc mydhcpc.o subr.o

mydhcps: mydhcps.o subr.o
	gcc -o mydhcps mydhcps.o subr.o

mydhcpc.o: mydhcpc.c mydhcp.h
	gcc -c mydhcpc.c

mydhcps.o: mydhcps.c mydhcp.h
	gcc -c mydhcps.c

subr.o: subr.c mydhcp.h
	gcc -c subr.c

clean:
	\rm mydhcpc mydhcps *.o
