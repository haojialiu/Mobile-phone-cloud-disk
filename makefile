all:ser

ser:ser.o	work_thread.o
	gcc -o ser ser.o work_thread.o -lpthread

ser.o:ser.c
	gcc -c ser.c

work_thread.o:work_thread.c
	gcc -c work_thread.c
clean:
	rm -f *.o ser
