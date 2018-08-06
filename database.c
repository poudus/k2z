
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <memory.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#include <math.h>
#include <pwd.h>

#include <libpq-fe.h>
#include "database.h"


//====================================================
//	UTILS
//====================================================

int getldate(time_t tt)
{
	struct tm *pnow = localtime( &tt ); /* Convert to local time. */
	
	return pnow->tm_mday + 100*(pnow->tm_mon+1) + 10000*(pnow->tm_year+1900);
}

int getlnow()
{	
	return getldate(time(NULL));
}

unsigned long long getCurrentUTS()
{
	unsigned long long uts = 0ULL;
	unsigned long long uty = 0ULL;
	time_t tt;
	
	time( &tt );                /* Get time as long integer. */
	struct tm *pnow = localtime( &tt ); /* Convert to local time. */
	
	uts = pnow->tm_sec +100*pnow->tm_min + 10000*pnow->tm_hour + 1000000*pnow->tm_mday + 100000000L*(pnow->tm_mon+1);
	
	uty = 10000000000ULL*(pnow->tm_year+1900);
	uts += uty;
	
	return uts;
}


char *formatTS(long long ts, char *buffer)
{
	int time = ts % 1000000;
	int seconds = time % 100;
	int restant = time / 100;
	int minutes = restant % 100;
	int hour = restant / 100;
	int date = ts / 1000000;
	int day = date % 100;
	restant = date / 100;
	int month = restant % 100;
	int year = restant / 100;

	sprintf(buffer, "%4d-%02d-%02d  %02d:%02d:%02d", year, month, day, hour, minutes, seconds);

	return buffer;
}

//====================================================
//	POSTGRES
//====================================================

void pgCloseConn(PGconn *conn)
{
	PQfinish(conn);
}

PGconn *pgOpenConn(const char *pdbname, const char *pdbusername, const char *pdbpassword, char *buffer_error)
{
PGconn *pgConn = NULL;
char connect_string[256];

	sprintf(connect_string, "dbname=%s user=%s password=%s",
		pdbname,
		pdbusername,
		pdbpassword);
		
	pgConn = PQconnectdb(connect_string);

	/*
    * check to see that the backend connection was successfully made
    */
	if (PQstatus(pgConn) == CONNECTION_BAD)
	{
		if (buffer_error != NULL)
		{
			strcpy(buffer_error, PQerrorMessage(pgConn));
		}
		return NULL;
	}
	else
	{
		return pgConn;
	}
}

bool pgExec(PGconn *pgConn, int *pnbc, const char *pCommand)
{
bool bOK = false;
PGresult   *pgres = NULL;

	*pnbc = 0;
        pgres = PQexec(pgConn, pCommand);
        if (pgres == NULL || PQresultStatus(pgres) != PGRES_COMMAND_OK)
        {
		printf("\nSQL_ERROR: '%s'  [%s]\n", pCommand, PQerrorMessage(pgConn));
		if (pgres != NULL) PQclear(pgres);
		bOK = false;
        }
	else
	{
		*pnbc = atoi(PQcmdTuples(pgres));
		bOK = true;
	}

	return bOK;
}

PGresult *pgQuery(PGconn *pgConn, const char *pQuery)
{
	PGresult   *pgres = NULL;

	pgres = PQexec(pgConn, pQuery);
	if (pgres == NULL)
	{
		printf("pgres is NULL\n");
	}
	else
	{
		ExecStatusType est = PQresultStatus(pgres);
		if (est != PGRES_TUPLES_OK)
		{
			printf("SQL_ERROR: '%s'  [%s]\n", pQuery, PQerrorMessage(pgConn));
			if (pgres != NULL) PQclear(pgres);
			pgres = NULL;
		}
	}
	return pgres;
}

bool pgExecFormat(PGconn *pgConn, int *pnbc, char *szFormat, ...)
{
	char buffer[4800];
	va_list args;

	va_start( args, szFormat );
	vsprintf( buffer, szFormat, args);
	va_end( args );
	*pnbc = 0;
	
	return pgExec(pgConn, pnbc, buffer);
}

bool pgBeginTransaction(PGconn *pgConn)
{
	int nbc = 0;
	
	return pgExec(pgConn, &nbc, "BEGIN");
}

bool pgCommitTransaction(PGconn *pgConn)
{
	int nbc = 0;
	
	return pgExec(pgConn, &nbc, "COMMIT");
}

bool pgRollbackTransaction(PGconn *pgConn)
{
	int nbc = 0;
	
	return pgExec(pgConn, &nbc, "ROLLBACK");
}

long long pgGetSequo(PGconn *pgConn, const char *psequo)
{
char query[256];
long long lval = -1;
PGresult   *pgres = NULL;

	sprintf(query, "select nextval('%s')", psequo);

	pgres = PQexec(pgConn, query);
	if (pgres != NULL)
	{
		if (PQresultStatus(pgres) != PGRES_TUPLES_OK)
		{
			printf("Database ERROR: %s\n", PQerrorMessage(pgConn));
			lval = -2;
		}
		else
		{
			if (PQntuples(pgres) > 0)
			{
				lval = atoll(PQgetvalue(pgres, 0, 0));
			}
		}
		PQclear(pgres);
	}
	else
	{
		printf("pgGetSequo(): NULL PGresult returned.\n");
		lval = -3;
	}

	return lval;
}

int pgGetCount(PGconn *pgConn, const char *ptable, const char *pwhere)
{
	char query[512];
	int count = -1;
	PGresult   *pgres = NULL;

	if (pwhere != NULL && strlen(pwhere) > 0)
		sprintf(query, "select count(*) from %s where %s", ptable, pwhere);
	else
		sprintf(query, "select count(*) from %s", ptable);

	pgres = PQexec(pgConn, query);
	if (pgres != NULL)
	{
		if (PQresultStatus(pgres) != PGRES_TUPLES_OK)
		{
			printf("Database ERROR: %s\n", PQerrorMessage(pgConn));
		}
		else
		{
			if (PQntuples(pgres) > 0)
			{
				count = atoi(PQgetvalue(pgres, 0, 0));
			}
		}
		PQclear(pgres);
	}
	else
	{
		printf("pgGetCount(): NULL PGresult returned.\n");
	}

	return count;
}


double pgGetDouble(PGconn *pgConn, const char *pQuery)
{
	double dValue = 0.0;
	PGresult   *pgres = NULL;


	pgres = PQexec(pgConn, pQuery);
	if (pgres != NULL)
	{
		if (PQresultStatus(pgres) != PGRES_TUPLES_OK)
		{
			printf("Database ERROR: %s\n", PQerrorMessage(pgConn));
		}
		else
		{
			if (PQntuples(pgres) > 0)
			{
				dValue = atoi(PQgetvalue(pgres, 0, 0));
			}
		}
		PQclear(pgres);
	}
	else
	{
		printf("pgGetDouble(): NULL PGresult returned.\n");
	}

	return dValue;
}

int pgGetMax(PGconn *pgConn, const char *pColumn, const char *ptable, const char *pwhere)
{
	char query[256];
	int max = -1;
	PGresult   *pgres = NULL;

	if (pwhere != NULL && strlen(pwhere) > 0)
		sprintf(query, "select max(%s) from %s where %s", pColumn, ptable, pwhere);
	else
		sprintf(query, "select max(%s) from %s", pColumn, ptable);

	pgres = PQexec(pgConn, query);
	if (pgres != NULL)
	{
		if (PQresultStatus(pgres) != PGRES_TUPLES_OK)
		{
			printf("Database ERROR: %s\n", PQerrorMessage(pgConn));
		}
		else
		{
			if (PQntuples(pgres) > 0)
			{
				max = atoi(PQgetvalue(pgres, 0, 0));
			}
		}
		PQclear(pgres);
	}
	else
	{
		printf("pgGetMax(): NULL PGresult returned.\n");
	}

	return max;
}

double pgGetSum(PGconn *pgConn, const char *pColumn, const char *ptable, const char *pwhere)
{
	char query[256];
	double sum = 0.0;
	PGresult   *pgres = NULL;

	if (pwhere != NULL && strlen(pwhere) > 0)
		sprintf(query, "select sum(%s) from %s where %s", pColumn, ptable, pwhere);
	else
		sprintf(query, "select sum(%s) from %s", pColumn, ptable);

	pgres = PQexec(pgConn, query);
	if (pgres != NULL)
	{
		if (PQresultStatus(pgres) != PGRES_TUPLES_OK)
		{
			printf("Database ERROR: %s\n", PQerrorMessage(pgConn));
		}
		else
		{
			if (PQntuples(pgres) > 0)
			{
				sum = atof(PQgetvalue(pgres, 0, 0));
			}
		}
		PQclear(pgres);
	}
	else
	{
		printf("pgGetSum(): NULL PGresult returned.\n");
	}

	return sum;
}

//====================================================
//	GAME
//====================================================

int insertGame(PGconn *pgConn, int hp, int vp, char *moves, char winner, char reason, int duration, int ht, int vt)
{
	int nbc = 0, game_id = -1;
	unsigned long long uts = getCurrentUTS();

	if (pgBeginTransaction(pgConn))
	{
		game_id = pgGetSequo(pgConn, "k2s.game_sequo");
		if (game_id < 0)
		{
			pgRollbackTransaction(pgConn);
		}
		else
		{
			int length = strlen(moves) / 2;
			pgExecFormat(pgConn, &nbc, "insert into k2s.game values (%d, %llu, %d, %d, '%s', '%c', '%c', %d, %d, %d, %d)",
				game_id, uts, hp, vp, moves, winner, reason, duration, length, ht, vt);
			if (nbc == 1)
				pgCommitTransaction(pgConn);
			else
			{
				pgRollbackTransaction(pgConn);
				game_id = -1;
			}
		}
	}
	return game_id;
}

bool LoadGame(PGconn *pgConn, int game, char *moves)
{
char query[256];

	sprintf(query, "select moves from k2s.game where id = %d", game);
	
	PGresult *pgres = pgQuery(pgConn, query);
	if (pgres != NULL && PQntuples(pgres) == 1)
	{
		strcpy(moves, PQgetvalue(pgres, 0, 0));
		PQclear(pgres);
		return true;
	} else return false;
}

//====================================================
//	PLAYER
//====================================================

bool LoadPlayerParameters(PGconn *pgConn, int player, PLAYER_PARAMETERS *ppp)
{
long long id = -1;
char query[256];

	sprintf(query, "select lambda_decay, opp_decay, max_moves, depth, rating from k2s.player where id = %d", player);
	
	PGresult *pgres = pgQuery(pgConn, query);
	if (pgres != NULL && PQntuples(pgres) == 1)
	{
		ppp->pid = player;
		ppp->lambda_decay = atof(PQgetvalue(pgres, 0, 0));
		ppp->opp_decay = atof(PQgetvalue(pgres, 0, 1));
		ppp->max_moves = atoi(PQgetvalue(pgres, 0, 2));
		ppp->depth = atoi(PQgetvalue(pgres, 0, 3));
		ppp->rating = atof(PQgetvalue(pgres, 0, 4));
		PQclear(pgres);
		return true;
	} else return false;
}

bool UpdatePlayerWin(PGconn *pgConn, int player, double gain)
{
	int nbc = 0;
	
	if (pgExecFormat(pgConn, &nbc, "update k2s.player set win = win+1, rating = rating + %.2f  where id = %d", gain, player))
	{
	  return nbc == 1;
	}
	else return false;
}

bool UpdatePlayerLoss(PGconn *pgConn, int player, double loss)
{
	int nbc = 0;
	
	if (pgExecFormat(pgConn, &nbc, "update k2s.player set loss = loss+1, rating = rating - %.2f  where id = %d", loss, player))
	{
	  return nbc == 1;
	}
	else return false;
}

bool UpdatePlayerDraw(PGconn *pgConn, int player, double gl)
{
	int nbc = 0;
	
	if (gl > 0.0)
	{
		if (pgExecFormat(pgConn, &nbc, "update k2s.player set draw = draw+1, rating = rating + %.2f  where id = %d", gl, player))
		{
		  return nbc == 1;
		}
		else return false;
	}
	else
	{
		if (pgExecFormat(pgConn, &nbc, "update k2s.player set draw = draw+1, rating = rating - %.2f  where id = %d", fabs(gl), player))
		{
		  return nbc == 1;
		}
		else return false;
	}
}

//====================================================
//	LIVE
//====================================================

void PrintLive(PGconn *pgConn, int channel)
{
char query[256];

	sprintf(query, "select ts, hp, vp, hpid, vpid, moves, trait, last_move, length, winner, reason from k2s.live where channel = %d", channel);
	
	PGresult *pgres = pgQuery(pgConn, query);
	if (pgres != NULL && PQntuples(pgres) == 1)
	{
		printf("%ld  %d/%d  vs  %d/%d\n", atol(PQgetvalue(pgres, 0, 0)),
			atoi(PQgetvalue(pgres, 0, 1)), atoi(PQgetvalue(pgres, 0, 3)),
			atoi(PQgetvalue(pgres, 0, 2)), atoi(PQgetvalue(pgres, 0, 4)));

		printf("%s  (%d)  %s  %s\n", PQgetvalue(pgres, 0, 5), atoi(PQgetvalue(pgres, 0, 8)), PQgetvalue(pgres, 0, 7), PQgetvalue(pgres, 0, 6));
		printf("Winner %s  reason = '%s'\n", PQgetvalue(pgres, 0, 9), PQgetvalue(pgres, 0, 10));

		PQclear(pgres);
	}
}

bool DeleteLive(PGconn *pgConn, int channel)
{
	int nbc = 0;

	pgExecFormat(pgConn, &nbc, "delete from k2s.live where channel = %d", channel);
	return nbc == 1;
}

bool RegisterLive(PGconn *pgConn, int channel, int hp)
{
	int nbc = 0;
	unsigned long long uts = getCurrentUTS();
	int pid = getpid();

	pgExecFormat(pgConn, &nbc, "insert into k2s.live values (%d, %llu, %d, %d, %d, %d, '%s', '%c', '%s', %d, '%c', '%c')",
				channel, uts, hp, 0, pid, 0, "", 'H', "  ", 0, ' ', ' ');
	return nbc == 1;
}

bool JoinLive(PGconn *pgConn, int channel, int vp)
{
	int nbc = 0;
	unsigned long long uts = getCurrentUTS();
	int pid = getpid();

	pgExecFormat(pgConn, &nbc, "update k2s.live set vp = %d, vpid = %d, ts = %llu where channel = %d and vp = 0 and vpid = 0",
				vp, pid, uts, channel);
	return nbc == 1;
}

bool CheckLive(PGconn *pgConn, int channel, char orientation, char *last_move, char *moves)
{
char query[256];

	sprintf(query, "select last_move, moves from k2s.live where channel = %d and trait = '%c'", channel, orientation);
	
	PGresult *pgres = pgQuery(pgConn, query);
	if (pgres != NULL && PQntuples(pgres) == 1)
	{
		strcpy(last_move, PQgetvalue(pgres, 0, 0));
		strcpy(moves, PQgetvalue(pgres, 0, 1));

		PQclear(pgres);
		return true;
	} else return false;
}

bool PlayLive(PGconn *pgConn, int channel, char orientation, char *move, char *moves)
{
	int nbc = 0;
	unsigned long long uts = getCurrentUTS();
	int pid = getpid();
	char trait = 'H';
	if (orientation == 'H')
		trait = 'V';

	char new_moves[128];
	sprintf(new_moves, "%s%s", moves, move);
	int nb_moves = strlen(new_moves) / 2;

	pgExecFormat(pgConn, &nbc,
	"update k2s.live set last_move = '%s', moves = '%s', ts = %llu, trait = '%c', length = %d where channel = %d and trait = '%c' and moves = '%s' and winner = ' '",
		move, new_moves, uts, trait, nb_moves, channel, orientation, moves);
	return nbc == 1;
}

bool ResignLive(PGconn *pgConn, int channel, char winner, char reason)
{
	int nbc = 0;
	unsigned long long uts = getCurrentUTS();
	int pid = getpid();

	pgExecFormat(pgConn, &nbc, "update k2s.live set winner = '%c', reason = '%c', ts = %llu where channel = %d",
				winner, reason, uts, channel);
	return nbc == 1;
}

bool CheckResign(PGconn *pgConn, int channel, char orientation)
{
char query[256];

	sprintf(query, "select reason from k2s.live where channel = %d and winner = '%c'", channel, orientation);
	
	PGresult *pgres = pgQuery(pgConn, query);
	if (pgres != NULL && PQntuples(pgres) == 1)
	{
		PQclear(pgres);
		return true;
	} else return false;
}

bool WinLive(PGconn *pgConn, int channel, char winner, char reason)
{
	int nbc = 0;
	unsigned long long uts = getCurrentUTS();
	int pid = getpid();

	pgExecFormat(pgConn, &nbc, "update k2s.live set winner = '%c', reason = '%c', ts = %llu where channel = %d",
				winner, reason, uts, channel);
	return nbc == 1;
}



