
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

typedef struct
{
	int	id;
	char	moves[128];
	double	hr, vr;
	char	winner;
} GAME;

typedef struct
{
	char	key[16];
	int	count, win, loss;
	double	wr, lr, ratio, dr;
} POSITION;

static POSITION gposition[1000];

int LoadGames(PGconn *pgConn, GAME *pg)
{
char query[256], buf[16];

strcpy(query, "select g.id, g.winner, g.moves, ph.rating as hr, pv.rating as vr from k2s.game g, k2s.player ph, k2s.player pv where g.hp = ph.id and g.vp = pv.id");
	
	PGresult *pgres = pgQuery(pgConn, query);
	if (pgres != NULL)
	{
		int nb_games = PQntuples(pgres);
		if (nb_games > 0)
		{
			for (int ig = 0 ; ig < nb_games ; ig++)
			{
				pg->id = atoi(PQgetvalue(pgres, ig, 0));
				strcpy(pg->moves, PQgetvalue(pgres, ig, 2));
				strcpy(buf, PQgetvalue(pgres, ig, 1));
				if (strlen(buf) > 0)
					pg->winner = buf[0];
				else
					pg->winner = '?';

				pg->hr = atof(PQgetvalue(pgres, ig, 3));
				pg->hr = atof(PQgetvalue(pgres, ig, 4));

				pg++;
			}
		}
		PQclear(pgres);
		return nb_games;
	} else return -1;
}

int AvgH(PGconn *pgConn, int hp, int min_game_id, int *count)
{
char query[1024];
double avg = 0.0;

	sprintf(query, "select avg(ht), count(*) from k2s.game where hp = %d and id >= %d", hp, min_game_id);
	
	PGresult *pgres = pgQuery(pgConn, query);
	if (pgres != NULL && PQntuples(pgres) == 1)
	{
		avg = atoi(PQgetvalue(pgres, 0, 0));
		*count = atoi(PQgetvalue(pgres, 0, 1));
		PQclear(pgres);
		return avg;
	} else return -1;
}

int AvgV(PGconn *pgConn, int vp, int min_game_id, int *count)
{
char query[1024];
double avg = 0.0;

	sprintf(query, "select avg(vt), count(*) from k2s.game where vp = %d and id >= %d", vp, min_game_id);
	
	PGresult *pgres = pgQuery(pgConn, query);
	if (pgres != NULL && PQntuples(pgres) == 1)
	{
		avg = atoi(PQgetvalue(pgres, 0, 0));
		*count = atoi(PQgetvalue(pgres, 0, 1));
		PQclear(pgres);
		return avg;
	} else return -1;
}

int LastGame(PGconn *pgConn, long *ts)
{
char query[1024];
int game = 0;

	sprintf(query, "select id, ts from k2s.game where id = (select max(id) from k2s.game)");
	
	PGresult *pgres = pgQuery(pgConn, query);
	if (pgres != NULL && PQntuples(pgres) == 1)
	{
		game = atoi(PQgetvalue(pgres, 0, 0));
		*ts = atol(PQgetvalue(pgres, 0, 1));
		PQclear(pgres);
		return game;
	} else return -1;
}

static char alfb[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

bool FlipSize(char *x, int size)
{
	if (*x -'A' >= size/2)
	{
		*x = alfb[size - *x + 'A' - 1];
		return true;
	} else return false;
}

char Flip(int size, char c, bool f)
{
	if (f) c = alfb['A' + size - c - 1];
	return c;
}


char *FlipMoves(int size, char *moves, int depth)
{
//printf("flip moves = %s\n", moves);
bool	hf = FlipSize(&moves[0], size);
bool	vf = FlipSize(&moves[1], size);
//printf("flip moves = %s  hf = %d  vf = %d\n", moves, hf, vf);

	for (int d = 1 ; d < depth ; d++)
	{
		moves[2*d]= Flip(size, moves[2*d], hf);
		moves[2*d+1]= Flip(size, moves[2*d+1], vf);
	}
	moves[2*depth] = 0;
	return moves;
}

int AddPosition(char *key, char trait, char winner, double hr, double vr, int nb_positions)
{
bool bfound = false;
int ip = 0, ifound = 0;

	while (ip < nb_positions && !bfound)
	{
		if (strcmp(key, gposition[ip].key) == 0)
		{
			ifound = ip;
			bfound = true;
		}
		ip++;
	}
	if (!bfound)
	{
		strcpy(gposition[nb_positions].key, key);
		gposition[nb_positions].count = 0;
		gposition[nb_positions].win = 0;
		gposition[nb_positions].loss = 0;
		gposition[nb_positions].wr = 0;
		gposition[nb_positions].lr = 0;
		ifound = nb_positions;
		nb_positions++;
	}
	if (ifound >= 0 && ifound < nb_positions)
	{
		gposition[ifound].count++;
		if (winner == 'H')
		{
			gposition[ifound].wr += hr;
			gposition[ifound].lr += vr;
		}
		else
		{
			gposition[ifound].wr += vr;
			gposition[ifound].lr += hr;
		}
		if (trait == winner)
			gposition[ifound].win++;
		else
			gposition[ifound].loss++;
	}
	return nb_positions;
}

int cmppos(const void *p1, const void *p2)
{
	if (((POSITION*)p2)->dr > ((POSITION*)p1)->dr)
		return 1;
	else
		return -1;
}

//
// main
//
int main(int argc, char* argv[])
{
char	buffer_error[512], database_name[32], flipped_moves[128], buf[128];
PGconn *pgConn = NULL;
GAME	games[1280];
int	depth = 2, size = 12, nb_positions = 0, min_count = 5;

	if (argc > 1 && strlen(argv[1]) == 5)
	{
		sprintf(database_name, "k%s", argv[1]);
		strcpy(buffer_error, argv[1]);
		buffer_error[2] = 0;
		size = atoi(buffer_error);
		printf("\nsize         = %d\n", size);
		pgConn = pgOpenConn(database_name, "k2", "", buffer_error);
		//printf("\ndatabase connection = %p %s\n", pgConn, database_name);
		//printf("\npid  d/mm   rating   avgmd   games    win   loss\n--------------------------------------------------\n");
		int nb_games = LoadGames(pgConn, &games[0]);
		printf("nb_games     = %d\n", nb_games);
		if (argc > 2)
			depth = atoi(argv[2]);
		if (argc > 3)
			min_count = atoi(argv[3]);
		for (int ig = 0 ; ig < nb_games ; ig++)
		{
			if (strlen(games[ig].moves) > 2 * depth)
			{
				strncpy(buf, games[ig].moves, 2 * depth);
				buf[2 * depth] = 0;
				strcpy(flipped_moves, FlipMoves(size, buf, depth));
				//printf("'%s' : [%s]  %s\n", flipped_moves, buf, games[ig].moves);
				nb_positions = AddPosition(flipped_moves, (strlen(flipped_moves)/2) % 2 == 0 ? 'H' : 'V',
								games[ig].winner, games[ig].hr, games[ig].vr, nb_positions);
			}
//if (ig == 5) break;

		}
		printf("nb_positions = %d\n\n", nb_positions);
		for (int ip = 0 ; ip < nb_positions ; ip++)
		{
			if (gposition[ip].win > 0)
				gposition[ip].wr /= gposition[ip].win;
			else
				gposition[ip].wr = 0.0;

			if (gposition[ip].loss > 0)
				gposition[ip].lr /= gposition[ip].loss;
			else
				gposition[ip].lr = 0.0;
			gposition[ip].ratio = 100.0 * gposition[ip].win / gposition[ip].count;
			gposition[ip].dr = gposition[ip].ratio - 100.0 * EloExpectedResult(gposition[ip].wr, gposition[ip].lr);
		}
		qsort(&gposition[0], nb_positions, sizeof(POSITION), cmppos);
		int nb_keys = 0;
		for (int ip = 0 ; ip < nb_positions ; ip++)
		{
			if (gposition[ip].count >= min_count)
			{
				printf("%s   %6.2f %%   %3d - %-3d   [ %6.2f ]\n", gposition[ip].key, gposition[ip].ratio,
					gposition[ip].win, gposition[ip].loss, gposition[ip].dr);
				nb_keys++;
			}
		}
		printf("\n%d entries\n\n", nb_keys);
	}
	else
	{
		printf("error.invalid-database-name\n");
		return -1;
	}
}

