

k2z: k2z.o state.o board.o database.o
	gcc -lpq -L/usr/lib -o k2z k2z.o state.o board.o database.o -lpq
	
k2z.o: k2z.c board.h state.h
	gcc -I/usr/include/postgresql -c k2z.c
	
state.o: state.c state.h board.h
	gcc -c state.c
	
board.o: board.c board.h
	gcc -c board.c
	
database.o: database.c database.h
	gcc -I/usr/include/postgresql -c database.c
		
all: k2z

clean: 
	rm *.o
