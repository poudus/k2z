

#include "board.h"

//#define __LINK1__

// ------------
// DYNAMIC PLAY
// ------------

extern bool	debug_state;

typedef struct
{
unsigned char	status, pegs, links, zeta;
#ifdef __ZETA__
unsigned char	slot[16], step[16];
#endif
} WAVE;


typedef struct
{
unsigned short	idx;
double		value, weight;
} TRACK;

typedef struct
{
int		waves;
unsigned int	pegs, links, zeta;
unsigned int	slot[NB_MAX_SLOTS];
unsigned int	step[NB_MAX_STEPS];
double        score, weight, spread, cross;
} LAMBDA;


typedef struct
{
int		waves, count, pegs, links;
unsigned short	peg[32], link[32];
WAVE		*wave;
LAMBDA		lambda[PATH_MAX_LENGTH];
TRACK		track[NB_MAX_SLOTS];
#ifdef __LINK1__
unsigned char	link1[NB_MAX_STEPS];
//unsigned char	wlink1[NB_MAX_STEPS];
#endif
} FIELD;

typedef struct
{
FIELD		horizontal, vertical;
#ifdef __LINK1__
unsigned char	link[NB_MAX_STEPS];
#endif
} STATE;


typedef struct
{
int	slot, steps;
char	orientation;
int	step[8];
} MOVE;


long init_state(STATE *state, unsigned int hpaths, unsigned int vpaths, bool alloc_wave);
void clone_state(STATE *from, STATE *to, bool alloc_waves);
int state_move(BOARD *board, STATE *state, MOVE *move);
void free_state(STATE *state);
double eval_state(BOARD *board, FIELD *player, FIELD *opponent, double lambda_decay, double wpegs, double wlinks, double wzeta);

void lambda_field(BOARD *board, GRID *grid, FIELD *field, bool init_tracks);
void printLambdaWave(GRID *grid, FIELD *field, int lambda);

void build_field_tracks(BOARD *board, FIELD *player);
void build_state_tracks(BOARD *board, STATE *state);

int state_moves(BOARD *board, STATE *state, char orientation, double opponent_decay, TRACK *move);

bool winning_field(FIELD *pfield);
bool empty_field(FIELD *pfield);
char* state_signature(BOARD *board, STATE *state, char *signature);
bool find_move(STATE *state, unsigned short sid);

double eval_orientation(BOARD *board, STATE *state, char orientation, double lambda_decay, double wpegs, double wlinks, double wzeta, bool btracks);

