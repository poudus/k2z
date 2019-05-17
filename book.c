
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



int LoadGames(PGconn *pgConn, GAME *pg)
{
char query[512], buf[32];

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
				pg->vr = atof(PQgetvalue(pgres, ig, 4));

				pg++;
			}
		}
		PQclear(pgres);
		return nb_games;
	} else return -1;
}

int add_position(char *key, char trait, char winner, double hr, double vr, int nb_positions, POSITION *positions, bool duplicate)
{
bool bfound = false;
int ip = 0, ifound = -1;

	while (ip < nb_positions && !bfound)
	{
		if (strcmp(key, positions[ip].key) == 0)
		{
			ifound = ip;
			bfound = true;
		}
		ip++;
	}
	if (!bfound)
	{
		strcpy(positions[nb_positions].key, key);
		positions[nb_positions].count = 0;
		positions[nb_positions].rcount = 0;
		positions[nb_positions].win = 0;
		positions[nb_positions].loss = 0;
		positions[nb_positions].hr = 0.0;
		positions[nb_positions].vr = 0.0;
		positions[nb_positions].dratio = 0.0;
		positions[nb_positions].elodiff = 0.0;
		positions[nb_positions].dexpr = 0.0;
		positions[nb_positions].trait_ratio = 0.0;
		positions[nb_positions].move_ratio = 0.0;
		ifound = nb_positions;
		nb_positions++;
	}
	if (ifound >= 0 && ifound < nb_positions)
	{
		positions[ifound].rcount++;
		positions[ifound].hr += hr;
		positions[ifound].vr += vr;

        if (!duplicate)
        {
            positions[ifound].count++;
            if (trait == winner)
                positions[ifound].win++;
            else
                positions[ifound].loss++;
        }
	}
	return nb_positions;
}

int cmppos(const void *p1, const void *p2)
{
	if (((POSITION*)p2)->dratio > ((POSITION*)p1)->dratio)
		return 1;
	else
		return -1;
}

bool chk_dup_move(char *moves, char *move, int *id1, int *id2)
{
	int len = strlen(moves);
	if (len % 2 == 1) return true;
	else
	{
		int len2 = len/2;
		bool dup = false;
		for (int i = 0 ; i < len2 -1 ; i++)
		{
		for (int j = i+1 ; j < len2 ; j++)
		{
			if (moves[2*i] == moves[2*j] && moves[2*i+1] == moves[2*j+1])
			{
				move[0] = moves[2*i];
				move[1] = moves[2*i+1];
				move[2] = 0;
				*id1 = i+1;
				*id2 = j+1;
				dup = true;
				break;
			}
		}
		}
		return dup;
	}
}

int ComputeBookMoves(PGconn *pgConn, BOOK_MOVE *book_moves, int size, int min_count, int depth, double min_elo_sum)
{
char	flipped_moves[128], buf[256], tmp[32];
int     nb_book_moves = 0, nb_duplicate = 0, id1, id2, nb_positions = 0, nb_games_filtered = 0;
bool	hf, vf, duplicate = false;
GAME	*games = malloc(sizeof(GAME)*500000);
POSITION *positions = malloc(sizeof(POSITION)*500000);

	//int depth = strlen(key) / 2;
	int filter = depth + 1;
	char trait = (depth % 2 == 0 ? 'H' : 'V');
	int nb_games = LoadGames(pgConn, games);
	for (int ig = 0 ; ig < nb_games ; ig++)
	{
		if (strlen(games[ig].moves) > (2 * depth) && (games[ig].hr + games[ig].vr) >= min_elo_sum)
		{
			duplicate = false;
			for (int ig2 = 0 ; ig2 < ig && !duplicate ; ig2++)
				duplicate = strcmp(games[ig].moves, games[ig2].moves) == 0;
			if (duplicate)
				nb_duplicate++;
            
            strcpy(buf, games[ig].moves);
            buf[2 * filter] = 0;
if (chk_dup_move(games[ig].moves, tmp, &id1, &id2)) printf("DUP-MOVE  %s(%2d,%2d)  in %-48s  id = %6d\n", tmp, id1, id2, games[ig].moves, games[ig].id);
else
{
            strcpy(flipped_moves, FlipMoves(size, buf, filter, &hf, &vf));
            nb_positions = add_position(flipped_moves, trait, games[ig].winner, games[ig].hr, games[ig].vr, nb_positions, positions, duplicate);
}
            nb_games_filtered++;
		}

	}
	printf("\n%d games filtered, %d duplicates, %d positions, depth = %d, elo >= %6.2f, count >= %d\n",
                nb_games_filtered, nb_duplicate, nb_positions, depth, min_elo_sum, min_count);
	for (int ip = 0 ; ip < nb_positions ; ip++)
	{
		if (positions[ip].rcount >= min_count && (positions[ip].win + positions[ip].loss) > 0)
		{
			positions[ip].hr /= positions[ip].rcount;
			positions[ip].vr /= positions[ip].rcount;
			positions[ip].trait_ratio = 100.0 * positions[ip].win / positions[ip].count;
			positions[ip].move_ratio = 100.0 * positions[ip].loss / positions[ip].count;

			if (positions[ip].trait_ratio > 0.0 && positions[ip].trait_ratio < 100.0)
				positions[ip].elodiff = EloDifference(positions[ip].trait_ratio / 100.0);
			else if (positions[ip].trait_ratio > 99.999)
				positions[ip].elodiff = 1000;
			else if (positions[ip].trait_ratio == 0.0)
				positions[ip].elodiff = -1000;

			if (trait == 'H')
			{
				positions[ip].dexpr = 100.0 * EloExpectedResult(positions[ip].hr, positions[ip].vr);
				positions[ip].elodiff -= (positions[ip].hr - positions[ip].vr);
			}
			else
			{
				positions[ip].dexpr = 100.0 * EloExpectedResult(positions[ip].vr, positions[ip].hr);
				positions[ip].elodiff -= (positions[ip].vr - positions[ip].hr);
			}
			positions[ip].dratio =  positions[ip].trait_ratio - positions[ip].dexpr;
		}
		else
		{
			positions[ip].hr = -1.0;
			positions[ip].vr = -1.0;
		}
	}
    
	qsort(&positions[0], nb_positions, sizeof(POSITION), cmppos);

	for (int ip = 0 ; ip < nb_positions ; ip++)
	{
		if (positions[ip].hr > 0.0 && positions[ip].vr > 0.0)
		{
			strncpy(book_moves[nb_book_moves].key, &positions[ip].key[0], 2*depth);
			book_moves[nb_book_moves].key[2*depth] = 0;
			strncpy(book_moves[nb_book_moves].move, &positions[ip].key[2*depth], 2);
			book_moves[nb_book_moves].move[2] = 0;
			book_moves[nb_book_moves].ratio = positions[ip].dratio;
			book_moves[nb_book_moves].win = positions[ip].win;
			book_moves[nb_book_moves].loss = positions[ip].loss;
			book_moves[nb_book_moves].count = positions[ip].count;
			nb_book_moves++;
		}
	}
	free(games);
	free(positions);
	return nb_book_moves;
}

bool InsertBookMove(PGconn *pgConn, int depth, BOOK_MOVE *bm)
{
	int nbc = 0;

	pgExecFormat(pgConn, &nbc, "insert into k2s.book values (%d, '%s', '%s', %6.2f, %6.2f, %d, %d, %d)",
				depth, bm->key, bm->move, bm->ratio, bm->ratio, bm->count, bm->win, bm->loss);
	return nbc == 1;
}

bool UpdateDeepRatio(PGconn *pgConn, int depth, char *key, char *move, double deep_ratio)
{
	int nbc = 0;

	pgExecFormat(pgConn, &nbc, "update k2s.book set deep_ratio = %6.2f where depth = %d and key = '%s' and move = '%s'",
				deep_ratio, depth, key, move);
	return nbc == 1;
}

int DeleteBookMoves(PGconn *pgConn, int depth)
{
	int nbc = 0;

	pgExecFormat(pgConn, &nbc, "delete from k2s.book where depth = %d", depth);
	return nbc;
}

int ListBookMoves(PGconn *pgConn, int size, char *moves, BOOK_MOVE *bm, bool deep)
{
char query[256], key[128];
bool hf, vf;
int nb_moves = 0;

    strcpy(key, moves);
    FlipMoves(size, key, strlen(key)/2, &hf, &vf);
    //printf("moves=%s  key=%s  hf=%c  vf=%c\n", moves, key, hf ? 'T' : 'F', vf ? 'T' : 'F');
            
    if (deep)
        sprintf(query, "select key, move, ratio, deep_ratio, count, win, loss from k2s.book where key = '%s' order by deep_ratio desc", key);
    else
        sprintf(query, "select key, move, ratio, deep_ratio, count, win, loss from k2s.book where key = '%s' order by ratio desc", key);
	
	PGresult *pgres = pgQuery(pgConn, query);
	if (pgres != NULL)
	{
		while (nb_moves < PQntuples(pgres))
		{
			strcpy(bm->key, PQgetvalue(pgres, nb_moves, 0));
            
			strcpy(key, PQgetvalue(pgres, nb_moves, 1));
            key[0]= Flip(size, key[0], hf);
            key[1]= Flip(size, key[1], vf);
			strcpy(bm->move, key);
            
            if (deep)
            {
			bm->ratio = atof(PQgetvalue(pgres, nb_moves, 3));
			bm->deep_ratio = atof(PQgetvalue(pgres, nb_moves, 3));
            }
            else
            {
			bm->ratio = atof(PQgetvalue(pgres, nb_moves, 2));
			bm->deep_ratio = atof(PQgetvalue(pgres, nb_moves, 2));
            }
			bm->count = atoi(PQgetvalue(pgres, nb_moves, 4));
			bm->win = atoi(PQgetvalue(pgres, nb_moves, 5));
			bm->loss = atoi(PQgetvalue(pgres, nb_moves, 6));
            
			nb_moves++;
			bm++;
		}
		PQclear(pgres);
	} else nb_moves = -1;
	return nb_moves;
}

void CheckBook(PGconn *pgConn)
{
    bool bfound = false;
    int depth = 0;
    char move[4], key[32];
	
	PGresult *pgres = pgQuery(pgConn, "select key, move, depth from k2s.book order by depth asc");
	if (pgres != NULL)
	{
        int nbc = PQntuples(pgres);
        for (int i = 0 ; i < nbc ; i++)
        {
			strcpy(key, PQgetvalue(pgres, i, 0));
			strcpy(move, PQgetvalue(pgres, i, 1));
			depth = atoi(PQgetvalue(pgres, i, 2));
            //----------
            if (strlen(key) == 2 * depth)
            {
                bfound = false;
                for (int j = 0 ; j < depth ; j++)
                {
                    if (key[2*j] == move[0] && key[2*j+1] == move[1]) bfound = true;
                }
                if (bfound) printf("MOVE '%s' FOUND in KEY '%s'\n", move, key);
            }
            else printf("LENGTH MISMATCH  depth = %d  key = '%s'\n", depth, key);
        }
		PQclear(pgres);
    }
}


int select_move_ratios(PGconn *pgConn, int depth, char* key, BOOK_MOVE *bm)
{
int nbc = 0;
char query[256];

	sprintf(query, "select move, ratio, deep_ratio, count, win, loss from k2s.book where depth = %d and key = '%s' order by deep_ratio desc", depth, key);
	PGresult *pgres = pgQuery(pgConn, query);
	if (pgres != NULL)
	{
		int count = PQntuples(pgres);
		for (int i = 0 ; i < count ; i++)
		{
			strcpy(bm->move, PQgetvalue(pgres, i, 0));
			bm->ratio = atof(PQgetvalue(pgres, i, 1));
			bm->deep_ratio = atof(PQgetvalue(pgres, i, 2));
			bm->count = atoi(PQgetvalue(pgres, i, 3));
			bm->win   = atoi(PQgetvalue(pgres, i, 4));
			bm->loss  = atoi(PQgetvalue(pgres, i, 5));
            bm++;
		}
		PQclear(pgres);
        return count;
	}
}


int DeepBookMoves(PGconn *pgConn, int depth, BOOK_MOVE *bm)
{
char	query[256];
            
	sprintf(query, "select key, move, ratio, deep_ratio, count, win, loss from k2s.book where depth = %d order by key asc, deep_ratio desc", depth);

	PGresult *pgres = pgQuery(pgConn, query);
	if (pgres != NULL)
	{
		int nb_books = PQntuples(pgres);
		for (int ib = 0 ; ib < nb_books ; ib++)
		{
			strcpy(bm->key, PQgetvalue(pgres, ib, 0));
			strcpy(bm->move, PQgetvalue(pgres, ib, 1));
	    
			bm->ratio = atof(PQgetvalue(pgres, ib, 2));
			bm->deep_ratio = atof(PQgetvalue(pgres, ib, 3));
			bm->count = atoi(PQgetvalue(pgres, ib, 4));
			bm->win   = atoi(PQgetvalue(pgres, ib, 5));
			bm->loss  = atoi(PQgetvalue(pgres, ib, 6));
	    
			bm++;
		}
		PQclear(pgres);
		return nb_books;
	} else return -1;
}

int BuildParentBookMoves(PGconn *pgConn, int depth, BOOK_MOVE *bm)
{
BOOK_MOVE children[5000];
char	previous_key[64], move[4];

int	nb_parents = 0;
int	nb_children = DeepBookMoves(pgConn, depth, &children[0]);

	previous_key[0] = 0;
	for (int ichild = 0 ; ichild < nb_children ; ichild++)
	{
		if (strcmp(previous_key, children[ichild].key) == 0)
		{
			bm->count += children[ichild].count;
			bm->win += children[ichild].loss;
			bm->loss += children[ichild].win;
		}
		else
		{
            int slen = strlen(children[ichild].key);
            move[0] = children[ichild].key[slen-2];
            move[1] = children[ichild].key[slen-1];
            move[2] = 0;
			strcpy(bm->key, children[ichild].key);
            bm->key[slen-2] = 0;
			strcpy(bm->move, move);
			bm->count = children[ichild].count;
			bm->ratio = -children[ichild].ratio;
			bm->deep_ratio = -children[ichild].deep_ratio;
			bm->win = children[ichild].loss;
			bm->loss = children[ichild].win;
            
			nb_parents++;
			bm++;
		}
		strcpy(previous_key, children[ichild].key);
	}
	return nb_parents;
}


