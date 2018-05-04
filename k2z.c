
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

	printf("move %c %s  %6.2f ms  complexity = %9d   (%d+%d %6.2f%%, %d+%d %6.2f%%)\n", orientation, board->slot[slot].code,
		duration(&t0, &t_end), complexity,
		state_to->horizontal.pegs, state_to->horizontal.links, 100.0 * state_to->horizontal.count / state_to->horizontal.waves,
		state_to->vertical.pegs, state_to->vertical.links, 100.0 * state_to->vertical.count / state_to->vertical.waves);
}

void eval_orientation(BOARD *board, STATE *state, char orientation, double lambda_decay,
		double weight_pegs, double weight_links, double weight_spread, bool btracks)
{
struct timeval t0, t_end, t_lambda_field;
FIELD *player = NULL, *opponent = NULL;

	gettimeofday(&t0, NULL);
	lambda_field(&board->horizontal, &state->horizontal, btracks);
	lambda_field(&board->vertical, &state->vertical, btracks);
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

	if (btracks) build_state_tracks(board, state);

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
int depth = 2, width = 16, height = 16, max_moves = 5;
double contempt = 10.0, ld, od;
double lambda_decay = 0.8, opponent_decay = 0.8;
struct timeval t0, t_init_board, t_init_wave, t_init_s0, t_clone, t_move;
BOARD board;
TRACK zemoves[256];

	printf("K2Z.engine-version=0.1\n");
	gettimeofday(&t0, NULL);
	srand(t0.tv_sec);
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

		char command[256], action[256], parameters[256], current_game_moves[64], orientation;
		int move_number = 0;
		printf("k2z> ");
		orientation = 'H';
		current_game_moves[0] = 0;
		while (1)
		{
			fgets(command, 256, stdin);
			if (strlen(command) > 0)
			{
				sscanf(command, "%s %s", action, parameters);
				if (strcmp("quit", action) == 0 || strcmp("exit", action) == 0)
					break;
				else if (strcmp("debug", action) == 0)
				{
					if (strlen(parameters) > 0 && parameters[0] == 'Y')
						debug_state = true;
					else
						debug_state = false;
				}
				else if (strcmp("move", action) == 0 || strcmp("default", action) == 0)
				{
					if (strcmp("default", action) == 0)
					{
						//strcpy(parameters, "FFGLHGFJDGJDBHIBAFGABBKKJFIGLG");
						strcpy(parameters, "FFFJHGGHJFGADGGCBFGLAH");
					}
					int moves = strlen(parameters)/2;
					for (int imove = 0 ; imove < moves ; imove++)
					{
						char smove[4];
						smove[0] = parameters[2*imove];
						smove[1] = parameters[2*imove+1];
						smove[2] = 0;
						int slot = parse_slot(&board, smove);
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
						strcat(current_game_moves, smove);
						move_number++;
					}
				}
				else if (strcmp("eval", action) == 0)
				{
					if (strlen(parameters) > 0)
						ld = atof(parameters);
					else
						ld = lambda_decay;
					eval_orientation(&board, current_state, orientation, ld, 0.0, 0.0, 0.0, false);
				}
				else if (strcmp("tracks", action) == 0)
				{
					if (strlen(parameters) > 0)
						ld = atof(parameters);
					else
						ld = lambda_decay;
					eval_orientation(&board, current_state, orientation, ld, 0.0, 0.0, 0.0, true);
				}
				else if (strcmp("moves", action) == 0)
				{
					if (strlen(parameters) > 0)
						od = atof(parameters);
					else
						od = opponent_decay;
					int nb_moves = state_moves(&board, current_state, orientation, od, &zemoves[0]);
					for (int m = 0 ; m < max_moves ; m++)
					printf("%03d: %3d/%s  %6.2f %%\n", m, zemoves[m].idx, board.slot[zemoves[m].idx].code, zemoves[m].weight);
				}
				else if (strcmp("play", action) == 0)
				{
					eval_orientation(&board, current_state, orientation, ld, 0.0, 0.0, 0.0, true);
					int nb_moves = state_moves(&board, current_state, orientation, od, &zemoves[0]);
					int idx_move = rand() % max_moves;
					TRACK *pmove = &zemoves[idx_move];
					if (orientation == 'H')
					{
						move(&board, &state_h, &state_v, pmove->idx, orientation);
						orientation = 'V';
						current_state = &state_v;
					}
					else
					{
						move(&board, &state_v, &state_h, pmove->idx, orientation);
						orientation = 'H';
						current_state = &state_h;
					}
					strcat(current_game_moves, board.slot[pmove->idx].code);
					move_number++;

					printf("move %d/%d/%s played\n", idx_move, pmove->idx, board.slot[pmove->idx].code);
					eval_orientation(&board, current_state, orientation, ld, 0.0, 0.0, 0.0, false);
				}
				else if (strcmp("parameter", action) == 0)
				{
					if (strlen(parameters) > 0)
					{
						int plen = strlen(parameters);
						char *pvalue = NULL;
						for (int ic = 0 ; ic < plen ; ic++)
						{
							if (parameters[ic] == '=')
							{
								parameters[ic] = 0;
								pvalue = &parameters[ic+1];
							}
						}
						if (*pvalue != 0)
						{
							double dvalue = atof(pvalue);
							int ivalue = atoi(pvalue);
							bool bp = true;
							if (strcmp(parameters, "ld") == 0) lambda_decay = dvalue;
							else if (strcmp(parameters, "od") == 0) opponent_decay = dvalue;
							else if (strcmp(parameters, "moves") == 0) max_moves = ivalue;
							else bp = false;
							if (bp) printf("parameter %s set to %6.3f\n", parameters, dvalue);
						}
					}
				}
				else if (strcmp("parameters", action) == 0)
				{
					printf("lambda_decay   = %6.3f\n", lambda_decay);
					printf("opponent_decay = %6.3f\n", opponent_decay);
					printf("max_moves      = %3d\n", max_moves);
				}
				else if (strcmp("position", action) == 0)
				{
					printf("moves = %s  (%ld)\n", current_game_moves, strlen(current_game_moves)/2);
					printf("trait = %c\n", orientation);
				}
				else if (strcmp("slot", action) == 0)
				{
					int slot = parse_slot(&board, parameters);
					printSlot(&board.slot[slot]);
				}
				else if (strcmp("step", action) == 0)
				{
					int step = atoi(parameters);
					traceStep(&board, step);
				}
				else if (strcmp("lambda", action) == 0)
				{
					if (strlen(parameters) > 0)
					{
						bool bh = (parameters[1] == 'H');
						bool bv = (parameters[1] == 'V');
						parameters[1] = 0;
						int il = atoi(parameters);
						for (int s = 0 ; s < board.slots ; s++)
						{
							if (bh && current_state->horizontal.lambda[il].slot[s] > 0)
								printf("%3d/%s %6d\n", s, board.slot[s].code, current_state->horizontal.lambda[il].slot[s]);
							else if (bv && current_state->vertical.lambda[il].slot[s] > 0)
								printf("%3d/%s %6d\n", s, board.slot[s].code, current_state->vertical.lambda[il].slot[s]);
						}
					}
					else
					{
						for (int lambda = 0 ; lambda < PATH_MAX_LENGTH ; lambda++)
						{
if (current_state->horizontal.lambda[lambda].waves > 0 || current_state->vertical.lambda[lambda].waves > 0)
	printf("lambda %02d:  [%6.4f] x    %6.2f %%  = (%8d+%8d+%8d-%8d)   /  %6.2f %%  = (%8d+%8d+%8d-%8d)  : %02d\n",
		lambda,
		current_state->horizontal.lambda[lambda].weight,
		current_state->horizontal.lambda[lambda].score, current_state->horizontal.lambda[lambda].waves,
		current_state->horizontal.lambda[lambda].pegs, current_state->horizontal.lambda[lambda].links, current_state->horizontal.lambda[lambda].weakness,
		current_state->vertical.lambda[lambda].score, current_state->vertical.lambda[lambda].waves,
		current_state->vertical.lambda[lambda].pegs, current_state->vertical.lambda[lambda].links, current_state->vertical.lambda[lambda].weakness,
		lambda);
						}
					}
				}
				else if (strcmp("reset", action) == 0)
				{
					current_state = &state_h;
					init_state(&state_h, board.horizontal.paths, board.vertical.paths);
					move_number = 0;
					orientation = 'H';
					current_game_moves[0] = 0;
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
