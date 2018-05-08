

#include "board.h"

// ------------
// DYNAMIC PLAY
// ------------

extern bool	debug_state;

typedef struct
{
unsigned char	status, pegs, links, weakness;
unsigned char	slot[16], step[16];
} WAVE;


typedef struct
{
unsigned short	idx;
double		value, weight;
} TRACK;

typedef struct
{
int		waves;
unsigned int	pegs, links, weakness;
unsigned int	slot[NB_MAX_SLOTS];
unsigned int	step[NB_MAX_STEPS];
double		score, weight;
} LAMBDA;


typedef struct
{
int		waves, count, pegs, links;
unsigned short	peg[32], link[32];
WAVE		*wave;
LAMBDA		lambda[PATH_MAX_LENGTH];
TRACK		track[NB_MAX_SLOTS];
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
double eval_state(BOARD *board, FIELD *player, FIELD *opponent, double lambda_decay, double w1, double w2);

void lambda_field(BOARD *board, GRID *grid, FIELD *field, bool init_slots, bool bh);
void printLambdaWave(GRID *grid, FIELD *field, int lambda);

void build_field_tracks(BOARD *board, FIELD *player);
void build_state_tracks(BOARD *board, STATE *state);

int state_moves(BOARD *board, STATE *state, char orientation, double opponent_decay, TRACK *move);



