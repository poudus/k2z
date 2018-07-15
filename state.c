

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <math.h>

#include "state.h"


bool	debug_state = false;

double score(unsigned int ui1, unsigned int ui2)
{
unsigned int sum = ui1 + ui2;

	if (sum > 0.0)
		return 100.0 * ui1 / sum;
	else
		return 50.0;
}

double score_lambda(LAMBDA *l1, LAMBDA *l2, double wpegs, double wlinks, double wzeta)
{
double weight = 1.0 + fabs(wpegs) + fabs(wlinks) + fabs(wzeta), sum_score = 0.0;

	sum_score = score(l1->waves, l2->waves);
	sum_score += wpegs * score(l1->pegs, l2->pegs);
	sum_score += wlinks * score(l1->links, l2->links);
	sum_score += wzeta * score(l1->zeta, l2->zeta);

	l1->score = sum_score / weight;
	l2->score = 100.0 - l1->score;

	return l1->score;
}

void printLambdaWave(GRID *grid, FIELD *field, int lambda)
{
char buffer1[64], buffer2[64];

	for (int w = 0 ; w < field->waves ; w++)
	{
		if (field->wave[w].status == 'O')
		{
			int ll = grid->path[w].slots - field->wave[w].pegs;
			if (ll == lambda)
			{
				PATH *ph = &grid->path[w];
				buffer1[0] = buffer2[0] = 0;
#ifdef __ZETA__
				strncpy(buffer1, field->wave[w].slot, 16);
				buffer1[ph->slots] = 0;
				strncpy(buffer2, field->wave[w].step, 16);
				buffer2[ph->steps] = 0;
#endif
				printf("%8d:  %2d - %-2d  [%s]  [%s]\n", w, grid->path[w].slots, field->wave[w].pegs, buffer1, buffer2);
			}
		}
	}
}

void lambda_field(BOARD *board, GRID *grid, FIELD *field, bool init_tracks)
{
int lambda = 0;

	bzero(&field->lambda[0], PATH_MAX_LENGTH * sizeof(LAMBDA));
	for (int w = 0 ; w < field->waves ; w++)
	{
		if (field->wave[w].status == 'O')
		{
			lambda = grid->path[w].slots - field->wave[w].pegs;

			field->lambda[lambda].pegs += field->wave[w].pegs;
			field->lambda[lambda].links += field->wave[w].links;
			field->lambda[lambda].zeta += field->wave[w].zeta;

			field->lambda[lambda].waves++;

			if (init_tracks)
			{
				int pegs = 0;
				for (int s = 0 ; s < grid->path[w].slots ; s++)
				{
#ifdef __ZETA__
					if (field->wave[w].slot[s] == '.')
#endif
						field->lambda[lambda].slot[grid->path[w].slot[s]]++;
					//else if (field->wave[w].slot[s] == 'X')
					//	pegs++;
				}
				int links = 0;
				for (int s = 0 ; s < grid->path[w].steps ; s++)
				{
#ifdef __ZETA__
					if (field->wave[w].step[s] == '-')
#endif
						field->lambda[lambda].step[grid->path[w].step[s]]++;
					//else if (field->wave[w].step[s] == '=')
					//	links++;
				}

				/*if (pegs != field->wave[w].pegs)
				{
					char buff[32];
					strncpy(buff, field->wave[w].slot, grid->path[w].slots);
					buff[grid->path[w].slots] = 0;
					printf("ERROR w = %8d  lambda = %2d  slots = %2d  pegs = %2d   count =%2d  [%s]\n",
						w, lambda, grid->path[w].slots, field->wave[w].pegs, pegs, buff);
				}*/
			}
		}
	}
}

bool winning_field(FIELD *pfield)
{
	return pfield->lambda[0].waves > 0;
}
bool empty_field(FIELD *pfield)
{
	return pfield->count == 0;
}

double eval_state(BOARD *board, FIELD *player, FIELD *opponent, double lambda_decay, double wpegs, double wlinks, double wzeta)
{
double	sum_weight = 0.0, sum_p = 0.0, lambda_weight = 1.0;
int lowest_lambda = 99;
	
	for (int lambda = 0 ; lambda < PATH_MAX_LENGTH ; lambda++)
	{
		if (player->lambda[lambda].waves > 0 || opponent->lambda[lambda].waves > 0)
		{
			if (lambda < lowest_lambda) lowest_lambda = lambda;
			sum_p += lambda_weight * score_lambda(&player->lambda[lambda], &opponent->lambda[lambda], wpegs, wlinks, wzeta);
			player->lambda[lambda].weight = opponent->lambda[lambda].weight = lambda_weight;    
			sum_weight += lambda_weight;
			lambda_weight *= lambda_decay;
		}
	}
	double dscore = sum_p / sum_weight;
	if (lowest_lambda == 0)
	{
		if (dscore > 90.0) dscore = 100.0;
		else if (dscore < 10.0) dscore = 0.0;
		else
		{
			printf("lowest_lambda_zero : score = %6.2f\n", dscore);
			exit(-1);
		}
	}
	else if (lowest_lambda == 1)
	{
		if (dscore > 99.9) dscore = 99.9;
		else if (dscore < 0.1) dscore = 0.1;
	}
	//if (lowest_lambda == 2 && dscore > 98.0) dscore = 98.0;
	if (dscore > 100.0 || dscore < 0.0)
	{
		printf("lowest_lambda = %2d  score = %6.2f\n", lowest_lambda, dscore);
		exit(-1);
	}
	return dscore;
}

void build_field_tracks(BOARD *board, FIELD *player)
{
double sum_w = 0.0;

	for (int s = 0 ; s < board->slots ; s++)
	{
		player->track[s].idx = s;
		player->track[s].value = 0.0;
		sum_w = 0.0;

		for (int lambda = 0 ; lambda < PATH_MAX_LENGTH ; lambda++)
		{
			if (player->lambda[lambda].waves > 0)
			{
				player->track[s].value += (1.0 * player->lambda[lambda].slot[s] / player->lambda[lambda].waves) * player->lambda[lambda].weight;
				sum_w += player->lambda[lambda].weight;
			}
		}
		if (sum_w > 0.0)
			player->track[s].weight = 100.0 * player->track[s].value / sum_w;
		else
			player->track[s].weight = 0.0;
	}
}

void build_state_tracks(BOARD *board, STATE *state)
{
	build_field_tracks(board, &state->horizontal);
	build_field_tracks(board, &state->vertical);
}

int cmpmove(const void *p1, const void *p2)
{
	if (((TRACK*)p2)->weight > ((TRACK*)p1)->weight)
		return 1;
	else
		return -1;
}

bool is_field_peg(FIELD *field, unsigned short slot)
{
	for (int s = 0 ; s < field->pegs ; s++)
	{
		if (field->peg[s] == slot) return true;
	}
	return false;
}

bool is_state_peg(STATE *state, unsigned short slot)
{
	if (is_field_peg(&state->horizontal, slot))
		return true;
	else
		return is_field_peg(&state->vertical, slot);
}

int state_moves(BOARD *board, STATE *state, char orientation, double opponent_decay, TRACK *move)
{
double	v = 0.0;
int	moves = 0;

//printf("state_moves %c\n", orientation);

	for (int s = 0 ; s < board->slots ; s++)
	{
		if (is_state_peg(state, s)) continue;

		if (orientation == 'H')
		{
			if (board->slot[s].y == 0 || board->slot[s].y == (board->height -1)) continue;
			v = state->horizontal.track[s].weight + opponent_decay * state->vertical.track[s].weight;
		}
		else
		{
			if (board->slot[s].x == 0 || board->slot[s].x == (board->width -1)) continue;
			v = opponent_decay * state->horizontal.track[s].weight + state->vertical.track[s].weight;
		}

		move[moves].idx = s;
		move[moves].value = v;
		move[moves].weight = v / (1.0 + opponent_decay);
		moves++;
	}
	qsort(move, moves, sizeof(TRACK), cmpmove);
//printf("state_moves %d moves\n", moves);
	return moves;
}

// Q
long init_state(STATE *state, unsigned int hpaths, unsigned int vpaths, bool alloc_wave)
{
	state->horizontal.waves = state->horizontal.count = hpaths;
	if (alloc_wave) state->horizontal.wave = malloc(hpaths * sizeof(WAVE));
	for (int w = 0 ; w < hpaths ; w++)
	{
		state->horizontal.wave[w].status = 'O';
		state->horizontal.wave[w].pegs = 0;
		state->horizontal.wave[w].links = 0;
		state->horizontal.wave[w].zeta = 0;
#ifdef __ZETA__
		memcpy(&state->horizontal.wave[w].slot, "................", 16);
		memcpy(&state->horizontal.wave[w].step, "----------------", 16);
#endif
	}
	state->vertical.waves = state->vertical.count = vpaths;
	if (alloc_wave) state->vertical.wave = malloc(vpaths * sizeof(WAVE));
	for (int w = 0 ; w < vpaths ; w++)
	{
		state->vertical.wave[w].status = 'O';
		state->vertical.wave[w].pegs = 0;
		state->vertical.wave[w].links = 0;
		state->vertical.wave[w].zeta = 0;
#ifdef __ZETA__
		memcpy(&state->vertical.wave[w].slot, "................", 16);
		memcpy(&state->vertical.wave[w].step, "----------------", 16);
#endif
	}
	state->horizontal.pegs = state->horizontal.links = 0;
	state->vertical.pegs = state->vertical.links = 0;
	return (hpaths+vpaths) * sizeof(WAVE);
}

void clone_state(STATE *from, STATE *to, bool alloc_waves)
{
	to->horizontal.waves = from->horizontal.waves;
	if (alloc_waves) to->horizontal.wave = malloc(to->horizontal.waves * sizeof(WAVE));
	for (int w = 0 ; w < to->horizontal.waves ; w++)
		memcpy(&to->horizontal.wave[w], &from->horizontal.wave[w], sizeof(WAVE));

	to->vertical.waves = from->vertical.waves;
	if (alloc_waves) to->vertical.wave = malloc(to->vertical.waves * sizeof(WAVE));
	for (int w = 0 ; w < to->vertical.waves ; w++)
		memcpy(&to->vertical.wave[w], &from->vertical.wave[w], sizeof(WAVE));

	to->horizontal.pegs = from->horizontal.pegs;
	for (int ip = 0 ; ip < from->horizontal.pegs ; ip++)
		to->horizontal.peg[ip] = from->horizontal.peg[ip];
	to->horizontal.links = from->horizontal.links;
	for (int il = 0 ; il < from->horizontal.links ; il++)
		to->horizontal.link[il] = from->horizontal.link[il];

	to->vertical.pegs = from->vertical.pegs;
	for (int ip = 0 ; ip < from->vertical.pegs ; ip++)
		to->vertical.peg[ip] = from->vertical.peg[ip];
	to->vertical.links = from->vertical.links;
	for (int il = 0 ; il < from->vertical.links ; il++)
		to->vertical.link[il] = from->vertical.link[il];
}

//
//	Increment my peg count of all horizontal paths for a given slot
//
void my_hpeg(BOARD *board, unsigned short slot, WAVE *wave, int waves)
{
int hpaths = 0, sx = 0, hp = 0;

	for (int h = 0 ; h < board->slot[slot].hpaths ; h++)
	{
		hp = board->slot[slot].hpath[h];
		wave[hp].pegs++;
		hpaths++;

#ifdef __ZETA__
		int idx = SearchPathSlot(&board->horizontal.path[hp], slot);
		if (idx >= 0 && idx < PATH_MAX_LENGTH)
		{
			wave[hp].slot[idx] = 'X';
			sx++;
		}
#endif
	}
	if (debug_state) printf("slot = %3d  hpaths = %8d  sx = %6d\n", slot, hpaths, sx);
}

//
//	Increment my peg count of all vertical paths for a given slot
//
void my_vpeg(BOARD *board, unsigned short slot, WAVE *wave, int waves)
{
int vpaths = 0, sx = 0, vp = 0;

	for (int v = 0 ; v < board->slot[slot].vpaths ; v++)
	{
		vp = board->slot[slot].vpath[v];
		wave[vp].pegs++;
		vpaths++;

#ifdef __ZETA__
		int idx = SearchPathSlot(&board->vertical.path[vp], slot);
		if (idx >= 0 && idx < PATH_MAX_LENGTH)
		{
			wave[vp].slot[idx] = 'X';
			sx++;
		}
#endif
	}
	if (debug_state) printf("slot = %3d  vpaths = %8d  sx = %6d\n", slot, vpaths, sx);
}

//
//	CUT all opponent horizontal paths for a given slot
//
void op_hpeg(BOARD *board, unsigned short slot, WAVE *wave, int waves)
{
	for (int h = 0 ; h < board->slot[slot].hpaths ; h++)
	{
		wave[board->slot[slot].hpath[h]].status = 'X';
	}
	if (debug_state) printf("slot[%4d     ] = %8d vx\n", slot, board->slot[slot].hpaths);
}

//
//	CUT all opponent vertical paths for a given slot
//
void op_vpeg(BOARD *board, unsigned short slot, WAVE *wave, int waves)
{
	for (int v = 0 ; v < board->slot[slot].vpaths ; v++)
	{
		int vp = board->slot[slot].vpath[v];
		wave[vp].status = 'X';
		if (debug_state && vp == 42979) printf(" vpath 42979 cut\n");
	}
	if (debug_state) printf("slot[%4d     ] = %8d hx\n", slot, board->slot[slot].vpaths);
}

//
//	CUT all horizontal paths for a given step
//
void cut_hlink(BOARD *board, unsigned short step, WAVE *wave, int waves)
{
int	nb = 0;

	for (int c = 0 ; c < board->step[step].cuts ; c++)
	{
		int sc = board->step[step].cut[c];
		if (debug_state) printf("xcut[%4d/%4s] = %8d hx\n", sc, board->step[sc].code, board->step[sc].hpaths);
		for (int s = 0 ; s < board->step[sc].hpaths ; s++)
		{
			wave[board->step[sc].hpath[s]].status = 'X';
			nb++;
		}
	}
	if (debug_state) printf("step[%4d     ] = %8d hx  %2d cuts\n", step, nb, board->step[step].cuts);
}

//
//	CUT all vertical paths for a given step
//
void cut_vlink(BOARD *board, unsigned short step, WAVE *wave, int waves)
{
int	nb = 0;

	for (int c = 0 ; c < board->step[step].cuts ; c++)
	{
		int sc = board->step[step].cut[c];
		if (debug_state) printf("xcut[%4d/%4s] = %8d vx\n", sc, board->step[sc].code, board->step[sc].vpaths);
		for (int s = 0 ; s < board->step[sc].vpaths ; s++)
		{
			wave[board->step[sc].vpath[s]].status = 'X';
			nb++;
		}
	}
	if (debug_state) printf("step[%4d     ] = %8d vx  %2d cuts\n", step, nb, board->step[step].cuts);
}

void my_hlink(BOARD *board, unsigned short step, WAVE *wave, int waves)
{
int hlinks = 0, lx = 0, hp = 0;

	for (int h = 0 ; h < board->step[step].hpaths ; h++)
	{
		hp = board->step[step].hpath[h];
		wave[hp].links++;
		hlinks++;

#ifdef __ZETA__
		int idx = SearchPathStep(&board->horizontal.path[hp], step);
		if (idx >= 0 && idx < PATH_MAX_LENGTH)
		{
			wave[hp].step[idx] = '=';
			lx++;
		}
#endif
	}
	if (debug_state) printf("step = %3d  hpaths = %8d  lx = %6d\n", step, hlinks, lx);
}

void my_vlink(BOARD *board, unsigned short step, WAVE *wave, int waves)
{
int vlinks = 0, lx = 0, vp = 0;

	for (int v = 0 ; v < board->step[step].vpaths ; v++)
	{
		vp = board->step[step].vpath[v];
		wave[vp].links++;
		vlinks++;

#ifdef __ZETA__
		int idx = SearchPathStep(&board->vertical.path[vp], step);
		if (idx >= 0 && idx < PATH_MAX_LENGTH)
		{
			wave[vp].step[idx] = '=';
			lx++;
		}
#endif
	}
	if (debug_state) printf("step = %3d  vpaths = %8d  lx = %6d\n", step, vlinks, lx);
}

int state_move(BOARD *board, STATE *state, MOVE *move)
{
	int hlinks = state->horizontal.links;
	int vlinks = state->vertical.links;

	if (move->orientation == 'H')
	{
		state->horizontal.peg[state->horizontal.pegs] = move->slot;
		for (int n = 0 ; n < board->slot[move->slot].neighbors ; n++) // for all neighbors
		{
			int sn = board->slot[move->slot].neighbor[n];
			int ln = board->slot[move->slot].link[n];
			bool bpeg = false, bcut = false;
			for (int p = 0 ; p < state->horizontal.pegs && !bcut ; p++) // for all horizontal pegs
			{
				if (sn == state->horizontal.peg[p]) // neighbor is peg
				{
					bpeg = true;
					for (int k = 0 ; k < state->horizontal.links && !bcut ; k++)
					{
						for (int kk = 0 ; kk < board->step[ln].cuts ; kk++)
						{
							if (board->step[ln].cut[kk] == state->horizontal.link[k])
							{
								bcut = true;
								break;
							}
						}
					}
					for (int k = 0 ; k < state->vertical.links && !bcut ; k++)
					{
						for (int kk = 0 ; kk < board->step[ln].cuts ; kk++)
						{
							if (board->step[ln].cut[kk] == state->vertical.link[k])
							{
								bcut = true;
								break;
							}
						}
					}
				}
			}
			if (bpeg && !bcut)
			{
				state->horizontal.link[state->horizontal.links] = ln;
				state->horizontal.links++;
				if (debug_state) printf("debug.move: horizontal link created = %d, sn = %d\n", ln, sn);
			}
		}
		state->horizontal.pegs++;
		my_hpeg(board, move->slot, state->horizontal.wave, state->horizontal.waves);
		op_vpeg(board, move->slot, state->vertical.wave, state->vertical.waves);

		for (int hs = hlinks ; hs < state->horizontal.links ; hs++) // hlinks
		{
			my_hlink(board, state->horizontal.link[hs], state->horizontal.wave, state->horizontal.waves);
		}
	}
	else if (move->orientation == 'V')
	{
		state->vertical.peg[state->vertical.pegs] = move->slot;
		for (int n = 0 ; n < board->slot[move->slot].neighbors ; n++) // for all neighbors
		{
			int sn = board->slot[move->slot].neighbor[n];
			int ln = board->slot[move->slot].link[n];
			bool bpeg = false, bcut = false;
			for (int p = 0 ; p < state->vertical.pegs ; p++) // for all vertical pegs
			{
				if (sn == state->vertical.peg[p]) // neighbor is peg
				{
					bpeg = true;
					for (int k = 0 ; k < state->horizontal.links && !bcut ; k++)
					{
						for (int kk = 0 ; kk < board->step[ln].cuts ; kk++)
						{
							if (board->step[ln].cut[kk] == state->horizontal.link[k])
							{
								bcut = true;
								break;
							}
						}
					}
					for (int k = 0 ; k < state->vertical.links && !bcut ; k++)
					{
						for (int kk = 0 ; kk < board->step[ln].cuts ; kk++)
						{
							if (board->step[ln].cut[kk] == state->vertical.link[k])
							{
								bcut = true;
								break;
							}
						}
					}
				}
			}
			if (bpeg && !bcut)
			{
				state->vertical.link[state->vertical.links] = ln;
				state->vertical.links++;
				if (debug_state) printf("debug.move: vertical link created = %d, sn = %d\n", ln, sn);
			}
		}
		state->vertical.pegs++;
		my_vpeg(board, move->slot, state->vertical.wave, state->vertical.waves);
		op_hpeg(board, move->slot, state->horizontal.wave, state->horizontal.waves);

		for (int vs = vlinks ; vs < state->vertical.links ; vs++) // vlinks
		{
			my_vlink(board, state->vertical.link[vs], state->vertical.wave, state->vertical.waves);
		}
	}
	for (int hs = hlinks ; hs < state->horizontal.links ; hs++) // hlinks
	{
		cut_hlink(board, state->horizontal.link[hs], state->horizontal.wave, state->horizontal.waves);
		cut_vlink(board, state->horizontal.link[hs], state->vertical.wave, state->vertical.waves);
	}
	for (int vs = vlinks ; vs < state->vertical.links ; vs++) // vlinks
	{
		cut_hlink(board, state->vertical.link[vs], state->horizontal.wave, state->horizontal.waves);
		cut_vlink(board, state->vertical.link[vs], state->vertical.wave, state->vertical.waves);
	}
	state->horizontal.count = 0;
	for (int hw = 0 ; hw < state->horizontal.waves ; hw++)
	{
		if (state->horizontal.wave[hw].status == 'O') state->horizontal.count++;
	}
	state->vertical.count = 0;
	for (int vw = 0 ; vw < state->vertical.waves ; vw++)
	{
		if (state->vertical.wave[vw].status == 'O') state->vertical.count++;
	}
	return state->horizontal.count + state->vertical.count;
	//return state->horizontal.waves + state->vertical.waves;
}

void free_state(STATE *state)
{
	free(state->horizontal.wave);
	free(state->vertical.wave);
}


