

// STATIC SINGLETON

#ifndef __BOARD__
#define __BOARD__

#define	NB_MAX_SLOTS	256
#define	NB_MAX_STEPS	512
#define	PATH_MAX_LENGTH	16


#define ONE_MILLION		1000000
#define ONE_BILLION		1000000000

#define MAX_PATH_PER_SLOT	3 * ONE_MILLION
#define MAX_PATH_PER_STEP	1 *ONE_MILLION
#define MAX_PATHS		30 * ONE_MILLION


typedef struct
{
unsigned char	x,y;
char		code[4];
unsigned short	idx, neighbors;
unsigned short	neighbor[8], link[8];
unsigned int	*hpath, *vpath, hpaths, vpaths;
//unsigned char	*hrank, *vrank;
} SLOT;

typedef struct
{
unsigned short	from, to, idx, cuts, sx, sy, sign, xmin, xmax, ymin, ymax, x1, y1, x2, y2;
unsigned short	cut[10];
char		code[8];
unsigned int	*hpath, *vpath, hpaths, vpaths;
//unsigned char	*hrank, *vrank;
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


unsigned int SumSlotPaths(BOARD *board, unsigned int* max);
unsigned int SumStepPaths(BOARD *board, unsigned int* max);

int SearchPathSlot(PATH *path, unsigned short slot);
int SearchPathStep(PATH *path, unsigned short step);

void printSlot(BOARD *board, SLOT *pslot);
void debugStep(STEP *pstep);
void printStep(BOARD *board, int step);
void printPath(BOARD *board, PATH *path);
int find_slot(BOARD *board, char *pslot);
int find_xy(BOARD *board, int sx, int sy);

unsigned long init_board(BOARD *board, int width, int height, int depth, int min_direction);

void free_board(BOARD *board);


bool FlipSize(char *x, int size);
char Flip(int size, char c, bool f);
char *FlipMoves(int size, char *moves, int depth, bool *hf, bool *vf);

#endif


