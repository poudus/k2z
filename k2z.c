
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>

// STATIC SINGLETON

#define	NB_MAX_SLOTS	1024
#define	NB_MAX_STEPS	4096
#define	PATH_MAX_LENGTH	32

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

#define ONE_MILLION		1000000
#define ONE_BILLION		1000000000

#define MAX_PATH_PER_SLOT	2 * ONE_MILLION
#define MAX_PATH_PER_STEP	ONE_MILLION
#define MAX_PATHS		32 * ONE_MILLION

bool init_board(BOARD *board, int width, int height, int depth, int min_direction)
{
	bzero(board, sizeof(BOARD));
	board->width = width;
	board->height = height;

	// slots
	board->slots = 0;
	long size = 0;
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
				size += MAX_PATH_PER_SLOT * sizeof(unsigned int);
				board->slot[board->slots].vpaths = 0;
				board->slot[board->slots].vpath = malloc(MAX_PATH_PER_SLOT * sizeof(unsigned int));
				size += MAX_PATH_PER_SLOT * sizeof(unsigned int);
				sprintf(board->slot[board->slots].code, "%c%c", 'A'+x, 'A'+y);
				board->slots++;
			}
		}
	}
	printf("debug.slots = %5d  size = %ld MB\n", board->slots, size/1000000);
	size = 0;
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
				size += MAX_PATH_PER_STEP * sizeof(unsigned int);
				board->step[board->steps].vpaths = 0;
				board->step[board->steps].vpath = malloc(MAX_PATH_PER_STEP * sizeof(unsigned int));
				size += MAX_PATH_PER_STEP * sizeof(unsigned int);
				sprintf(board->step[board->steps].code, "%c%c%c%c",
					'A'+board->slot[s1].x, 'A'+board->slot[s1].y,
					'A'+board->slot[s2].x, 'A'+board->slot[s2].y);
				board->steps++;
			}
		}
	}
	printf("debug.steps = %5d  size = %ld MB\n", board->steps, size/1000000);
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
	unsigned short *slots = malloc(32*sizeof(unsigned short));
	unsigned short *steps = malloc(32*sizeof(unsigned short));

	size = MAX_PATHS * sizeof(PATH);
	board->horizontal.path = malloc(size);
	board->horizontal.paths = 0;
	for (int s = 0 ; s < board->slots ; s++)
	{
		if (board->slot[s].x == 0)
			search_path_h(board, s, -1, 0, 0, slots, 0, steps, depth, min_direction);
	}
	printf("debug.horizontal_paths  = %10d             gslots = %10d  size = %ld MB\n",
			board->horizontal.paths, gslots, size/1000000);

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
	board->vertical.path = malloc(size);
	board->vertical.paths = 0;
	for (int s = 0 ; s < board->slots ; s++)
	{
		if (board->slot[s].y == 0)
			search_path_v(board, s, -1, 0, 0, slots, 0, steps, depth, min_direction);
	}
	printf("debug.vertical_paths    = %10d             gslots = %10d  size = %ld MB\n",
			board->vertical.paths, gslots, size/1000000);

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

	fflush(stdout);

	return true;
}

// ------------
// DYNAMIC PLAY
// ------------

typedef struct
{
//unsigned int	path;
unsigned char	status, pegs, links, spread;
} QW;

typedef struct
{
int		path;
unsigned char	pegs, links, lambda, max_steps;
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
int		waves;
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
SLOT		slot;
int		number;
char		orientation;
} MOVE;

STATE* next_state(STATE *state, MOVE *move)
{
}

void free_state(STATE *state)
{
}

double evaluation(STATE *state)
{
	return 50.0;
}

double duration(struct timeval *t1, struct timeval *t2)
{
double elapsedTime = 0.0;

	// compute and print the elapsed time in millisec
	elapsedTime = (t2->tv_sec - t1->tv_sec) * 1000.0;      // sec to ms
	elapsedTime += (t2->tv_usec - t1->tv_usec) / 1000.0;   // us to ms

	return elapsedTime;
}

int main(int argc, char* argv[])
{
int depth = 2, width = 16, height = 16;
double contempt = 10.0;
struct timeval t0, t_init_board, t_init_wave;
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
	if (init_board(&board, width, height, 12, 1))
	{
		gettimeofday(&t_init_board, NULL);
		printf("init.board %6.2f ms\n", duration(&t0, &t_init_board));

		int nb = board.horizontal.paths+board.vertical.paths;
		int size = nb * sizeof(QW);
		QW *pqw = malloc(size);
		for (int w = 0 ; w < nb ; w++)
		{
			pqw[w].status = ' ';
			pqw[w].pegs = 0;
			pqw[w].links = 0;
			pqw[w].spread = 0;
		}
		srand(time(NULL));
		for (int rd = 0 ; rd < nb ; rd++)
		{
			int idx = rand() % nb;
			pqw[idx].status = 'X';
		}
		for (int z = 0 ; z < nb ; z++)
		{
			if (pqw[z].status == ' ') pqw[z].status = '=';
		}
		gettimeofday(&t_init_wave, NULL);
		printf("init.qw  %6.2f ms  complexity = %d  size = %d MB\n",
			duration(&t_init_board, &t_init_wave), nb, size/1000000);
	}
	else
	{
		printf("error.init\n");
	}
}

