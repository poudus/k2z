

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

double score_lambda(LAMBDA *l1, LAMBDA *l2, double w1, double w2)
{
double weight = 1.0 + fabs(w1) + fabs(w2), sum_score = 0.0;

	sum_score = score(l1->waves, l2->waves);
	//sum_score += w1 * score(l1->p1, l2->p1);
	//sum_score += w2 * score(l1->p2, l2->p2);

	l1->score = sum_score / weight;
	l2->score = 100.0 - l1->score;

	return l1->score;
}

void lambda_field(BOARD *board, GRID *grid, FIELD *field, bool init_tracks, bool bh)
{
int lambda = 0;

	bzero(&field->lambda[0], PATH_MAX_LENGTH * sizeof(LAMBDA));
	for (int w = 0 ; w < field->waves ; w++)
	{
		if (field->wave[w].status == ' ')
		{
			lambda = grid->path[w].slots - field->wave[w].pegs;

			field->lambda[lambda].pegs += field->wave[w].pegs;
			field->lambda[lambda].links += field->wave[w].links;
			field->lambda[lambda].weakness += field->wave[w].weakness;

			field->lambda[lambda].waves++;

			if (init_tracks)
			{
				for (int s = 0 ; s < grid->path[w].slots ; s++)
				{
					int rank= 0;
					if (bh)
						rank = board->slot[grid->path[w].slot[s]].hrank[s];
					else
						rank = board->slot[grid->path[w].slot[s]].vrank[s];

					if (field->wave[w].slot[rank] == ' ')
						field->lambda[lambda].slot[grid->path[w].slot[s]]++;
				}
				for (int s = 0 ; s < grid->path[w].steps ; s++)
				{
					int rank= 0;
					if (bh)
						rank = board->step[grid->path[w].step[s]].hrank[s];
					else
						rank = board->step[grid->path[w].step[s]].vrank[s];

					if (field->wave[w].step[rank] == ' ')
						field->lambda[lambda].step[grid->path[w].step[s]]++;
				}
			}
		}
	}
}

double eval_state(BOARD *board, FIELD *player, FIELD *opponent, double lambda_decay, double w1, double w2)
{
double	sum_weight = 0.0, sum_p = 0.0, lambda_weight = 1.0;
	
	for (int lambda = 0 ; lambda < PATH_MAX_LENGTH ; lambda++)
	{
		if (player->lambda[lambda].waves > 0 || opponent->lambda[lambda].waves > 0)
		{
			sum_p += lambda_weight * score_lambda(&player->lambda[lambda], &opponent->lambda[lambda], w1, w2);
			player->lambda[lambda].weight = opponent->lambda[lambda].weight = lambda_weight;    
			sum_weight += lambda_weight;
			lambda_weight *= lambda_decay;
			//printf("L = %2d  sum_p = %6.2f  lw = %6.2f  sw = %6.2f  ld = %6.2f\n", lambda, sum_p, lambda_weight, sum_weight, lambda_decay);
		} //else printf("no-waves\n");
	}
	return sum_p / sum_weight;
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
		player->track[s].weight = 100.0 * player->track[s].value / sum_w;
		//printf("S = %3d  weight = %6.2f  value = %6.2f  sumw = %6.2f\n", s, player->track[s].weight, player->track[s].value, sum_w);
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
	return moves;
}

// Q
long init_state(STATE *state, unsigned int hpaths, unsigned int vpaths)
{
	state->horizontal.waves = hpaths;
	state->horizontal.wave = malloc(hpaths * sizeof(WAVE));
	for (int w = 0 ; w < hpaths ; w++)
	{
		state->horizontal.wave[w].status = ' ';
		state->horizontal.wave[w].pegs = 0;
		state->horizontal.wave[w].links = 0;
		state->horizontal.wave[w].weakness = 0;
		memcpy(&state->horizontal.wave[w].slot, "                ", 16);
		memcpy(&state->horizontal.wave[w].step, "                ", 16);
	}
	state->vertical.waves = vpaths;
	state->vertical.wave = malloc(vpaths * sizeof(WAVE));
	for (int w = 0 ; w < vpaths ; w++)
	{
		state->vertical.wave[w].status = ' ';
		state->vertical.wave[w].pegs = 0;
		state->vertical.wave[w].links = 0;
		state->vertical.wave[w].weakness = 0;
		memcpy(&state->vertical.wave[w].slot, "                ", 16);
		memcpy(&state->vertical.wave[w].step, "                ", 16);
	}
	state->horizontal.pegs = state->horizontal.links = 0;
	state->vertical.pegs = state->vertical.links = 0;
	return (hpaths+vpaths) * sizeof(WAVE);
}

void clone_state(STATE *from, STATE *to)
{
	to->horizontal.waves = from->horizontal.waves;
	to->horizontal.wave = malloc(to->horizontal.waves * sizeof(WAVE));
	for (int w = 0 ; w < to->horizontal.waves ; w++)
		memcpy(&to->horizontal.wave[w], &from->horizontal.wave[w], sizeof(WAVE));

	to->vertical.waves = from->vertical.waves;
	to->vertical.wave = malloc(to->vertical.waves * sizeof(WAVE));
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
	for (int h = 0 ; h < board->slot[slot].hpaths ; h++)
	{
		wave[board->slot[slot].hpath[h]].pegs++;
		wave[board->slot[slot].hpath[h]].slot[board->slot[slot].hrank[h]] = 'X';
	}
}

//
//	Increment my peg count of all vertical paths for a given slot
//
void my_vpeg(BOARD *board, unsigned short slot, WAVE *wave, int waves)
{
	for (int v = 0 ; v < board->slot[slot].vpaths ; v++)
	{
		wave[board->slot[slot].vpath[v]].pegs++;
		wave[board->slot[slot].vpath[v]].slot[board->slot[slot].vrank[v]] = 'X';
	}
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
		wave[board->slot[slot].vpath[v]].status = 'X';
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
	for (int h = 0 ; h < board->step[step].hpaths ; h++)
	{
		wave[board->step[step].hpath[h]].links++;
		wave[board->step[step].hpath[h]].step[board->step[step].hrank[h]] = 'X';
	}
}
void my_vlink(BOARD *board, unsigned short step, WAVE *wave, int waves)
{
	for (int v = 0 ; v < board->step[step].vpaths ; v++)
	{
		wave[board->step[step].vpath[v]].links++;
		wave[board->step[step].vpath[v]].step[board->step[step].vrank[v]] = 'X';
	}
}

int state_move(BOARD *board, STATE *state, MOVE *move)
{
	//int hlinks = state->horizontal.links;
	//int vlinks = state->vertical.links;

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
				printf("debug.move: horizontal link created = %d, sn = %d\n", ln, sn);
			}
		}
		state->horizontal.pegs++;
		my_hpeg(board, move->slot, state->horizontal.wave, state->horizontal.waves);
		op_hpeg(board, move->slot, state->vertical.wave, state->vertical.waves);

		for (int hs = 0 ; hs < state->horizontal.links ; hs++) // hlinks
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
printf("debug.move: vertical link created = %d, sn = %d\n", ln, sn);
			}
		}
		state->vertical.pegs++;
		my_vpeg(board, move->slot, state->vertical.wave, state->vertical.waves);
		op_vpeg(board, move->slot, state->horizontal.wave, state->horizontal.waves);

		for (int vs = 0 ; vs < state->vertical.links ; vs++) // vlinks
		{
			my_vlink(board, state->vertical.link[vs], state->vertical.wave, state->vertical.waves);
		}
	}
	for (int hs = 0 ; hs < state->horizontal.links ; hs++) // hlinks
	{
		cut_hlink(board, state->horizontal.link[hs], state->horizontal.wave, state->horizontal.waves);
		cut_vlink(board, state->horizontal.link[hs], state->vertical.wave, state->vertical.waves);
	}
	for (int vs = 0 ; vs < state->vertical.links ; vs++) // vlinks
	{
		cut_hlink(board, state->vertical.link[vs], state->horizontal.wave, state->horizontal.waves);
		cut_vlink(board, state->vertical.link[vs], state->vertical.wave, state->vertical.waves);
	}
	state->horizontal.count = 0;
	for (int hw = 0 ; hw < state->horizontal.waves ; hw++)
	{
		if (state->horizontal.wave[hw].status == ' ') state->horizontal.count++;
	}
	state->vertical.count = 0;
	for (int vw = 0 ; vw < state->vertical.waves ; vw++)
	{
		if (state->vertical.wave[vw].status == ' ') state->vertical.count++;
	}
	return state->horizontal.count + state->vertical.count;
	//return state->horizontal.waves + state->vertical.waves;
}

void free_state(STATE *state)
{
	free(state->horizontal.wave);
	free(state->vertical.wave);
}


