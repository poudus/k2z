
#include <libpq-fe.h>


typedef struct
{
	int	id;
	char	moves[128];
	double	hr, vr;
	char	winner;
} GAME;

typedef struct
{
	char	key[32];
	int	count, win, loss;
	double	hr, vr, trait_ratio, move_ratio, dratio, elodiff, dexpr;
} POSITION;


typedef struct
{
	char	key[12];
	char	move[4];
	int	count, win, loss;
	double	ratio;
} BOOK_MOVE;


int ComputeBookMoves(PGconn *pgConn, BOOK_MOVE *book_moves, int size, int min_count, int depth);
bool InsertBookMove(PGconn *pgConn, int depth, BOOK_MOVE *bm);
int DeleteBookMoves(PGconn *pgConn, int depth);
int ListBookMoves(PGconn *pgConn, int size, char *moves, BOOK_MOVE *bm);



