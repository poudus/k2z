
#include <libpq-fe.h>


typedef struct
{
	int    id;
	char   moves[128];
	double hr, vr;
	char   winner;
} GAME;

typedef struct
{
	char   key[32];
	int    count, win, loss, rcount;
	double hr, vr, trait_ratio, move_ratio, dratio, elodiff, dexpr;
} POSITION;


typedef struct
{
	char   key[32];
	char   move[4];
	int    count, win, loss;
	double ratio, deep_ratio;
} BOOK_MOVE;



int ComputeBookMoves(PGconn *pgConn, BOOK_MOVE *book_moves, int size, int min_count, int depth, double min_elo_sum);
bool InsertBookMove(PGconn *pgConn, int depth, BOOK_MOVE *bm);
int DeleteBookMoves(PGconn *pgConn, int depth);
int ListBookMoves(PGconn *pgConn, int size, char *moves, BOOK_MOVE *bm, bool deep);
void CheckBook(PGconn *pgConn);

bool chk_dup_move(char *moves, char *move, int *id1, int *id2);
bool UpdateDeepRatio(PGconn *pgConn, int depth, char *key, char *move, double deep_ratio);
int select_move_ratios(PGconn *pgConn, int depth, char* key, BOOK_MOVE *bm);
int BuildParentBookMoves(PGconn *pgConn, int depth, BOOK_MOVE *bm);



