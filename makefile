xcursorgen-conf: xcgc.c
	gcc -Wall xcgc.c -o xcursorgen-conf -lpng -lz

run: xcursorgen-conf
	./xcursorgen-conf

clean: xcgc.o
	rm xcgc.o

install: xcursorgen-conf
	sudo cp xcursorgen-conf /usr/bin/xcursorgen-conf
	sudo cp man/xcursorgen-conf.1 /usr/local/man/man1/xcursorgen-conf.1
	sudo gzip /usr/local/man/man1/xcursorgen-conf.1
