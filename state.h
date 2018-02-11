

#include "board.h"


// ------------
// DYNAMIC PLAY
// ------------

typedef struct
{
unsigned char	status, pegs, links, spread;
} WAVE;


typedef struct
{
unsigned short	idx;
unsigned int	count;
double		weight;
} TRACK;

typedef struct
{
int		waves;
unsigned int	pegs, links, spread;
unsigned int	slot[NB_MAX_SLOTS];
double		score;
} LAMBDA;


typedef struct
{
int		waves, count, pegs, links, tracks;
unsigned short	peg[32], link[32];
WAVE		*wave;
LAMBDA		lambda[PATH_MAX_LENGTH];
TRACK		track[NB_MAX_SLOTS];
} FIELD;

typedef struct
{
FIELD		horizontal, vertical;
double		score;
} STATE;


typedef struct
{
int	slot, steps;
char	orientation;
int	step[8];
} MOVE;


long init_state(STATE *state, unsigned int hpaths, unsigned int vpaths);
void clone_state(STATE *from, STATE *to);
int state_move(BOARD *board, STATE *state, MOVE *move);
void free_state(STATE *state);
double score_state(BOARD *board, STATE *state, double lambda_decay, double weight_pegs, double weight_links, double weight_spread);


