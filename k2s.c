
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
	int id, depth, max_moves, games, win, loss;
	char label[16];
	double rating, avg, des;
} PLAYER;

int LoadPlayers(PGconn *pgConn, PLAYER *pp)
{
char query[256];

	strcpy(query, "select id, max_moves, depth, rating, win, loss from k2s.player order by rating desc");
	
	PGresult *pgres = pgQuery(pgConn, query);
	if (pgres != NULL)
	{
		int nb_players = PQntuples(pgres);
		if (nb_players > 0)
		{
			//int ip = 0;
			for (int ip = 0 ; ip < nb_players ; ip++)
			{
				pp->id = atoi(PQgetvalue(pgres, ip, 0));
				pp->rating = atof(PQgetvalue(pgres, ip, 3));
				pp->max_moves = atoi(PQgetvalue(pgres, ip, 1));
				pp->depth = atoi(PQgetvalue(pgres, ip, 2));
				pp->win = atoi(PQgetvalue(pgres, ip, 4));
				pp->loss = atoi(PQgetvalue(pgres, ip, 5));
				pp->avg = 0.0;
				sprintf(pp->label, "%1d/%-2d", pp->depth, pp->max_moves);
				pp++;
			}
		}
		PQclear(pgres);
		return nb_players;
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

int cmp_player_avg(const void *p1, const void *p2)
{
	if (((PLAYER*)p2)->avg > ((PLAYER*)p1)->avg)
		return 1;
	else
		return -1;
}

//
// main
//
int main(int argc, char* argv[])
{
char buffer_error[512], database_name[32];
PGconn *pgConn = NULL;
PLAYER players[32];
int min_game_id = 0;
bool	byavgmd = false;

	if (argc > 1 && strlen(argv[1]) == 5)
	{
		sprintf(database_name, "k%s", argv[1]);
		pgConn = pgOpenConn(database_name, "k2", "", buffer_error);
		//printf("database connection     = %p %s\n", pgConn, database_name);
		printf("\npid  d/mm   rating   avgmd   games    win   loss     e/s\n--------------------------------------------------------\n");
		int nb_players = LoadPlayers(pgConn, &players[0]);
		if (argc > 2)
		{
			if (strcmp(argv[2], "-avgmd") == 0)
				byavgmd = true;
			else
				min_game_id = atoi(argv[2]);
		}
		for (int ip = 0 ; ip < nb_players ; ip++)
		{
			int ch = 0, cv = 0;
			int avgh = AvgH(pgConn, players[ip].id, min_game_id, &ch);
			int avgv = AvgV(pgConn, players[ip].id, min_game_id, &cv);
			double avg = 0.0;
			if (ch + cv > 0)
				avg = ((ch * avgh) + (cv * avgv)) / (ch + cv);
			players[ip].avg = avg;
			players[ip].games = ch + cv;
			players[ip].des = 0.0;
		}
		if (byavgmd)
		{
			qsort(players, nb_players, sizeof(PLAYER), cmp_player_avg);
			for (int ip = 0 ; ip < nb_players ; ip++)
			{
				if (ip < nb_players -1)
					players[ip].des = 1000.0 * (players[ip].rating - players[ip+1].rating) / (players[ip].avg - players[ip+1].avg);
				printf("%3d %5s %8.2f  %6.1f  %6d  %5d - %-5d   %4d\n", players[ip].id, players[ip].label,
					players[ip].rating, players[ip].avg / 1000.0, players[ip].games, players[ip].win, players[ip].loss, (int)players[ip].des);
			}
		}
		else
		{
			for (int ip = 0 ; ip < nb_players ; ip++)
			{
				printf("%3d %5s %8.2f  %6.1f  %6d  %5d - %-5d\n", players[ip].id, players[ip].label,
					players[ip].rating, players[ip].avg / 1000.0, players[ip].games, players[ip].win, players[ip].loss);
			}
		}
		long ts;
		int last_game_id = LastGame(pgConn, &ts);
		printf("\nlast game : %6d   %8ld  %06ld\n", last_game_id, ts/1000000, ts%1000000);
		int count = pgGetCount(pgConn, "k2s.game", "");
		printf("\ngame count: %6d\n\n", count);
	}
	else
	{
		printf("error.invalid-database-name\n");
		return -1;
	}
}

