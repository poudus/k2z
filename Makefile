

k2z: k2z.o state.o board.o database.o
	gcc -lpq -L/usr/lib -o k2z k2z.o state.o board.o database.o -lpq -lm

k2s: k2s.o database.o
	gcc -lpq -L/usr/lib -o k2s k2s.o database.o -lpq -lm

k2b: k2b.o database.o
	gcc -lpq -L/usr/lib -o k2b k2b.o database.o -lpq -lm
	
k2z.o: k2z.c board.h state.h
	gcc -I/usr/include/postgresql -c k2z.c
	
state.o: state.c state.h board.h
	gcc -c state.c
	
board.o: board.c board.h
	gcc -c board.c
	
database.o: database.c database.h
	gcc -I/usr/include/postgresql -c database.c
		
k2s.o: k2s.c database.h
	gcc -I/usr/include/postgresql -c k2s.c
		
k2b.o: k2b.c database.h
	gcc -I/usr/include/postgresql -c k2b.c
	
all: k2z k2s k2b

clean: 
	rm *.o
