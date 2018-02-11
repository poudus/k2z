

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <math.h>


#include "state.h"

double score(unsigned int ui1, unsigned int ui2)
{
unsigned int sum = ui1 + ui2;

	if (sum > 0.0)
		return 100.0 * ui1 / sum;
	else
		return 50.0;
}

double score_lambda(LAMBDA *l1, LAMBDA *l2, double weight_pegs, double weight_links, double weight_spread)
{
double weight = 1.0 + fabs(weight_pegs) + fabs(weight_links) + fabs(weight_spread), sum_score = 0.0;

	sum_score = score(l1->waves, l2->waves);
	sum_score += weight_pegs * score(l1->pegs, l2->pegs);
	sum_score += weight_links * score(l1->links, l2->links);
	sum_score += weight_spread * score(l1->spread, l2->spread);

	l1->score = sum_score / weight;
	l2->score = 100.0 - l1->score;

	return l1->score;
}

void lambda_field(GRID *grid, FIELD *field)
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
			field->lambda[lambda].spread += field->wave[w].spread;

			field->lambda[lambda].waves++;

			for (int s = 0 ; s < grid->path[w].slots ; s++)
				field->lambda[lambda].slot[grid->path[w].slot[s]]++;
		}
	}
}

double score_state(BOARD *board, STATE *state, double lambda_decay, double weight_pegs, double weight_links, double weight_spread)
{
double	sum_weight = 0.0, sum_h = 0.0, lambda_weight = 1.0;

	lambda_field(&board->horizontal, &state->horizontal);
	lambda_field(&board->vertical, &state->vertical);
	
	for (int lambda = 0 ; lambda < PATH_MAX_LENGTH ; lambda++)
	{
		if (state->horizontal.lambda[lambda].waves > 0 || state->vertical.lambda[lambda].waves > 0)
		{
			sum_h += lambda_weight * score_lambda(&state->horizontal.lambda[lambda],
								&state->vertical.lambda[lambda],
								weight_pegs, weight_links, weight_spread); 
			sum_weight += lambda_weight;
			lambda_weight *= lambda_decay;
		}
	}
	// track moves
	state->score = sum_h / sum_weight;
	return state->score;
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
		state->horizontal.wave[w].spread = 0;
	}
	state->vertical.waves = vpaths;
	state->vertical.wave = malloc(vpaths * sizeof(WAVE));
	for (int w = 0 ; w < vpaths ; w++)
	{
		state->vertical.wave[w].status = ' ';
		state->vertical.wave[w].pegs = 0;
		state->vertical.wave[w].links = 0;
		state->vertical.wave[w].spread = 0;
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
void my_hpeg(BOARD *board, unsigned short slot, WAVE *wave, int waves)
{
	for (int h = 0 ; h < board->slot[slot].hpaths ; h++)
		wave[board->slot[slot].hpath[h]].pegs++;
}
void my_vpeg(BOARD *board, unsigned short slot, WAVE *wave, int waves)
{
	for (int v = 0 ; v < board->slot[slot].vpaths ; v++)
		wave[board->slot[slot].vpath[v]].pegs++;
}
void op_hpeg(BOARD *board, unsigned short slot, WAVE *wave, int waves)
{
	for (int h = 0 ; h < board->slot[slot].hpaths ; h++)
		wave[board->slot[slot].hpath[h]].status = 'X';
}
void op_vpeg(BOARD *board, unsigned short slot, WAVE *wave, int waves)
{
	for (int v = 0 ; v < board->slot[slot].vpaths ; v++)
		wave[board->slot[slot].vpath[v]].status = 'X';
}

void cut_hlink(BOARD *board, unsigned short step, WAVE *wave, int waves)
{
	for (int c = 0 ; c < board->step[step].cuts ; c++)
	{
		int sc = board->step[step].cut[c];
		for (int s = 0 ; s < board->step[sc].hpaths ; s++)
			wave[board->step[sc].hpath[s]].status = 'X';
	}
}
void cut_vlink(BOARD *board, unsigned short step, WAVE *wave, int waves)
{
	for (int c = 0 ; c < board->step[step].cuts ; c++)
	{
		int sc = board->step[step].cut[c];
		for (int s = 0 ; s < board->step[sc].vpaths ; s++)
			wave[board->step[sc].vpath[s]].status = 'X';
	}
}

void my_hlink(BOARD *board, unsigned short step, WAVE *wave, int waves)
{
	for (int h = 0 ; h < board->step[step].hpaths ; h++)
		wave[board->step[step].hpath[h]].links++;
}
void my_vlink(BOARD *board, unsigned short step, WAVE *wave, int waves)
{
	for (int v = 0 ; v < board->step[step].vpaths ; v++)
		wave[board->step[step].vpath[v]].links++;
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
				printf("debug.move: horizontal link created = %d, sn = %d\n", ln, sn);
			}
		}
		state->horizontal.pegs++;
		my_hpeg(board, move->slot, state->horizontal.wave, state->horizontal.waves);
		op_hpeg(board, move->slot, state->vertical.wave, state->vertical.waves);

		for (int hs = hlinks ; hs < state->horizontal.links ; hs++)
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

		for (int vs = vlinks ; vs < state->vertical.links ; vs++)
		{
			my_vlink(board, state->vertical.link[vs], state->vertical.wave, state->vertical.waves);
		}
	}
	for (int hs = hlinks ; hs < state->horizontal.links ; hs++)
	{
		cut_hlink(board, state->horizontal.link[hs], state->horizontal.wave, state->horizontal.waves);
		cut_vlink(board, state->horizontal.link[hs], state->vertical.wave, state->vertical.waves);
	}
	for (int vs = vlinks ; vs < state->vertical.links ; vs++)
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


