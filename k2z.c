
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
		double w1, double w2, bool btracks)
{
struct timeval t0, t_end, t_lambda_field;
FIELD *player = NULL, *opponent = NULL;

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

	gettimeofday(&t0, NULL);
	lambda_field(board, &board->horizontal, &state->horizontal, btracks);
	lambda_field(board, &board->vertical, &state->vertical, btracks);
	gettimeofday(&t_lambda_field, NULL);
	double score = eval_state(board, player, opponent, lambda_decay, w1, w2);

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
int depth = 2, width = 16, height = 16, max_moves = 5, slambda = 10, sdirection = -1;
double ld, od, dw1 = 0.0, dw2 = 0.0;
double lambda_decay = 0.8, opponent_decay = 0.8;
struct timeval t0, t_init_board, t_init_wave, t_init_s0, t_clone, t_move;
BOARD board;
TRACK zemoves[256];

	printf("K2Z.engine-version=0.1\n");
	gettimeofday(&t0, NULL);
	srand(t0.tv_sec);
	if (argc > 1 && strlen(argv[1]) == 5)
	{
		char lsize[8];
		strcpy(lsize, argv[1]);
		lsize[2] = 0;
		width = atoi(&lsize[0]);
		height = atoi(&lsize[3]);
		printf("init.size=%dx%d\n", width, height);

		if (argc > 2)
		{
			slambda = atoi(argv[2]);
			if (argc > 3)
				sdirection = atoi(argv[3]);
		}
	}
	else
	{
		printf("error.invalid-size-parameter\n");
		return -1;
	}
	unsigned long ul_allocated = init_board(&board, width, height, slambda, sdirection);
	if (ul_allocated > 0)
	{
		gettimeofday(&t_init_board, NULL);
		printf("init.board %6.2f ms        size = %4.1f GB  complexity = %10d\n", duration(&t0, &t_init_board), (double)ul_allocated / 1000, board.horizontal.paths+board.vertical.paths);

		STATE state_h, state_v, *current_state;
		current_state = &state_h;
		long size_s0 = init_state(&state_h, board.horizontal.paths, board.vertical.paths);
		gettimeofday(&t_init_s0, NULL);
		printf("init.s0    %6.2f ms        size = %4ld MB\n", duration(&t_init_board, &t_init_s0), size_s0 / ONE_MILLION);

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
				int len_parameter = strlen(parameters);
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
						strcpy(parameters, "FFFJHGGHJFGADGIBBFGLAH");
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
					eval_orientation(&board, current_state, orientation, ld, dw1, dw2, false);
				}
				else if (strcmp("tracks", action) == 0)
				{
					if (strlen(parameters) > 0)
						ld = atof(parameters);
					else
						ld = lambda_decay;
					eval_orientation(&board, current_state, orientation, ld, dw1, dw2, true);
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
					eval_orientation(&board, current_state, orientation, ld, dw1, dw2, true);
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
					eval_orientation(&board, current_state, orientation, ld, dw1, dw2, false);
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
					printf("max_moves      =  %d\n", max_moves);
					printf("dw1            = %6.3f\n", dw1);
					printf("dw2            = %6.3f\n", dw2);
				}
				else if (strcmp("position", action) == 0)
				{
					printf("moves = %s  (%ld)\n", current_game_moves, strlen(current_game_moves)/2);
					printf("trait = %c\n", orientation);
				}
				else if (strcmp("slot", action) == 0)
				{
					int slot = parse_slot(&board, parameters);
					printSlot(&board, &board.slot[slot]);
				}
				else if (strcmp("step", action) == 0)
				{
					int step = atoi(parameters);
					printStep(&board, step);
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
								printf("%3d/%4s %6d\n", s, board.slot[s].code, current_state->horizontal.lambda[il].slot[s]);
							else if (bv && current_state->vertical.lambda[il].slot[s] > 0)
								printf("%3d/%4s %6d\n", s, board.slot[s].code, current_state->vertical.lambda[il].slot[s]);
						}
						for (int s = 0 ; s < board.steps ; s++)
						{
							if (bh && current_state->horizontal.lambda[il].step[s] > 0)
								printf("%3d/%4s %6d\n", s, board.step[s].code, current_state->horizontal.lambda[il].step[s]);
							else if (bv && current_state->vertical.lambda[il].step[s] > 0)
								printf("%3d/%4s %6d\n", s, board.step[s].code, current_state->vertical.lambda[il].step[s]);
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
				else if (strcmp("path", action) == 0 && len_parameter >= 2)
				{
					bool bh = (parameters[len_parameter-1] == 'H');
					bool bv = (parameters[len_parameter-1] == 'V');
					parameters[len_parameter-1] = 0;
					int ipath = atoi(parameters);

					if (bh)
						printPath(&board, &board.horizontal.path[ipath]);
					else if (bv)
						printPath(&board, &board.vertical.path[ipath]);
				}
				else if (strcmp("wave", action) == 0 && len_parameter >= 2)
				{
					bool bh = (parameters[len_parameter-1] == 'H');
					bool bv = (parameters[len_parameter-1] == 'V');
					parameters[len_parameter-1] = 0;
					int iw = atoi(parameters);

					WAVE *pw = NULL;
					PATH *ph = NULL;
					if (bh && iw < current_state->horizontal.waves)
					{
						pw = &current_state->horizontal.wave[iw];
						ph = &board.horizontal.path[iw];
					}
					else if (bv && iw < current_state->vertical.waves)
					{
						pw = &current_state->vertical.wave[iw];
						ph = &board.vertical.path[iw];
					}

					char buff1[32], buff2[32];
					strncpy(buff1, pw->slot, PATH_MAX_LENGTH);
					strncpy(buff2, pw->step, PATH_MAX_LENGTH);
					buff1[ph->slots] = 0;
					buff2[ph->steps] = 0;
					printf("s='%C'  p=%d  l=%d  w=%d  slots=[%s]  steps=[%s]\n", pw->status, pw->pegs, pw->links, pw->weakness, buff1, buff2);
				}
				else if (strcmp("waves", action) == 0 && len_parameter >= 2)
				{
					bool bh = (parameters[len_parameter-1] == 'H');
					bool bv = (parameters[len_parameter-1] == 'V');
					parameters[len_parameter-1] = 0;
					int lw = atoi(parameters);

					if (bh)
						printLambdaWave(&board.horizontal, &current_state->horizontal, lw);
					else if (bv)
						printLambdaWave(&board.vertical, &current_state->vertical, lw);

				}
				else if (strcmp("reset", action) == 0)
				{
					current_state = &state_h;
					init_state(&state_h, board.horizontal.paths, board.vertical.paths);
					move_number = 0;
					orientation = 'H';
					current_game_moves[0] = 0;
				}
				else if (strcmp("settings", action) == 0)
				{
					printf("max_lambda = %2d\n", slambda);
					printf("direction  = %2d\n", sdirection);
				}
				else if (strcmp("help", action) == 0)
				{
					printf("\tmove XY\n");
					printf("\tplay\n");
					printf("\teval <lambda-decay>\n");
					printf("\ttracks <lambda-decay>\n");
					printf("\tmoves <opponent-decay>\n");
					printf("\tlambda\n");
					printf("\tlambda <lambda><H/V>\n");
					printf("\tposition\n");
					printf("\tparameters\n");
					printf("\tpath <pid><H/V>\n");
					//printf("\tpaths <lambda><H/V>\n");
					printf("\twave <wid><H/V>\n");
					printf("\twaves <lambda><H/V>\n");
					printf("\tslot s\n");
					printf("\tstep s\n");
					printf("\tdebug Y/N\n");
					printf("\tdefault\n");
					printf("\tsettings\n");
					printf("\treset\n");
					printf("\tquit\n");
					printf("\texit\n");
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
