
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>


#include "board.h"
#include "state.h"


double duration(struct timeval *t1, struct timeval *t2)
{
double elapsedTime = 0.0;

	// compute and print the elapsed time in millisec
	elapsedTime = (t2->tv_sec - t1->tv_sec) * 1000.0;      // sec to ms
	elapsedTime += (t2->tv_usec - t1->tv_usec) / 1000.0;   // us to ms

	return elapsedTime;
}

int move(BOARD *board, STATE *state_from, STATE *state_to, unsigned short slot, char orientation)
{
struct timeval t0, t_end;

	gettimeofday(&t0, NULL);
	clone_state(state_from, state_to);
	//printf("init.clone  %6.2f ms\n", duration(&t_init_s0, &t_clone));

	MOVE move;
	move.slot = slot;
	move.orientation = orientation;
	move.steps = 0;
	int complexity = state_move(board, state_to, &move);
	gettimeofday(&t_end, NULL);

	printf("move   %6.2f ms  complexity = %9d   (%d+%d %6.2f%%, %d+%d %6.2f%%)\n", duration(&t0, &t_end), complexity,
		state_to->horizontal.pegs, state_to->horizontal.links, 100.0 * state_to->horizontal.count / state_to->horizontal.waves,
		state_to->vertical.pegs, state_to->vertical.links, 100.0 * state_to->vertical.count / state_to->vertical.waves);
}

void eval_orientation(BOARD *board, STATE *state, char orientation, double lambda_decay,
		double weight_pegs, double weight_links, double weight_spread, bool bmoves)
{
struct timeval t0, t_end, t_lambda_field;
FIELD *player = NULL, *opponent = NULL;

	gettimeofday(&t0, NULL);
	lambda_field(&board->horizontal, &state->horizontal, bmoves);
	lambda_field(&board->vertical, &state->vertical, bmoves);
	gettimeofday(&t_lambda_field, NULL);
	if (orientation == 'H')
	{
		player = &state->horizontal;
		opponent = &state->vertical;
	}
	else
	{
		player = &state->vertical;
		opponent = &state->horizontal;
	}
	double score = eval_state(board, player, opponent, lambda_decay, weight_pegs, weight_links, weight_spread);

	if (bmoves) build_state_tracks(board, state);

	gettimeofday(&t_end, NULL);

	printf("eval %c = %6.2f %%   (%6.2f ms, LF = %6.2f)\n", orientation, score, duration(&t0, &t_end), duration(&t0, &t_lambda_field));
}

int parse_slot(BOARD *board, char *pslot)
{
	if (strlen(pslot) == 2 && isupper((int)pslot[0]) && isupper((int)pslot[1]))
		return find_slot(board, pslot);
	else return atoi(pslot);
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
	unsigned long ul_allocated = init_board(&board, width, height, 10, -1);
	if (ul_allocated > 0)
	{
		gettimeofday(&t_init_board, NULL);
		printf("init.board %6.2f ms        size = %4.1f GB  complexity = %10d\n", duration(&t0, &t_init_board), (double)ul_allocated / 1000, board.horizontal.paths+board.vertical.paths);

		STATE state_h, state_v, *current_state;
		current_state = &state_h;
		long size_s0 = init_state(&state_h, board.horizontal.paths, board.vertical.paths);
		gettimeofday(&t_init_s0, NULL);
		printf("init.s0     %6.2f ms        size = %4ld MB\n", duration(&t_init_board, &t_init_s0), size_s0 / ONE_MILLION);

		char command[256], action[256], parameters[256], orientation;
		int move_number = 0;
		printf("k2z> ");
		orientation = 'H';
		while (1)
		{
			fgets(command, 256, stdin);
			if (strlen(command) > 0)
			{
				sscanf(command, "%s %s", action, parameters);
				if (strcmp("quit", action) == 0 || strcmp("exit", action) == 0)
					break;
				else if (strcmp("move", action) == 0)
				{
					int slot = parse_slot(&board, parameters);
					if (orientation == 'H')
					{
						move(&board, &state_h, &state_v, slot, orientation);
						orientation = 'V';
						current_state = &state_v;
					}
					else
					{
						move(&board, &state_v, &state_h, slot, orientation);
						orientation = 'H';
						current_state = &state_h;
					}
					move_number++;
				}
				else if (strcmp("eval", action) == 0)
				{
					double lambda_decay = 1.0;
					if (strlen(parameters) > 0)
						lambda_decay = atof(parameters);
					eval_orientation(&board, current_state, orientation, lambda_decay, 0.0, 0.0, 0.0, false);
				}
				else if (strcmp("evalm", action) == 0)
				{
					double lambda_decay = 1.0;
					if (strlen(parameters) > 0)
						lambda_decay = atof(parameters);
					eval_orientation(&board, current_state, orientation, lambda_decay, 0.0, 0.0, 0.0, true);
				}
				else if (strcmp("moves", action) == 0)
				{
					double opponent_decay = 1.0;
					if (strlen(parameters) > 0)
						opponent_decay = atof(parameters);
					TRACK move[256];
					int nb_moves = state_moves(&board, current_state, orientation, opponent_decay, &move[0]);
	for (int m = 0 ; m < 10 ; m++)
		printf("%03d: %3d/%s  %6.2f %%\n", m, move[m].idx, board.slot[move[m].idx].code, move[m].weight);
				}
				else if (strcmp("slot", action) == 0)
				{
					int slot = parse_slot(&board, parameters);
					printSlot(&board.slot[slot]);
				}
				else if (strcmp("step", action) == 0)
				{
					int step = atoi(parameters);
					printStep(&board.step[step]);
				}
				else if (strcmp("lambda", action) == 0)
				{
					for (int lambda = 0 ; lambda < PATH_MAX_LENGTH ; lambda++)
					{
if (current_state->horizontal.lambda[lambda].waves > 0 || current_state->vertical.lambda[lambda].waves > 0)
	printf("lambda %02d:  [%6.4f] x  %6.2f %%  (%8d+%8d+%8d)    [%6.4f] x  %6.2f %%  (%8d+%8d+%8d)\n",
		lambda,
		current_state->horizontal.lambda[lambda].weight,
		current_state->horizontal.lambda[lambda].score, current_state->horizontal.lambda[lambda].waves,
		current_state->horizontal.lambda[lambda].pegs, current_state->horizontal.lambda[lambda].links,
		current_state->vertical.lambda[lambda].weight,
		current_state->vertical.lambda[lambda].score, current_state->vertical.lambda[lambda].waves,
		current_state->vertical.lambda[lambda].pegs, current_state->vertical.lambda[lambda].links);
					}
				}
				else if (strcmp("reset", action) == 0)
				{
					current_state = &state_h;
					init_state(&state_h, board.horizontal.paths, board.vertical.paths);
					move_number = 0;
					orientation = 'H';
				}
				action[0] = parameters[0] = 0;
			}
			printf("k2z> ");
		}
		printf("bye.\n");
		free_board(&board);
	}
	else
	{
		printf("error.init\n");
	}
}
