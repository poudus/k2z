
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
unsigned short	idx, neighbors, hpaths, vpaths;
unsigned short	neighbor[8];
unsigned int	*hpath, *vpath;
} SLOT;

typedef struct
{
unsigned short	from, to, idx, cuts, hpaths, vpaths, sx, sy, sign;
unsigned short	cut[24];
unsigned int	*hpath, *vpath;
} STEP;

typedef struct
{
unsigned int	slots, steps;
unsigned short	slot[PATH_MAX_LENGTH];
unsigned short	step[PATH_MAX_LENGTH];
} PATH;

typedef struct
{
unsigned int	paths;
PATH		*path;
} GRID;

typedef struct
{
int		width, height, slots, steps;
SLOT		slot[NB_MAX_SLOTS];
STEP		step[NB_MAX_STEPS];
GRID		horizontal, vertical;
} BOARD;

bool init_board(BOARD *board, int width, int height, int depth, int min_direction)
{
	bzero(board, sizeof(BOARD));
	board->width = width;
	board->height = height;

	// slots
	board->slots = 0;
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
				board->slot[board->slots].hpath = NULL;
				board->slot[board->slots].vpaths = 0;
				board->slot[board->slots].vpath = NULL;
				board->slots++;
			}
		}
	}
	// neighbors
	board->steps = 0;
	for (int s1 = 0 ; s1 < board->slots -1 ; s1++)
	{
		for (int s2 = s1 +1 ; s2 < board->slots ; s2++)
		{
			if ((board->slot[s2].x - board->slot[s1].x)*(board->slot[s2].x - board->slot[s1].x) + (board->slot[s2].y - board->slot[s1].y)*(board->slot[s2].y - board->slot[s1].y) == 5)
			{
				board->slot[s1].neighbor[board->slot[s1].neighbors] = board->slot[s2].idx;
				board->slot[s1].neighbors++;

				board->slot[s2].neighbor[board->slot[s2].neighbors] = board->slot[s1].idx;
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
				board->step[board->steps].hpath = NULL;
				board->step[board->steps].vpaths = 0;
				board->step[board->steps].vpath = NULL;
				board->steps++;
			}
		}
	}
	int max_neighbors = 0;
	for (int s = 0 ; s < board->slots ; s++)
	{
		if (board->slot[s].neighbors > max_neighbors) max_neighbors = board->slot[s].neighbors;
	}
	printf("debug.max_neighbors = %d\n", max_neighbors);
	// cuts
	for (int s1 = 0 ; s1 < board->steps ; s1++)
	{
		for (int s2 = 0 ; s2 < board->steps ; s2++)
		{
			int sqd = (board->step[s1].sx - board->step[s2].sx) * (board->step[s1].sx - board->step[s2].sx) + (board->step[s1].sy - board->step[s2].sy) * (board->step[s1].sy - board->step[s2].sy);
			if (s1 != s2 && (sqd < 4 || (sqd == 4 || board->step[s1].sign != board->step[s2].sign)))
			{
				board->step[s1].cut[board->step[s1].cuts] = s2;
				board->step[s1].cuts++;

				//board->step[s2].cut[board->step[s2].cuts] = s1;
				//board->step[s2].cuts++;
			}
		}
	}
	int max_cuts = 0;
	for (int s = 0 ; s < board->steps ; s++)
	{
		if (board->step[s].cuts > max_cuts) max_cuts = board->step[s].cuts;
	}
	printf("debug.max_cuts = %d\n", max_cuts);
	printf("debug.slots = %d\n", board->slots);
	printf("debug.steps = %d\n", board->steps);

	// init arrays
	int sz = 2048000;
	for (int s = 0 ; s < board->slots ; s++)
	{
		board->slot[s].hpath = malloc(sz * sizeof(unsigned int));
		board->slot[s].hpaths = 1;
		board->slot[s].vpath = malloc(sz * sizeof(unsigned int));
		board->slot[s].vpaths = 1;
	}
	for (int s = 0 ; s < board->steps ; s++)
	{
		board->step[s].hpath = malloc(sz * sizeof(unsigned int));
		board->step[s].hpaths = 1;
		board->step[s].vpath = malloc(sz * sizeof(unsigned int));
		board->step[s].vpaths = 1;
	}

	/*int sz = 10000000;
	srand(time(NULL));
	board->horizontal.paths = sz;
	board->horizontal.path = malloc(sz * sizeof(PATH));
	printf("debug.hpath = %p  (%lu mb)\n", board->horizontal.path, sz * sizeof(PATH) / 1000000);
	for (int i = 0 ; i < sz ; i++)
	{
		board->horizontal.path[i].slots = 0;
		board->horizontal.path[i].steps = 0;

		for (int islot = 0 ; islot < PATH_MAX_LENGTH ; islot++)
			board->horizontal.path[i].slot[islot] = rand() % board->slots;
		for (int istep = 0 ; istep < PATH_MAX_LENGTH ; istep++)
			board->horizontal.path[i].step[istep] = rand() % board->steps;
	}*/
	//printf("end h\n");
	//fflush(stdout);
	/*board->vertical.paths = sz;
	board->vertical.path = malloc(sz * sizeof(PATH));
	printf("debug.vpath = %p  (%lu mb)\n", board->vertical.path, sz * sizeof(PATH) / 1000000);
	if (board->vertical.path != NULL)
	{
		for (int j = 0 ; j < sz ; j++)
		{
			board->vertical.path[j].slots = 0;
			board->vertical.path[j].steps = 0;

			for (int islot = 0 ; islot < PATH_MAX_LENGTH ; islot++)
				board->vertical.path[j].slot[islot] = rand() % board->slots;
			for (int istep = 0 ; istep < PATH_MAX_LENGTH ; istep++)
				board->vertical.path[j].step[istep] = rand() % board->steps;
		}
	}*/
	//printf("end v\n");
	//fflush(stdout);

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
unsigned short	slot;
unsigned int	count;
double		weight;
} TRACK;

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
	if (init_board(&board, width, height, 16, 1))
	{
		gettimeofday(&t_init_board, NULL);
		printf("init.board %6.2f ms\n", duration(&t0, &t_init_board));

		int sz = 10000000;
		QW *pqw = malloc(sz * sizeof(QW));
		for (int w = 0 ; w < sz ; w++)
		{
			pqw[w].status = ' ';
			pqw[w].pegs = 0;
			pqw[w].links = 0;
			pqw[w].spread = 0;
		}
		srand(time(NULL));
		for (int rd = 0 ; rd < 1000000 ; rd++)
		{
			int idx = rand() % sz;
			pqw[idx].status = 'X';
		}
		int nb = 0;
		for (int z = 0 ; z < sz ; z++)
		{
			if (pqw[z].status == ' ') nb++;
		}
		gettimeofday(&t_init_wave, NULL);
		printf("init.qw %6.2f ms  nb = %d\n", duration(&t_init_board, &t_init_wave), nb);
	}
	else
	{
		printf("error.init\n");
	}
}

