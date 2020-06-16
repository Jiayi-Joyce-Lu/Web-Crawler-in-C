crawler: functions.o crawler.o
	gcc -g -o crawler functions.o crawler.o
	
functions.o: functions.c functions.h
	gcc -c functions.c
	
crawler.o: crawler.c
	gcc -c crawler.c

clean:
	rm -f function.o crawler.o
