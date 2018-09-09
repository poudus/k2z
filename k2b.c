
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

//
// main
//
int main(int argc, char* argv[])
{
char	buffer_error[512], database_name[32], flipped_moves[128], root[128], buf[128];
PGconn *pgConn = NULL;
int	size = 12, min_count = 10, depth = 2;
BOOK_MOVE bmoves[1000];

	if (argc > 1 && strlen(argv[1]) == 5)
	{
		sprintf(database_name, "k%s", argv[1]);
		strcpy(buffer_error, argv[1]);
		buffer_error[2] = 0;
		size = atoi(buffer_error);
		//printf("\nsize         = %d\n", size);
		pgConn = pgOpenConn(database_name, "k2", "", buffer_error);
		//printf("database connection     = %p %s\n", pgConn, database_name);
		root[0] = 0;
		if (argc > 2)
			depth = atoi(argv[2]);
		if (argc > 3)
			min_count = atoi(argv[3]);
		if (argc > 4)
			strcpy(root, argv[4]);

		int nbi = 0, nbd = 0, len_root = strlen(root);
		bool bInsert = (len_root == 0);
		if (root[0] == '.') len_root = 0;
		int nb_book_moves = ComputeBookMoves(pgConn, &bmoves[0], size, min_count, depth);

		if (bInsert)
			printf("\n%d entries deleted, depth = %d\n", DeleteBookMoves(pgConn, depth), depth);
		else
			printf("++\n");
		for (int bm = 0 ; bm < nb_book_moves ; bm++)
		{
			if (bInsert && InsertBookMove(pgConn, depth, &bmoves[bm])) nbi++;
			else if (len_root == 0 || strncmp(root, bmoves[bm].key, len_root) == 0)
			{
				printf("%s.%s   %6.2f %%   %5d  %4d - %-4d\n",
					bmoves[bm].key, bmoves[bm].move, bmoves[bm].ratio,
					bmoves[bm].count, bmoves[bm].win, bmoves[bm].loss);
				nbd++;
			}
		}
		if (bInsert)
			printf("\n%d/%d inserted, depth = %d\n", nbi, nb_book_moves, depth);
		else
			printf("--\n%d book moves  (%d entries)\n\n", nbd, nb_book_moves);
	}
	else
	{
		printf("error.invalid-database-name\n");
		return -1;
	}
}

