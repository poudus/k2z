
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
	int id, depth, max_moves, games;
	char label[16];
	double rating, avg;
} PLAYER;

int LoadPlayers(PGconn *pgConn, PLAYER *pp)
{
char query[256];

	strcpy(query, "select id, max_moves, depth, rating from k2s.player order by rating desc");
	
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
				pp->avg = 0.0;
				sprintf(pp->label, "%d/%d", pp->depth, pp->max_moves);
				pp++;
			}
		}
		PQclear(pgres);
		return nb_players;
	} else return -1;
}

int AvgH(PGconn *pgConn, int hp, int *count)
{
char query[1024];
double avg = 0.0;

	sprintf(query, "select avg(ht), count(*) from k2s.game where hp = %d", hp);
	
	PGresult *pgres = pgQuery(pgConn, query);
	if (pgres != NULL && PQntuples(pgres) == 1)
	{
		avg = atoi(PQgetvalue(pgres, 0, 0));
		*count = atoi(PQgetvalue(pgres, 0, 1));
		PQclear(pgres);
		return avg;
	} else return -1;
}

int AvgV(PGconn *pgConn, int vp, int *count)
{
char query[1024];
double avg = 0.0;

	sprintf(query, "select avg(vt), count(*) from k2s.game where vp = %d", vp);
	
	PGresult *pgres = pgQuery(pgConn, query);
	if (pgres != NULL && PQntuples(pgres) == 1)
	{
		avg = atoi(PQgetvalue(pgres, 0, 0));
		*count = atoi(PQgetvalue(pgres, 0, 1));
		PQclear(pgres);
		return avg;
	} else return -1;
}

//
// main
//
int main(int argc, char* argv[])
{
char buffer_error[512], database_name[32];
PGconn *pgConn = NULL;
PLAYER players[32];

	if (argc > 1 && strlen(argv[1]) == 5)
	{
		sprintf(database_name, "k%s", argv[1]);
		pgConn = pgOpenConn(database_name, "k2", "", buffer_error);
		printf("database connection     = %p %s\n", pgConn, database_name);
		int nb_players = LoadPlayers(pgConn, &players[0]);
		for (int ip = 0 ; ip < nb_players ; ip++)
		{
			int ch = 0, cv = 0;
			int avgh = AvgH(pgConn, players[ip].id, &ch);
			int avgv = AvgV(pgConn, players[ip].id, &cv);
			double avg = 0.0;
			if (ch + cv > 0)
				avg = ((ch * avgh) + (cv * avgv)) / (ch + cv);
			players[ip].avg = avg;
			players[ip].games = ch + cv;
			printf("%3d %5s %8.2f  %6.1f  %6d\n", players[ip].id, players[ip].label, players[ip].rating, avg / 1000.0, players[ip].games);
		}
	}
	else
	{
		printf("error.invalid-database-name\n");
		return -1;
	}
}
