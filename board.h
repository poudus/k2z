

// STATIC SINGLETON

#ifndef __BOARD__
#define __BOARD__

#define	NB_MAX_SLOTS	1024
#define	NB_MAX_STEPS	4096
#define	PATH_MAX_LENGTH	16


#define ONE_MILLION		1000000
#define ONE_BILLION		1000000000

#define MAX_PATH_PER_SLOT	2 * ONE_MILLION
#define MAX_PATH_PER_STEP	1 *ONE_MILLION
#define MAX_PATHS		10 * ONE_MILLION


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


void printSlot(SLOT *pslot);
void printStep(STEP *pstep);
int find_slot(BOARD *board, char *pslot);

bool init_board(BOARD *board, int width, int height, int depth, int min_direction);

void free_board(BOARD *board);

#endif


