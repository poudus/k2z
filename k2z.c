
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
unsigned short	idx;
unsigned short	neighbor[8];
} SLOT;

typedef struct
{
unsigned short	from, to, idx;
unsigned short	cut[8];
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

	board->slots = 0;
	for (int x = 0 ; x < width ; x++)
	{
		for (int y = 0 ; y < height ; y++)
		{
			board->slot[board->slots].x = x;
			board->slot[board->slots].y = y;
			board->slot[board->slots].idx = board->slots;
	
//char		code[4];
//unsigned short	neighbor[8];

			board->slots++;
		}
	}
	board->steps = 4 * board->slots;

	int sz = 10000000;
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
	}
	//printf("end h\n");
	//fflush(stdout);
	board->vertical.paths = sz;
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
	}
	//printf("end v\n");
	//fflush(stdout);

	return true;
}

// ------------
// DYNAMIC PLAY
// ------------


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
		WAVE *pw = malloc(sz * sizeof(WAVE));
		for (int w = 0 ; w < sz ; w++)
		{
			pw[w].path = 0;
			pw[w].pegs = 0;
			pw[w].links = 0;
		}
		gettimeofday(&t_init_wave, NULL);
		printf("init.wave %6.2f ms\n", duration(&t_init_board, &t_init_wave));
	}
	else
	{
		printf("error.init\n");
	}
}

