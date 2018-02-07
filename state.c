

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>


#include "state.h"


double score_lambda(LAMBDA *lambda)
{
	return 50.0;
}

double score_field(FIELD *field)
{
	return 50.0;
}

double evaluation(STATE *state)
{
	return 50.0;
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
		for (int n = 0 ; n < board->slot[move->slot].neighbors ; n++)
		{
			int sn = board->slot[move->slot].neighbor[n];
			int ln = board->slot[move->slot].link[n];
			bool bcut = false;
			for (int p = 0 ; p < state->horizontal.pegs && !bcut ; p++)
			{
				if (sn == state->horizontal.peg[p])
				{
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
			if (!bcut)
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
		for (int n = 0 ; n < board->slot[move->slot].neighbors ; n++)
		{
			int sn = board->slot[move->slot].neighbor[n];
			int ln = board->slot[move->slot].link[n];
			for (int p = 0 ; p < state->vertical.pegs ; p++)
			{
				if (sn == state->vertical.peg[p])
				{
					bool bcut = false;
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
					if (!bcut)
					{
						state->vertical.link[state->vertical.links] = ln;
						state->vertical.links++;
printf("debug.move: vertical link created = %d\n", ln);
					}
				}
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


