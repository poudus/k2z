
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <sys/time.h>

#include "board.h"
#include "state.h"
#include "database.h"


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
	//free_state(state_to);
	clone_state(state_from, state_to, false);
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

void eval_orientation(BOARD *board, STATE *state, char orientation, double lambda_decay, double wpegs, double wlinks, double wzeta, bool btracks)
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
	double score = eval_state(board, player, opponent, lambda_decay, wpegs, wlinks, wzeta);

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

double EloExpectedResult(double r1, double r2)
{
	return 1.0 / (1.0 + pow(10.0, (r2-r1)/400));
}

//
// main
//
int main(int argc, char* argv[])
{
int depth = 2, width = 16, height = 16, max_moves = 5, slambda = 10, sdirection = -1;
int hp_min = 0, hp_max = 50;
int vp_min = 0, vp_max = 50;
double wpegs = 0.0, wlinks = 0.0, wzeta = 0.0;
double lambda_decay = 0.8, opponent_decay = 0.8;
struct timeval t0, t_init_board, t_init_wave, t_init_s0, t_clone, t_move, t0_game, tend_game, t0_session, tend_session;
BOARD board;
TRACK zemoves[512];
char buffer_error[512], database_name[32];
PGconn *pgConn = NULL;

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
		sprintf(database_name, "k%s", argv[1]);

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
		printf("init.board %8.2f ms        size = %6ld MB  complexity = %10d\n", duration(&t0, &t_init_board), ul_allocated, board.horizontal.paths+board.vertical.paths);

		STATE state_h, state_v, *current_state;
		current_state = &state_h;
		long size_s0 = init_state(&state_h, board.horizontal.paths, board.vertical.paths, true);
		init_state(&state_v, board.horizontal.paths, board.vertical.paths, true);
		gettimeofday(&t_init_s0, NULL);
		printf("init.s0    %8.2f ms        size = %6ld MB\n", duration(&t_init_board, &t_init_s0), size_s0 / ONE_MILLION);

		char command[256], action[256], parameters[256], current_game_moves[256], orientation;
		int move_number = 0;
		orientation = 'H';
		current_game_moves[0] = 0;
		pgConn = pgOpenConn(database_name, "k2", "", buffer_error);
		printf("database connection     = %p %s\n", pgConn, database_name);
		printf("k2z> ");
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
					double ld = lambda_decay;
					if (strlen(parameters) > 0)
						ld = atof(parameters);
					eval_orientation(&board, current_state, orientation, ld, wpegs, wlinks, wzeta, false);
				}
				else if (strcmp("tracks", action) == 0)
				{
					double ld = lambda_decay;
					if (strlen(parameters) > 0)
						ld = atof(parameters);
					eval_orientation(&board, current_state, orientation, ld, wpegs, wlinks, wzeta, true);
				}
				else if (strcmp("moves", action) == 0)
				{
					double od = opponent_decay;
					if (strlen(parameters) > 0)
						od = atof(parameters);
					int nb_moves = state_moves(&board, current_state, orientation, od, &zemoves[0]);
					for (int m = 0 ; m < max_moves ; m++)
					printf("%03d: %3d/%s  %6.2f %%\n", m, zemoves[m].idx, board.slot[zemoves[m].idx].code, zemoves[m].weight);
				}
				else if (strcmp("play", action) == 0)
				{
					eval_orientation(&board, current_state, orientation, lambda_decay, wpegs, wlinks, wzeta, true);
					int nb_moves = state_moves(&board, current_state, orientation, opponent_decay, &zemoves[0]);
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
					printf("move %d: [%d] %d/%s played\n", move_number, idx_move, pmove->idx, board.slot[pmove->idx].code);
					eval_orientation(&board, current_state, orientation, lambda_decay, wpegs, wlinks, wzeta, false);
				}
				else if (strcmp("game", action) == 0)
				{
					bool end_of_game = false;
					char winner = ' ', reason = '?';
					gettimeofday(&t0_game, NULL);
					while (!end_of_game)
					{
						eval_orientation(&board, current_state, orientation, lambda_decay, wpegs, wlinks, wzeta, true);
						if (orientation == 'H')
						{
							if (empty_field(&state_h.horizontal))
							{
								printf("H resign\n");
								winner = 'V';
								reason = 'R';
								end_of_game = true;
							}
							else if (empty_field(&state_h.horizontal))
							{
								printf("H win\n");
								winner = 'H';
								reason = 'W';
								end_of_game = true;
							}
						}
						else
						{
							if (empty_field(&state_v.vertical))
							{
								printf("V resign\n");
								winner = 'H';
								reason = 'R';
								end_of_game = true;
							}
							else if (empty_field(&state_v.vertical))
							{
								printf("V win\n");
								winner = 'V';
								reason = 'W';
								end_of_game = true;
							}
						}
						if (!end_of_game)
						{
							int nb_moves = state_moves(&board, current_state, orientation, opponent_decay, &zemoves[0]);
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
							printf("move %d: [%d] %d/%s played\n", move_number, idx_move, pmove->idx, board.slot[pmove->idx].code);
							eval_orientation(&board, current_state, orientation, lambda_decay, wpegs, wlinks, wzeta, false);
							printf("---------- next move %c ----------\n", orientation);
						}
					}
					gettimeofday(&tend_game, NULL);
printf("========== winner %c  reason=%c  in %d moves : %s  duration = %5.2f s  average = %5.1f ms/move\n",
	winner, reason, move_number, current_game_moves, duration(&t0_game, &tend_game)/1000, duration(&t0_game, &tend_game)/move_number);

int game_id = insertGame(pgConn, 0, 0, current_game_moves, winner, reason, duration(&t0_game, &tend_game), 102, 201);
printf("saved as game_id = %d\n", game_id);

				}
				else if (strcmp("session", action) == 0)
				{
					int nb_loops = 100;
					if (strlen(parameters) > 0)
						nb_loops = atoi(parameters);
					bool end_of_game = false;
					char winner = ' ', reason = '?';
					PLAYER_PARAMETERS hpp, vpp;
					gettimeofday(&t0_session, NULL);
					for (int iloop = 1 ; iloop <= nb_loops ; iloop++)
					{
	while (!LoadPlayerParameters(pgConn, hp_min + rand() % (hp_max - hp_min), &hpp));
	while (!LoadPlayerParameters(pgConn, vp_min + rand() % (vp_max - vp_min), &vpp) || vpp.pid == hpp.pid);
						// reset
					current_state = &state_h;
					init_state(&state_h, board.horizontal.paths, board.vertical.paths, false);
					init_state(&state_v, board.horizontal.paths, board.vertical.paths, false);
					move_number = 0;
					orientation = 'H';
					current_game_moves[0] = 0;
					lambda_field(&board, &board.horizontal, &state_h.horizontal, false);
					lambda_field(&board, &board.vertical, &state_h.vertical, false);
					lambda_field(&board, &board.horizontal, &state_v.horizontal, false);
					lambda_field(&board, &board.vertical, &state_v.vertical, false);
						//
						winner = ' '; reason = '?';
						end_of_game = false;
						gettimeofday(&t0_game, NULL);
printf("=======> New Game %d/%d   HPID = %d  VPID = %d\n", iloop, nb_loops, hpp.pid, vpp.pid);
						while (!end_of_game)
						{
							if (orientation == 'H')
							{
								eval_orientation(&board, current_state, orientation, hpp.lambda_decay, wpegs, wlinks, wzeta, true);
								if (empty_field(&state_h.horizontal))
								{
									printf("H resign\n");
									winner = 'V';
									reason = 'R';
									end_of_game = true;
								}
								else if (empty_field(&state_h.horizontal))
								{
									printf("H win\n");
									winner = 'H';
									reason = 'W';
									end_of_game = true;
								}
							}
							else
							{
								eval_orientation(&board, current_state, orientation, vpp.lambda_decay, wpegs, wlinks, wzeta, true);
								if (empty_field(&state_v.vertical))
								{
									printf("V resign\n");
									winner = 'H';
									reason = 'R';
									end_of_game = true;
								}
								else if (empty_field(&state_v.vertical))
								{
									printf("V win\n");
									winner = 'V';
									reason = 'W';
									end_of_game = true;
								}
								//printf("winner = %c  reason = %c\n", winner, reason);
							}
							if (!end_of_game)
							{
								int idx_move = 0, msid = 63;
								if (orientation == 'H')
								{
									if (move_number == 0)
									{
										msid = find_xy(&board, 2+rand()%(width-4), 2+rand()%(height-4));
									}
									else
									{
										int nb_moves = state_moves(&board, current_state, orientation, hpp.opp_decay, &zemoves[0]);
										idx_move = rand() % hpp.max_moves;
										msid = zemoves[idx_move].idx;
									}
									move(&board, &state_h, &state_v, msid, orientation);
									orientation = 'V';
									current_state = &state_v;
								}
								else
								{
									int nb_moves = state_moves(&board, current_state, orientation, vpp.opp_decay, &zemoves[0]);
								//printf("winner = %c  reason = %c  vpp.max_moves = %d\n", winner, reason, vpp.max_moves);
									idx_move = rand() % vpp.max_moves;
								//printf("nb_moves = %d  idx_move = %d\n", nb_moves, idx_move);
									msid = zemoves[idx_move].idx;
									move(&board, &state_v, &state_h, msid, orientation);
									orientation = 'H';
									current_state = &state_h;
								}
								strcat(current_game_moves, board.slot[msid].code);
								move_number++;
								printf("play %d: [%d] %d/%s\n", move_number, idx_move, msid, board.slot[msid].code);
								eval_orientation(&board, current_state, orientation, lambda_decay, wpegs, wlinks, wzeta, false);
								printf("---------- next move %c ----------\n", orientation);
							}
						}
						//EOG
						gettimeofday(&tend_game, NULL);
printf("==== %d/%d ==== winner %c  reason=%c  in %d moves : %s\n", iloop, nb_loops, winner, reason, move_number, current_game_moves);
printf("duration = %6d sec\n", (int)duration(&t0_game, &tend_game)/1000);
printf("average  = %6d ms/move\n", (int)duration(&t0_game, &tend_game)/move_number);

if (winner == 'H')
{
	double expr = EloExpectedResult(hpp.rating, vpp.rating);
	double gl = 10.0 * (1.0 - expr);
printf("H  hpr = %.2f  vpr = %.2f   gl = %.2f   expr = %.4f\n", hpp.rating, vpp.rating, gl, expr);
	UpdatePlayerWin(pgConn, hpp.pid, gl);
	UpdatePlayerLoss(pgConn, vpp.pid, gl);
}
else
{
	double expr = EloExpectedResult(vpp.rating, hpp.rating);
	double gl = 10.0 * (1.0 - expr);
printf("V  hpr = %.2f  vpr = %.2f   gl = %.2f   expr = %.4f\n", hpp.rating, vpp.rating, gl, expr);
	UpdatePlayerWin(pgConn, vpp.pid, gl);
	UpdatePlayerLoss(pgConn, hpp.pid, gl);
}
int mdur = duration(&t0_game, &tend_game)/move_number;
if (strlen(current_game_moves) > 128)
	current_game_moves[128] = 0;
int game_id = insertGame(pgConn, hpp.pid, vpp.pid, current_game_moves, winner, reason, duration(&t0_game, &tend_game), mdur, mdur);
printf("saved as game_id = %d\n\n", game_id);
					}
					gettimeofday(&tend_session, NULL);
printf("===== End of Session: %d games in %.2f sec, avg = %.2f sec/game\n",
		nb_loops, duration(&t0_session, &tend_session)/1000, 1.0*duration(&t0_session, &tend_session)/(1000.0*nb_loops));
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
							if (strcmp(parameters, "lambda-decay") == 0) lambda_decay = dvalue;
							else if (strcmp(parameters, "opponnent-decay") == 0) opponent_decay = dvalue;
							else if (strcmp(parameters, "max-moves") == 0) max_moves = ivalue;
							else if (strcmp(parameters, "wzeta") == 0) wzeta = dvalue;
							else if (strcmp(parameters, "hp-min") == 0) hp_min = ivalue;
							else if (strcmp(parameters, "hp-max") == 0) hp_max = ivalue;
							else if (strcmp(parameters, "vp-min") == 0) vp_min = ivalue;
							else if (strcmp(parameters, "vp-max") == 0) vp_max = ivalue;
							else bp = false;
							if (bp) printf("parameter %s set to %6.3f\n", parameters, dvalue);
						}
					}
				}
				else if (strcmp("parameters", action) == 0)
				{
					printf("lambda-decay   = %6.3f\n", lambda_decay);
					printf("opponent-decay = %6.3f\n", opponent_decay);
					printf("max-moves      =  %d\n", max_moves);
					printf("wpegs          = %6.3f\n", wpegs);
					printf("wlinks         = %6.3f\n", wlinks);
					printf("wzeta          = %6.3f\n", wzeta);
					printf("hp-min         = %3d\n", hp_min);
					printf("hp-max         = %3d\n", hp_max);
					printf("vp-min         = %3d\n", vp_min);
					printf("vp-max         = %3d\n", vp_max);
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
		current_state->horizontal.lambda[lambda].pegs, current_state->horizontal.lambda[lambda].links, current_state->horizontal.lambda[lambda].zeta,
		current_state->vertical.lambda[lambda].score, current_state->vertical.lambda[lambda].waves,
		current_state->vertical.lambda[lambda].pegs, current_state->vertical.lambda[lambda].links, current_state->vertical.lambda[lambda].zeta,
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
					buff1[0] = buff2[0] = 0;
#ifdef __ZETA__
					strncpy(buff1, pw->slot, PATH_MAX_LENGTH);
					strncpy(buff2, pw->step, PATH_MAX_LENGTH);
#endif
					buff1[ph->slots] = 0;
					buff2[ph->steps] = 0;
					printf("s='%C'  p=%d  l=%d  z=%d  slots=[%s]  steps=[%s]\n",
							pw->status, pw->pegs, pw->links, pw->zeta, buff1, buff2);
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
					init_state(&state_h, board.horizontal.paths, board.vertical.paths, false);
					init_state(&state_v, board.horizontal.paths, board.vertical.paths, false);
					move_number = 0;
					orientation = 'H';
					current_game_moves[0] = 0;
					lambda_field(&board, &board.horizontal, &state_h.horizontal, false);
					lambda_field(&board, &board.vertical, &state_h.vertical, false);
					lambda_field(&board, &board.horizontal, &state_v.horizontal, false);
					lambda_field(&board, &board.vertical, &state_v.vertical, false);
				}
				else if (strcmp("board", action) == 0)
				{
					printf("size         = %dx%d\n", width, height);
					printf("max_lambda   = %2d\n", slambda);
					printf("direction    = %2d\n", sdirection);
					printf("hpaths       = %8d  %6ld MB\n", board.horizontal.paths, board.horizontal.paths * sizeof(PATH) / 1000000);
					printf("vpaths       = %8d  %6ld MB\n", board.vertical.paths, board.vertical.paths * sizeof(PATH) / 1000000);
					unsigned int slots_max = 0, steps_max = 0;
					long sz_slots = sizeof(unsigned int) * SumSlotPaths(&board, &slots_max) / 1000000;
					long sz_steps = sizeof(unsigned int) * SumStepPaths(&board, &steps_max) / 1000000;
					printf("slots        = %-4d     %6ld MB  %8d\n", board.slots, sz_slots, slots_max);
					printf("steps        = %-4d     %6ld MB  %8d\n", board.steps, sz_steps, steps_max);
		printf("total        =          %6ld MB\n", (board.horizontal.paths + board.vertical.paths) * sizeof(PATH) / 1000000 + sz_slots + sz_steps);
					printf("field        =          %6ld MB\n", (board.horizontal.paths + board.vertical.paths) * sizeof(WAVE) / 1000000);

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
					printf("\tboard\n");
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
		if (pgConn != NULL) pgCloseConn(pgConn);
	}
	else
	{
		printf("error.init\n");
	}
}
