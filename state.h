

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
int		value;
double		score, weight;
} PARAMETER;

typedef struct
{
int		count;
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
int		waves, count, pegs, links;
unsigned short	peg[32], link[32];
WAVE		*wave;
LAMBDA		lambda[PATH_MAX_LENGTH];
} FIELD;

typedef struct
{
FIELD		horizontal, vertical;
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


