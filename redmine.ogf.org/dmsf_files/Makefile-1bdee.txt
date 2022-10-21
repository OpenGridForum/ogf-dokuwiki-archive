example: example.o
	gcc -o example example.o -L/usr/local/60b2/lib/darwin -ldrmaa -lm -lpthread -ldl 
example.o:
	gcc -c example.c -I/usr/local/60b2/include

clean:
	rm -f example.o example
