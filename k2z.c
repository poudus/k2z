
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>

// STATIC SINGLETON

#define	NB_MAX_SLOTS	1024
#define	NB_MAX_STEPS	4096
#define	PATH_MAX_LENGTH	16

typedef struct
{
unsigned char	x,y;
char		code[4];
unsigned short	idx, neighbors;
unsigned short	neighbor[8], link[8];
unsigned int	*hpath, *vpath, hpaths, vpaths;
} SLOT;

typedef struct
{
unsigned short	from, to, idx, cuts, sx, sy, sign;
unsigned short	cut[10];
char		code[4];
unsigned int	*hpath, *vpath, hpaths, vpaths;
} STEP;

typedef struct
{
unsigned int	slots, steps;
unsigned short	slot[PATH_MAX_LENGTH];
unsigned short	step[PATH_MAX_LENGTH];
} PATH;

typedef struct
{
unsigned int	count;
double		weight;
} TRACK;

/* typedef struct
{
unsigned int	start, count, slots, steps;
TRACK		slot[256];
TRACK		step[1024];
} LPATH; */

typedef struct
{
unsigned int	paths;
PATH		*path;
//LPATH		lpath[16];
} GRID;

typedef struct
{
int		width, height, slots, steps;
SLOT		slot[NB_MAX_SLOTS];
STEP		step[NB_MAX_STEPS];
GRID		horizontal, vertical;
} BOARD;

#define ONE_MILLION		1000000
#define ONE_BILLION		1000000000

#define MAX_PATH_PER_SLOT	2 * ONE_MILLION
#define MAX_PATH_PER_STEP	1 *ONE_MILLION
#define MAX_PATHS		10 * ONE_MILLION


void printStep(STEP *pstep)
{
	printf("idx=%d sx=%d sy=%d sign=%d  from=%d to=%d\n",
		pstep->idx, pstep->sx, pstep->sy, pstep->sign, pstep->from, pstep->to);
}
static int gslots = 0, gsteps = 0;

void search_path_v(BOARD *board, int slot, int step, int depth,
			int slots, unsigned short *pslot,
			int steps, unsigned short *pstep,
			int max_depth, int min_dy)
{
	pslot[slots] = slot;
	slots++;
	if (step >= 0)
	{
		pstep[steps] = step;
		steps++;
	}

	if (board->slot[slot].y == board->height-1)
	{
		board->vertical.path[board->vertical.paths].slots = slots;
		for (int s = 0 ; s < slots ; s++)
		{
			int ss = pslot[s];
			if (ss < 0 || ss >= board->slots)
				printf("error slot %d %d\n", s, ss);
			else
			{
				board->vertical.path[board->vertical.paths].slot[s] = ss;
				// slots array
				board->slot[ss].vpath[board->slot[ss].vpaths] = board->vertical.paths;
				board->slot[ss].vpaths++;
			}
			gslots++;
		}
		// sort slots ?
		board->vertical.path[board->vertical.paths].steps = steps;
		for (int s = 0 ; s < steps ; s++)
		{
			int ss = pstep[s];
			if (ss < 0 || ss >= board->steps)
				printf("error step %d %d\n", s, ss);
			else
			{
				board->vertical.path[board->vertical.paths].step[s] = ss;
				// steps array
				board->step[ss].vpath[board->step[ss].vpaths] = board->vertical.paths;
				board->step[ss].vpaths++;
			}
			gsteps++;
		}
		// sort steps ?

		board->vertical.paths++;
	}
	else if (board->slot[slot].x == 0 || board->slot[slot].x == board->width-1 ||
		(board->height - board->slot[slot].y -1 ) > 2 * (max_depth - depth))
	{
		return;
	}
	else if (depth < max_depth)
	{
		for (int n = 0 ; n < board->slot[slot].neighbors ; n++)
		{
			int sn = board->slot[slot].neighbor[n];
			if (board->slot[sn].y - board->slot[slot].y >= min_dy)
			{
search_path_v(board, sn, board->slot[slot].link[n], depth+1, slots, pslot, steps, pstep, max_depth, min_dy);
			}
		}
	}
}

void search_path_h(BOARD *board, int slot, int step, int depth,
			int slots, unsigned short *pslot,
			int steps, unsigned short *pstep,
			int max_depth, int min_dx)
{
	pslot[slots] = slot;
	slots++;
	if (step >= 0)
	{
		pstep[steps] = step;
		steps++;
	}

	if (board->slot[slot].x == board->width-1)
	{
		board->horizontal.path[board->horizontal.paths].slots = slots;
		for (int s = 0 ; s < slots ; s++)
		{
			int ss = pslot[s];
			if (ss < 0 || ss >= board->slots)
				printf("error slot %d %d\n", s, ss);
			else
			{
				board->horizontal.path[board->horizontal.paths].slot[s] = ss;
				// slots array
				board->slot[ss].hpath[board->slot[ss].hpaths] = board->horizontal.paths;
				board->slot[ss].hpaths++;
				if (board->slot[ss].hpaths > MAX_PATH_PER_SLOT)
					printf("error.hpath.slot %d", board->slot[ss].hpaths);
			}
			gslots++;
		}
		// sort slots ?
		board->horizontal.path[board->horizontal.paths].steps = steps;
		for (int s = 0 ; s < steps ; s++)
		{
			int ss = pstep[s];
			if (ss < 0 || ss >= board->steps)
				printf("error step %d %d\n", s, ss);
			else
			{
				board->horizontal.path[board->horizontal.paths].step[s] = ss;
				// steps array
				board->step[ss].hpath[board->step[ss].hpaths] = board->horizontal.paths;
				board->step[ss].hpaths++;
				if (board->step[ss].hpaths > MAX_PATH_PER_STEP)
					printf("error.hpath.step %d", board->step[ss].hpaths);
			}
			gsteps++;
		}
		// sort steps ?

		board->horizontal.paths++;
	}
	else if (board->slot[slot].y == 0 || board->slot[slot].y == board->height-1 ||
		(board->width - board->slot[slot].x -1 ) > 2 * (max_depth - depth))
	{
		return;
	}
	else if (depth < max_depth)
	{
		for (int n = 0 ; n < board->slot[slot].neighbors ; n++)
		{
			int sn = board->slot[slot].neighbor[n];
			if (board->slot[sn].x - board->slot[slot].x >= min_dx)
			{
search_path_h(board, sn, board->slot[slot].link[n], depth+1, slots, pslot, steps, pstep, max_depth, min_dx);
			}
		}
	}
}


bool init_board(BOARD *board, int width, int height, int depth, int min_direction)
{
	bzero(board, sizeof(BOARD));
	board->width = width;
	board->height = height;

	// slots
	board->slots = 0;
	long size_slots = 0;
	for (int x = 0 ; x < width ; x++)
	{
		for (int y = 0 ; y < height ; y++)
		{
			bool corner = ((x == 0 && y == 0) || (x == 0 && y == height - 1) ||
				(x == width - 1 && y == 0) || (x == width - 1 && y == height - 1));
			if (!corner)
			{
				board->slot[board->slots].x = x;
				board->slot[board->slots].y = y;
				board->slot[board->slots].idx = board->slots;
				board->slot[board->slots].neighbors = 0;
				board->slot[board->slots].hpaths = 0;
				board->slot[board->slots].hpath = malloc(MAX_PATH_PER_SLOT * sizeof(unsigned int));
				size_slots += MAX_PATH_PER_SLOT * sizeof(unsigned int);
				board->slot[board->slots].vpaths = 0;
				board->slot[board->slots].vpath = malloc(MAX_PATH_PER_SLOT * sizeof(unsigned int));
				size_slots += MAX_PATH_PER_SLOT * sizeof(unsigned int);
				sprintf(board->slot[board->slots].code, "%c%c", 'A'+x, 'A'+y);
				board->slots++;
			}
		}
	}
	printf("debug.slots = %5d  size = %ld MB\n", board->slots, size_slots/1000000);
	long size_steps = 0;
	// neighbors
	board->steps = 0;
	for (int s1 = 0 ; s1 < board->slots -1 ; s1++)
	{
		for (int s2 = s1 +1 ; s2 < board->slots ; s2++)
		{
			if ((board->slot[s2].x - board->slot[s1].x)*(board->slot[s2].x - board->slot[s1].x) + (board->slot[s2].y - board->slot[s1].y)*(board->slot[s2].y - board->slot[s1].y) == 5)
			{
				board->slot[s1].neighbor[board->slot[s1].neighbors] = board->slot[s2].idx;
				board->slot[s1].link[board->slot[s1].neighbors] = board->steps+1;
				board->slot[s1].neighbors++;

				board->slot[s2].neighbor[board->slot[s2].neighbors] = board->slot[s1].idx;
				board->slot[s2].link[board->slot[s2].neighbors] = board->steps+1;
				board->slot[s2].neighbors++;

				board->step[board->steps].from = board->slot[s1].idx;
				board->step[board->steps].to = board->slot[s2].idx;
				board->step[board->steps].idx = board->steps;
				board->step[board->steps].sx = board->slot[s2].x + board->slot[s1].x;
				board->step[board->steps].sy = board->slot[s2].y + board->slot[s1].y;
				double slope = (double)(board->slot[s2].y - board->slot[s1].y) / (double)(board->slot[s2].x - board->slot[s1].x);
				if (slope > 0.0)
					board->step[board->steps].sign = 1;
				else
					board->step[board->steps].sign = 2;

				board->step[board->steps].cuts = 0;
				board->step[board->steps].hpaths = 0;
				board->step[board->steps].hpath = malloc(MAX_PATH_PER_STEP * sizeof(unsigned int));
				size_steps += MAX_PATH_PER_STEP * sizeof(unsigned int);
				board->step[board->steps].vpaths = 0;
				board->step[board->steps].vpath = malloc(MAX_PATH_PER_STEP * sizeof(unsigned int));
				size_steps += MAX_PATH_PER_STEP * sizeof(unsigned int);
				sprintf(board->step[board->steps].code, "%c%c%c%c",
					'A'+board->slot[s1].x, 'A'+board->slot[s1].y,
					'A'+board->slot[s2].x, 'A'+board->slot[s2].y);
				board->steps++;
			}
		}
	}
	printf("debug.steps = %5d  size = %ld MB\n", board->steps, size_steps/1000000);
	int max_neighbors = 0;
	for (int s = 0 ; s < board->slots ; s++)
	{
		if (board->slot[s].neighbors > max_neighbors) max_neighbors = board->slot[s].neighbors;
	}
	printf("debug.max_neighbors = %d\n", max_neighbors);
	// cuts
	for (int s1 = 0 ; s1 < board->steps -1; s1++)
	{
		for (int s2 = 0 ; s2 < board->steps ; s2++)
		{
			if (s1 != s2)
			{
int sqd = (board->step[s1].sx - board->step[s2].sx) * (board->step[s1].sx - board->step[s2].sx);
sqd += (board->step[s1].sy - board->step[s2].sy) * (board->step[s1].sy - board->step[s2].sy);

				if (
					((sqd == 4 || sqd == 0) && board->step[s1].sign != board->step[s2].sign) ||
					(sqd == 2 &&
						(board->step[s1].sign == 1 &&
			((board->step[s2].sx == board->step[s1].sx+1 && board->step[s2].sy == board->step[s1].sy+1) ||
			(board->step[s2].sx == board->step[s1].sx-1 && board->step[s2].sy == board->step[s1].sy-1))
						) ||
						(board->step[s1].sign == 2 &&
			((board->step[s2].sx == board->step[s1].sx+1 && board->step[s2].sy == board->step[s1].sy-1) ||
			(board->step[s2].sx == board->step[s1].sx-1 && board->step[s2].sy == board->step[s1].sy+1))
						)
					)
				    )
				{
					board->step[s1].cut[board->step[s1].cuts] = s2;
					board->step[s1].cuts++;
				}
			}
		}
	}
	int max_cuts = 0, sign = 0, sx = 0, sy = 0;
	for (int s = 0 ; s < board->steps ; s++)
	{
		sign += board->step[s].sign;
		sx += board->step[s].sx;
		sy += board->step[s].sy;
		if (board->step[s].cuts > max_cuts) max_cuts = board->step[s].cuts;
//printStep(&board->step[s]);
//printf("step[%03d].cuts = %d:", s, board->step[s].cuts);
//for (int icut = 0 ; icut < board->step[s].cuts ; icut++)
//	printf(" %d", board->step[s].cut[icut]);
//printf("\n");
	}
	printf("debug.max_cuts = %d\n", max_cuts);
	//printf("debug.avg_sign = %6.4f\n", (double)sign/board->steps);
	//printf("debug.avg_sx = %6.4f\n", (double)sx/board->steps);
	//printf("debug.avg_sy = %6.4f\n", (double)sy/board->steps);

	// paths
	unsigned short *slots = malloc(PATH_MAX_LENGTH*sizeof(unsigned short));
	unsigned short *steps = malloc(PATH_MAX_LENGTH*sizeof(unsigned short));

	long size_hpaths = MAX_PATHS * sizeof(PATH);
	board->horizontal.path = malloc(size_hpaths);
	board->horizontal.paths = 0;
	for (int s = 0 ; s < board->slots ; s++)
	{
		if (board->slot[s].x == 0)
			search_path_h(board, s, -1, 0, 0, slots, 0, steps, depth, min_direction);
	}
	printf("debug.horizontal_paths  = %10d             gslots = %10d  size = %ld MB\n",
			board->horizontal.paths, gslots, size_hpaths/ONE_MILLION);

	// slots paths
	int nb_slots_path = 0, nb_steps_path = 0;
	for (int p = 0 ; p < board->horizontal.paths ; p++)
	{
		nb_slots_path += board->horizontal.path[p].slots;
		nb_steps_path += board->horizontal.path[p].steps;
	}
	printf("debug.nb_slots_path                                     = %10d\n", nb_slots_path);

	int max_path_per_slot = 0, smax = 0, sump = 0, nbw = 0;
	for (int s = 0 ; s < board->slots ; s++)
	{
		if (board->slot[s].hpaths > max_path_per_slot)
		{
			max_path_per_slot = board->slot[s].hpaths;
			smax = s;
		}
		sump += board->slot[s].hpaths;
		if (board->slot[s].hpaths < 0) nbw++;
	}
	printf("debug.max_path_per_slot = %10d  %s   %5.2f %%  sum = %10d  nbw  = %d\n\n", max_path_per_slot,
			board->slot[smax].code, 100.0 * max_path_per_slot / board->horizontal.paths, sump, nbw);

	printf("debug.horizontal_paths  = %10d             gsteps = %10d\n", board->horizontal.paths, gsteps);
	printf("debug.nb_steps_path                                     = %10d\n", nb_steps_path);
	int max_path_per_step = 0;
	sump = 0; nbw = 0;
	for (int s = 0 ; s < board->steps ; s++)
	{
		if (board->step[s].hpaths > max_path_per_step)
		{
			max_path_per_step = board->step[s].hpaths;
			smax = s;
		}
		sump += board->step[s].hpaths;
		if (board->step[s].hpaths < 0) nbw++;
	}
	printf("debug.max_path_per_step = %10d  %s %5.2f %%  sum = %10d  nbw  = %d\n\n", max_path_per_step,
			board->step[smax].code, 100.0 * max_path_per_step / board->horizontal.paths, sump, nbw);


	// vertical

	gslots = 0; gsteps = 0;
	long size_vpaths = MAX_PATHS * sizeof(PATH);
	board->vertical.path = malloc(size_vpaths);
	board->vertical.paths = 0;
	for (int s = 0 ; s < board->slots ; s++)
	{
		if (board->slot[s].y == 0)
			search_path_v(board, s, -1, 0, 0, slots, 0, steps, depth, min_direction);
	}
	printf("debug.vertical_paths    = %10d             gslots = %10d  size = %ld MB\n",
			board->vertical.paths, gslots, size_vpaths/ONE_MILLION);

	// slots paths
	nb_slots_path = 0; nb_steps_path = 0;
	for (int p = 0 ; p < board->vertical.paths ; p++)
	{
		nb_slots_path += board->vertical.path[p].slots;
		nb_steps_path += board->vertical.path[p].steps;
	}
	printf("debug.nb_slots_path                                     = %10d\n", nb_slots_path);

	max_path_per_slot = 0; smax = 0; sump = 0; nbw = 0;
	for (int s = 0 ; s < board->slots ; s++)
	{
		if (board->slot[s].vpaths > max_path_per_slot)
		{
			max_path_per_slot = board->slot[s].vpaths;
			smax = s;
		}
		sump += board->slot[s].vpaths;
		if (board->slot[s].vpaths < 0) nbw++;
	}
	printf("debug.max_path_per_slot = %10d  %s   %5.2f %%  sum = %10d  nbw  = %d\n\n", max_path_per_slot,
			board->slot[smax].code, 100.0 * max_path_per_slot / board->vertical.paths, sump, nbw);

	printf("debug.vertical_paths    = %10d             gsteps = %10d\n", board->vertical.paths, gsteps);
	printf("debug.nb_steps_path                                     = %10d\n", nb_steps_path);
	max_path_per_step = 0;
	sump = 0; nbw = 0;
	for (int s = 0 ; s < board->steps ; s++)
	{
		if (board->step[s].vpaths > max_path_per_step)
		{
			max_path_per_step = board->step[s].vpaths;
			smax = s;
		}
		sump += board->step[s].vpaths;
		if (board->step[s].vpaths < 0) nbw++;
	}
	printf("debug.max_path_per_step = %10d  %s %5.2f %%  sum = %10d  nbw  = %d\n", max_path_per_step,
			board->step[smax].code, 100.0 * max_path_per_step / board->vertical.paths, sump, nbw);

	printf("debug.total_allocated   = %10ld  MB\n", (size_slots+size_steps+size_hpaths+size_vpaths) / ONE_MILLION);

	fflush(stdout);

	long paths_size_released = (2* MAX_PATHS - board->horizontal.paths - board->vertical.paths) * sizeof(PATH);
	PATH *hpath = board->horizontal.path;
	board->horizontal.path = malloc(board->horizontal.paths * sizeof(PATH));
	for (int ih = 0 ; ih < board->horizontal.paths ; ih++)
		memcpy(&board->horizontal.path[ih], &hpath[ih], sizeof(PATH));
	free(hpath);

	PATH *vpath = board->vertical.path;
	board->vertical.path = malloc(board->vertical.paths * sizeof(PATH));
	for (int ih = 0 ; ih < board->vertical.paths ; ih++)
		memcpy(&board->vertical.path[ih], &vpath[ih], sizeof(PATH));
	free(vpath);

	printf("debug.hpath_released    = %10ld  MB\n", paths_size_released / ONE_MILLION);

	// slots release
	long slots_size_released = 0;
	for (int s = 0 ; s < board->slots ; s++)
	{
		if (board->slot[s].hpaths == 0)
		{
			slots_size_released += MAX_PATH_PER_SLOT * sizeof(unsigned int);
			free(board->slot[s].hpath);
		}
		else
		{
				slots_size_released += (MAX_PATH_PER_SLOT - board->slot[s].hpaths) * sizeof(unsigned int);
				unsigned int *ph = board->slot[s].hpath;
				board->slot[s].hpath = malloc(board->slot[s].hpaths * sizeof(unsigned int));
				for (int ih = 0 ; ih < board->slot[s].hpaths ; ih++)
					board->slot[s].hpath[ih] = ph[ih];
				free(ph);
		}
		//---------
		if (board->slot[s].vpaths == 0)
		{
			slots_size_released += MAX_PATH_PER_SLOT * sizeof(unsigned int);
			free(board->slot[s].vpath);
		}
		else
		{
				slots_size_released += (MAX_PATH_PER_SLOT - board->slot[s].vpaths) * sizeof(unsigned int);
				unsigned int *ph = board->slot[s].vpath;
				board->slot[s].vpath = malloc(board->slot[s].vpaths * sizeof(unsigned int));
				for (int ih = 0 ; ih < board->slot[s].vpaths ; ih++)
					board->slot[s].vpath[ih] = ph[ih];
				free(ph);
		}
	}
	printf("debug.slots_released    = %10ld  MB\n", slots_size_released / ONE_MILLION);
		// steps release
		long steps_size_released = 0;
		for (int s = 0 ; s < board->steps ; s++)
		{
			if (board->step[s].hpaths == 0)
			{
				steps_size_released += MAX_PATH_PER_STEP * sizeof(unsigned int);
				free(board->step[s].hpath);
			}
			else
			{
					steps_size_released += (MAX_PATH_PER_STEP - board->step[s].hpaths) * sizeof(unsigned int);
					unsigned int *ph = board->step[s].hpath;
					board->step[s].hpath = malloc(board->step[s].hpaths * sizeof(unsigned int));
					for (int ih = 0 ; ih < board->step[s].hpaths ; ih++)
						board->step[s].hpath[ih] = ph[ih];
					free(ph);
			}
			//---------
			if (board->step[s].vpaths == 0)
			{
				steps_size_released += MAX_PATH_PER_STEP * sizeof(unsigned int);
				free(board->step[s].vpath);
			}
			else
			{
					steps_size_released += (MAX_PATH_PER_STEP - board->step[s].vpaths) * sizeof(unsigned int);
					unsigned int *ph = board->step[s].vpath;
					board->step[s].vpath = malloc(board->step[s].vpaths * sizeof(unsigned int));
					for (int ih = 0 ; ih < board->step[s].vpaths ; ih++)
						board->step[s].vpath[ih] = ph[ih];
					free(ph);
			}
		}
		printf("debug.steps_released    = %10ld  MB\n", steps_size_released / ONE_MILLION);
		printf("debug.total_released    = %10ld  MB\n", (paths_size_released+slots_size_released+steps_size_released) / ONE_MILLION);

		printf("debug.total_allocated   = %10ld  MB\n", (size_slots+size_steps+size_hpaths+size_vpaths-paths_size_released-slots_size_released-steps_size_released) / ONE_MILLION);

	return true;
}

// ------------
// DYNAMIC PLAY
// ------------

typedef struct
{
unsigned char	status, pegs, links, spread;
} WAVE;


typedef struct
{
int		value;
double		score, weight;
} PARAMETER;

typedef struct
{
PARAMETER	waves, pegs, links, max_steps, weight;
int tracks;
TRACK		track[NB_MAX_SLOTS];
} LAMBDA;

typedef struct
{
int			waves, count, pegs, links;
unsigned short	peg[32], link[32];
WAVE		*wave;
LAMBDA		lambda[PATH_MAX_LENGTH];
} FIELD;

typedef struct
{
FIELD		horizontal, vertical;
} STATE;

double score_lambda(LAMBDA *lambda)
{
	return 50.0;
}

double score_field(FIELD *field)
{
	return 50.0;
}

typedef struct
{
int		slot, number, steps;
char	orientation;
int		step[8];
} MOVE;


double duration(struct timeval *t1, struct timeval *t2)
{
double elapsedTime = 0.0;

	// compute and print the elapsed time in millisec
	elapsedTime = (t2->tv_sec - t1->tv_sec) * 1000.0;      // sec to ms
	elapsedTime += (t2->tv_usec - t1->tv_usec) / 1000.0;   // us to ms

	return elapsedTime;
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
			for (int p = 0 ; p < state->horizontal.pegs ; p++)
			{
				if (sn == state->horizontal.peg[p])
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
						state->horizontal.link[state->horizontal.links] = ln;
						state->horizontal.links++;
					}
				}
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

double evaluation(STATE *state)
{
	return 50.0;
}

//
// main
//
int main(int argc, char* argv[])
{
int depth = 2, width = 16, height = 16;
double contempt = 10.0;
struct timeval t0, t_init_board, t_init_wave, t_init_s0, t_clone, t_move;
BOARD board;

	printf("K2Z.engine-version=0.1\n");
	gettimeofday(&t0, NULL);
	if (argc > 0 && strlen(argv[1]) == 5)
	{
		char size[8];
		strcpy(size, argv[1]);
		size[2] = 0;
		width = atoi(&size[0]);
		height = atoi(&size[3]);
		printf("init.size=%dx%d\n", width, height);
	}
	else
	{
		printf("error.invalid-size-parameter\n");
		return -1;
	}
	if (init_board(&board, width, height, 10, -1))
	{
		gettimeofday(&t_init_board, NULL);
		printf("init.board %6.2f ms  complexity = %d\n", duration(&t0, &t_init_board), board.horizontal.paths+board.vertical.paths);

		STATE s0, s1, s2, s3, s4;
		long size_s0 = init_state(&s0, board.horizontal.paths, board.vertical.paths);
		gettimeofday(&t_init_s0, NULL);
		printf("init.s0     %6.2f ms        size = %ld MB\n", duration(&t_init_board, &t_init_s0), size_s0 / ONE_MILLION);

		clone_state(&s0, &s1);
		gettimeofday(&t_clone, NULL);
		printf("init.clone  %6.2f ms\n", duration(&t_init_s0, &t_clone));

		MOVE move;
		move.slot = 40;
		move.orientation = 'H';
		move.number = 1;
		move.steps = 0;
		int complexity = state_move(&board, &s1, &move);
		gettimeofday(&t_move, NULL);
		printf("init.move   %6.2f ms  complexity = %9d   (%d+%d %6.2f%%, %d+%d %6.2f%%)\n", duration(&t_clone, &t_move),
			complexity, s1.horizontal.pegs, s1.horizontal.links, 100.0 * s1.horizontal.count / s1.horizontal.waves,
			s1.vertical.pegs, s1.vertical.links, 100.0 * s1.vertical.count / s1.vertical.waves);

		clone_state(&s1, &s2);
		move.slot = 42;
		move.orientation = 'V';
		move.number = 2;
		complexity = state_move(&board, &s2, &move);
		printf("init.move   %6.2f ms  complexity = %9d   (%d+%d %6.2f%%, %d+%d %6.2f%%)\n", duration(&t_clone, &t_move),
			complexity, s2.horizontal.pegs, s2.horizontal.links, 100.0 * s2.horizontal.count / s2.horizontal.waves,
			s2.vertical.pegs, s2.vertical.links, 100.0 * s2.vertical.count / s2.vertical.waves);

		clone_state(&s2, &s3);
		move.slot = 54;
		move.orientation = 'H';
		move.number = 3;
		complexity = state_move(&board, &s3, &move);
		printf("init.move   %6.2f ms  complexity = %9d   (%d+%d %6.2f%%, %d+%d %6.2f%%)\n", duration(&t_clone, &t_move),
			complexity, s3.horizontal.pegs, s3.horizontal.links, 100.0 * s3.horizontal.count / s3.horizontal.waves,
			s3.vertical.pegs, s3.vertical.links, 100.0 * s3.vertical.count / s3.vertical.waves);

		clone_state(&s3, &s4);
		move.slot = 65;
		move.orientation = 'V';
		move.number = 4;
		complexity = state_move(&board, &s4, &move);
		printf("init.move   %6.2f ms  complexity = %9d   (%d+%d %6.2f%%, %d+%d %6.2f%%)\n", duration(&t_clone, &t_move),
			complexity, s4.horizontal.pegs, s4.horizontal.links, 100.0 * s4.horizontal.count / s4.horizontal.waves,
			s4.vertical.pegs, s4.vertical.links, 100.0 * s4.vertical.count / s4.vertical.waves);

	}
	else
	{
		printf("error.init\n");
	}
}
