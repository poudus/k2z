
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <sys/time.h>

#include "board.h"
#include "state.h"
#include "database.h"
#include "book.h"


typedef struct
{
char	key[32];
int	count, win, loss;
} BOOK_KEY;


int select_depth_keys(PGconn *pgConn, int depth, BOOK_KEY* bkeys)
{
int nbc = 0;
char query[256];

	sprintf(query, "select distinct(key), sum(count), sum(win), sum(loss) from k2s.book where depth = %d group by key", depth);
	PGresult *pgres = pgQuery(pgConn, query);
	if (pgres != NULL)
	{
		int count = PQntuples(pgres);
		for (int i = 0 ; i < count ; i++)
		{
			strcpy(bkeys[i].key, PQgetvalue(pgres, i, 0));
			bkeys[i].count = atoi(PQgetvalue(pgres, i, 1));
			bkeys[i].win = atoi(PQgetvalue(pgres, i, 2));
			bkeys[i].loss = atoi(PQgetvalue(pgres, i, 3));
		}

		PQclear(pgres);
        	return count;
	}
}

//
// main
//
int main(int argc, char* argv[])
{
char	buffer_error[512], database_name[32], moves[128], filter[128];
PGconn *pgConn = NULL;
int	depth = 2;
BOOK_KEY	bkeys[1024];
BOOK_MOVE	book_moves[256];

	if (argc > 1 && strlen(argv[1]) == 5 && argc > 2)
	{
		sprintf(database_name, "k%s", argv[1]);
		pgConn = pgOpenConn(database_name, "k2", "", buffer_error);
		depth = atoi(argv[2]);

		filter[0] = 0;
		if (argc > 3)
			strcpy(filter, argv[3]);
		int len_filter = strlen(filter);

		int nb_wins = 0, nb_bkeys = select_depth_keys(pgConn, depth, &bkeys[0]);

		printf("\n%d keys\n\n", nb_bkeys);
		for (int bkey = 0 ; bkey < nb_bkeys ; bkey++)
		{
			int key_move_ratios = select_move_ratios(pgConn, depth, bkeys[bkey].key, &book_moves[0]);
			if (book_moves[0].ratio < 0.0 && (len_filter == 0 || strncmp(bkeys[bkey].key, filter, len_filter) == 0))
			{
				moves[0] = 0;
				for (int im = 1 ; im < key_move_ratios ; im++)
				{
					strcat(moves, book_moves[im].move);
					strcat(moves, " ");
				}
				printf("%s:  %6.2f %%  %6.2f %%  [%-4d+%-4d-%-4d]  %s   %s\n", bkeys[bkey].key,
						book_moves[0].ratio, book_moves[0].deep_ratio, bkeys[bkey].count, bkeys[bkey].win, bkeys[bkey].loss,
						book_moves[0].move, moves);
				nb_wins++;
			}
		}
		printf("\n%d wins\n\n", nb_wins);
	}
	else
	{
		printf("error.invalid-database-name\n");
		return -1;
	}
}

