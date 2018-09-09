

k2z: k2z.o state.o board.o database.o
	gcc -lpq -L/usr/lib -o k2z k2z.o state.o board.o database.o -lpq -lm

k2s: k2s.o database.o
	gcc -lpq -L/usr/lib -o k2s k2s.o database.o -lpq -lm

k2b: k2b.o database.o book.o
	gcc -lpq -L/usr/lib -o k2b k2b.o database.o book.o -lpq -lm
	
k2z.o: k2z.c board.h state.h
	gcc -I/usr/include/postgresql -c -O3 k2z.c
	
state.o: state.c state.h board.h
	gcc -c -O3 state.c
	
board.o: board.c board.h
	gcc -c -O3 board.c
	
database.o: database.c database.h
	gcc -I/usr/include/postgresql -c -O3 database.c
	
book.o: book.c book.h database.h
	gcc -I/usr/include/postgresql -c -O3 book.c
		
k2s.o: k2s.c database.h
	gcc -I/usr/include/postgresql -c k2s.c
		
k2b.o: k2b.c database.h book.h
	gcc -I/usr/include/postgresql -c k2b.c
	
all: k2z k2s k2b

clean: 
	rm *.o
