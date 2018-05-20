

k2z: k2z.o state.o board.o
	gcc -o k2z k2z.o state.o board.o
	
k2z.o: k2z.c board.h state.h
	gcc -c k2z.c
	
state.o: state.c state.h board.h
	gcc -c state.c
	
board.o: board.c board.h
	gcc -c board.c
		
all: k2z

clean: 
	rm *.o
