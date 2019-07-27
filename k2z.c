
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>

#include "board.h"
#include "state.h"
#include "database.h"
#include "book.h"

static bool gtrace_alphabeta = false;
static int mcts_min_visits = 100;
static double mcts_min_ratio = 0.4;

double duration(struct timeval *t1, struct timeval *t2)
{
double elapsedTime = 0.0;

	// compute and print the elapsed time in millisec
	elapsedTime = (t2->tv_sec - t1->tv_sec) * 1000.0;      // sec to ms
	elapsedTime += (t2->tv_usec - t1->tv_usec) / 1000.0;   // us to ms

	return elapsedTime;
}

char *format_duration(char *buffer, int duration)
{
        if (duration < 60)
            sprintf(buffer, "%d s", duration);
        else if (duration < 3600)
            sprintf(buffer, "%d m %02d s", duration/60, duration % 60);
        else
            sprintf(buffer, "%d h %02d m", duration / 3600, (duration / 60) % 60);
        
        return buffer;
}

bool move(BOARD *board, STATE *state_from, STATE *state_to, unsigned short slot, char orientation)
{
//struct timeval t0, t_end;

	if (find_move(state_from, slot))
	{
		char signature[128];
		printf("ERROR-ILLEGAL-MOVE %c %3d/%2s : %s\n", orientation, slot, board->slot[slot].code,
			state_signature(board, state_from, signature));
		return false;
	}
	else
	{
		//gettimeofday(&t0, NULL);
		//free_state(state_to);
		clone_state(state_from, state_to, false);
		//printf("init.clone  %6.2f ms\n", duration(&t_init_s0, &t_clone));

		MOVE move;
		move.slot = slot;
		move.orientation = orientation;
		move.steps = 0;
		int complexity = state_move(board, state_to, &move);
		//gettimeofday(&t_end, NULL);
	}
	return true;
	/*
		printf("move %c %s  %6.2f ms  complexity = %9d   (%d+%d %6.2f%%, %d+%d %6.2f%%)\n", orientation, board->slot[slot].code,
		duration(&t0, &t_end), complexity,
		state_to->horizontal.pegs, state_to->horizontal.links, 100.0 * state_to->horizontal.count / state_to->horizontal.waves,
		state_to->vertical.pegs, state_to->vertical.links, 100.0 * state_to->vertical.count / state_to->vertical.waves);
	*/
}


// ==================
// think()
// ==================
int think_deprecated(BOARD *board, STATE *current_state, char orientation, int depth, int max_moves,
			double lambda_decay, double opponent_decay, double wpegs, double wlinks, double wzeta)
{
int	sid = -1;
TRACK zemoves[1024];

	eval_orientation(board, current_state, orientation, lambda_decay, wpegs, wlinks, wzeta, true);
	int nb_moves = state_moves(board, current_state, orientation, opponent_decay, &zemoves[0]);

	if (depth == 0)
	{
		sid = zemoves[rand() % max_moves].idx;
	}
	else if (depth == 1)
	{
		STATE ns;
		double best_eval = -1.0;
		int best_move = -1;
		for (int im = 0 ; im < max_moves ; im++)
		{
			init_state(&ns, board->horizontal.paths, board->vertical.paths, true);

			move(board, current_state, &ns, zemoves[im].idx, orientation);
			double eval1 = eval_orientation(board, &ns, orientation, lambda_decay, wpegs, wlinks, wzeta, false);
printf("T1  %2d/%2d : %4d/%s   %5.2f %%\n", im, max_moves, zemoves[im].idx, board->slot[zemoves[im].idx].code, eval1);
			if (eval1 > best_eval)
			{
				best_eval = eval1;
				best_move = im;
				sid = zemoves[best_move].idx;
			}
			free_state(&ns);
		}
printf("T1  BEST    %4d/%s : %5.2f %%\n", zemoves[best_move].idx, board->slot[zemoves[best_move].idx].code, best_eval);
	}
	else if (depth == 2)
	{
		TRACK zemoves2[1024];
		STATE ns1;
		double best_eval1 = -1.0;
		int best_move = -1, best_move2 = -1, nb_break2 = 0;
		for (int im = 0 ; im < max_moves ; im++)
		{
			STATE ns2;
			init_state(&ns1, board->horizontal.paths, board->vertical.paths, true);
			move(board, current_state, &ns1, zemoves[im].idx, orientation);
			double eval1 = eval_orientation(board, &ns1, orientation, lambda_decay, wpegs, wlinks, wzeta, true);
printf("T2  %2d/%2d : %4d/%s   %5.2f %%\n", im, max_moves, zemoves[im].idx, board->slot[zemoves[im].idx].code, eval1);

			char opp_orientation = 'H';
			if (orientation == 'H')
				opp_orientation = 'V';
			int nb_moves2 = state_moves(board, &ns1, opp_orientation, opponent_decay, &zemoves2[0]);
			double worst_eval2 = 100.0;
			best_move2 = -1;
			for (int im2 = 0 ; im2 < max_moves ; im2++)
			{
				init_state(&ns2, board->horizontal.paths, board->vertical.paths, true);
				move(board, &ns1, &ns2, zemoves2[im2].idx, opp_orientation);
				double eval2 = eval_orientation(board, &ns2, orientation, lambda_decay, wpegs, wlinks, wzeta, false);
printf("T22 %2d/%2d : %4d/%s   %5.2f %%\n", im2, max_moves, zemoves2[im2].idx, board->slot[zemoves2[im2].idx].code, eval2);
				if (eval2 < worst_eval2)
				{
					worst_eval2 = eval2;
					best_move2 = im2;
				}
				free_state(&ns2);
				if (worst_eval2 < best_eval1)
				{
printf("T22 BREAK   %5.2f %%  <  %5.2f %%  im2 = %d / %d\n", worst_eval2, best_eval1, im2, max_moves);
					nb_break2 += (max_moves - im2 -1);
					break;
				}
			}
printf("T22 WORST   %4d/%s : %5.2f %%\n", zemoves2[best_move2].idx, board->slot[zemoves2[best_move2].idx].code, worst_eval2);
			eval1 = worst_eval2;
			if (eval1 > best_eval1)
			{
				best_eval1 = eval1;
				best_move = im;
				sid = zemoves[best_move].idx;
			}
			free_state(&ns1);
		}
printf("T2  BEST    %4d/%s : %5.2f %%    pruning = %5.2f %%\n",
	zemoves[best_move].idx, board->slot[zemoves[best_move].idx].code, best_eval1, 100.0 * nb_break2 / (max_moves * max_moves));
	}
	return sid;
}
// ==================
// alpha_beta()
// ==================

static int gst_calls = 0;

double alpha_beta(BOARD *board, STATE *state, char move_orientation, char player_orientation, int depth, int max_moves, double alpha, double beta, bool max_player,
			double lambda_decay, double opponent_decay, double wpegs, double wlinks, double wzeta, int* sid, int zmove)
{
double dv = 50.0, d0 = 0.0;
int dsid = 0;
TRACK zemoves[256];
char minmax[8], szmov[32];

	*sid = -1;
	if (zmove >= 0)
		sprintf(szmov, "%4d/%s", zmove, board->slot[zmove].code);
	else
		strcpy(szmov, "       ");

	if (depth == 0)
	{
		dv = eval_orientation(board, state, player_orientation, lambda_decay, wpegs, wlinks, wzeta, false);
		*sid = 0;
		gst_calls++;
		if (gtrace_alphabeta) printf("ab[%c:%d%c/%2d]  %s = %6.2f %%\n", player_orientation, depth, player_orientation, max_moves, szmov, dv);
	}
	else if (depth >= 1)
	{
		STATE new_state;
		double evab = 0.0, dv0 = eval_orientation(board, state, player_orientation, lambda_decay, wpegs, wlinks, wzeta, true);
		int mm = 0, nb_moves = state_moves(board, state, move_orientation, opponent_decay, &zemoves[0]);
		if (nb_moves < max_moves)
		{
			printf("WARNING-TOO-FEW-MOVES !!!!!!!!!!\n");
			mm = nb_moves;
		} else mm = max_moves;
		char next_orientation = 'H';
		if (move_orientation == 'H') next_orientation = 'V';

		// -------------------
		// if-game-won-or-lost
		// -------------------
		if (((player_orientation == 'H' || player_orientation == 'h') && winning_field(&state->horizontal)) ||
			((player_orientation == 'V' || player_orientation == 'v') && winning_field(&state->vertical)))
				return 100.0 + depth;
		else if (((player_orientation == 'H' || player_orientation == 'h') && winning_field(&state->vertical)) ||
			((player_orientation == 'V' || player_orientation == 'v') && winning_field(&state->horizontal)))
				return 0.0 - depth;
		// -------------------

		if (max_player)
		{
			strcpy(minmax, "MAX");
			dv = -999.0;
			for (int im = 0 ; im < mm ; im++)
			{
				init_state(&new_state, board->horizontal.paths, board->vertical.paths, true);
				move(board, state, &new_state, zemoves[im].idx, move_orientation);
				if (depth > 1)
					d0 = eval_orientation(board, &new_state, player_orientation, lambda_decay, wpegs, wlinks, wzeta, false);
				else
					d0 = 0.0;

				if (d0 < 100.0)
					evab = alpha_beta(board, &new_state, next_orientation, player_orientation,
						depth-1, max_moves, alpha, beta, false,
						lambda_decay, opponent_decay, wpegs, wlinks, wzeta, &dsid, zemoves[im].idx);
				else evab = 100.0 + depth;

				if (evab > dv)
				{
					dv = evab;
					*sid = zemoves[im].idx;
				}
				if (dv > alpha) alpha = dv;
				free_state(&new_state);
				if (alpha >= beta) break; // beta cut-off
			}
			//if (dv >= 100.0) dv = 100.0 + depth;
		}
		else
		{
			strcpy(minmax, "min");
			dv = 999.0;
			for (int im = 0 ; im < mm ; im++)
			{
				init_state(&new_state, board->horizontal.paths, board->vertical.paths, true);
				move(board, state, &new_state, zemoves[im].idx, move_orientation);
				if (depth > 1)
					d0 = eval_orientation(board, &new_state, player_orientation, lambda_decay, wpegs, wlinks, wzeta, false);
				else
					d0 = 100.0;

				if (d0 > 0.0)
					evab = alpha_beta(board, &new_state, next_orientation, player_orientation,
						depth-1, max_moves, alpha, beta, true,
						lambda_decay, opponent_decay, wpegs, wlinks, wzeta, &dsid, zemoves[im].idx);
				else evab = 0.0 - depth;

				if (evab < dv)
				{
					dv = evab;
					*sid = zemoves[im].idx;
				}
				if (dv < beta) beta = dv;
				free_state(&new_state);
				if (alpha >= beta) break; // alpha cut-off
			}
			//if (dv <= 0.0) dv = 0.0 - depth;
		}
		if (gtrace_alphabeta)
			printf("ab[%c:%d%c/%2d]  %s -> %4d/%s = %6.2f %%   { a = %7.2f  b = %7.2f }  %s\n",
				player_orientation, depth, move_orientation, mm, szmov, *sid, board->slot[*sid].code, dv, alpha, beta, minmax);
	}
	return dv;
}

int think_alpha_beta(BOARD *board, STATE *state, char orientation, int depth, int max_moves,
			double lambda_decay, double opponent_decay, double wpegs, double wlinks, double wzeta, double *eval)
{
int	sid = -1;
TRACK zemoves[1024];

	*eval = 0.0;
	if (depth == 0)
	{
		eval_orientation(board, state, orientation, lambda_decay, wpegs, wlinks, wzeta, true);
		int nb_moves = state_moves(board, state, orientation, opponent_decay, &zemoves[0]);
		while (sid < 0)
		{
		    sid = zemoves[rand() % max_moves].idx;
		    if (find_move(state, sid)) sid = -1;
		}
	}
	else if (depth >= 1)
	{
		struct timeval t0, t_end;

		gettimeofday(&t0, NULL);
		gst_calls = 0;
		*eval = alpha_beta(board, state, orientation, orientation, depth, max_moves, -999.99, 999.99, true,
			lambda_decay, opponent_decay, wpegs, wlinks, wzeta, &sid, -1);

		gettimeofday(&t_end, NULL);
		double dms = duration(&t0, &t_end) / 1000.0;

		long max_pos = pow(max_moves, depth);
		printf("alpha_beta(%d,%2d) = %6.2f %%   sid = %4d/%s  %6.2f sec  pr = %4.2f %% %7d/%-10ld %4d p/s\n",
			depth, max_moves, *eval, sid, board->slot[sid].code, dms, (double)100.0*(max_pos-gst_calls)/(double)max_pos, gst_calls,
			max_pos, (int)(gst_calls/dms));
	}
	return sid;
}

// parse_slot()
int parse_slot(BOARD *board, char *pslot)
{
	if (strlen(pslot) == 2 && isupper((int)pslot[0]) && isupper((int)pslot[1]))
		return find_slot(board, pslot);
	else return atoi(pslot);
}

int triangle_opposition(int width, int height, int x, int y, char orientation, double alpha_ratio, int *px, int *py)
{
int distance = 0, direction = 0, slots = 0;

	if (orientation == 'H')
	{
		int sx = 0, x0 = 0, xn = 0;
		double alpha_up = 0.0, alpha_down = 0.0;
		//double ddr = (double)(height-1.0) * alpha_ratio;
		double ddr = (height - 1.0) * (1.0 - alpha_ratio) / 2.0;
		double y13 = ddr;
		double y23 = height - 1.0 - ddr;
		if (x < width/2)
		{
			distance = width - x -2;
			direction = 1;
			sx = 10;
			x0 = x + 2;
			xn = width - 2 -distance/3;
			alpha_up = (double)(y - y13) / (double)distance;
			alpha_down = (double)(y - y23) / (double)distance;
		}
		else
		{
			distance = x-1;
			direction = -1;
			sx = 1;
			x0 = 1 + distance/3;
			xn = x - 2;
			alpha_up = (double)(y - y13) / (double)distance;
			alpha_down = (double)(y - y23) / (double)distance;
		}
		//printf("distance = %d  up = %5.2f  down = %5.2f  y13 = %4.2f  y23 = %4.2f\n", distance, alpha_up, alpha_down, y13, y23);
		for (int xx = x0 ; xx <= xn ; xx++)
		{
			for (int yy = 3 ; yy <= 9 ; yy++)
			{
				double a = direction * (double)(y - yy) / (double)(xx - x);
				if (a > alpha_down && a < alpha_up && xx != x && yy != y)
				{
					*px = xx;
					*py = yy;
					px++;
					py++;
					slots++;
					//printf("T %d/%d  a = %5.2f\n", xx, yy, a);
				}
			}
		}
	}
	return slots;
}

int triangle_opposition_board(BOARD *board, int x, int y, char orientation, double alpha_ratio, int *slots)
{
int px[256], py[256];

	int nb_slots = triangle_opposition(board->width, board->height, x, y, orientation, alpha_ratio, &px[0], &py[0]);
	for (int s = 0 ; s < nb_slots ; s++)
	{
		*slots = find_xy(board, px[s], py[s]);
		slots++;
	}
	return nb_slots;
}

//
// live
//
void GameLive(PGconn *pgConn, BOARD *board, int channel, char orientation, int pid, int timeout, int offset, int opid)
{
bool end_of_game = false;
char last_move[8], zmove[8], moves[128], winner = ' ', reason = ' ', opp_orientation = 'H', buffer[256];
int  tick = 0, slot = 0, move_number = 0, depth = 2, max_moves = 20, msid = 0, total_think_duration = 0, total_wait_duration = 0, idb_move = -1;
STATE my_state, new_state;
double wpegs = 0.0, wspread = 0.0, wzeta = 0.0, lambda_decay = 0.8, opponent_decay = 0.8, alpha_beta_eval = 0.0;
PLAYER_PARAMETERS pp;
struct timeval t_begin, t_end;
BOOK_MOVE bm[100];

	gettimeofday(&t_begin, NULL);
	init_state(&my_state, board->horizontal.paths, board->vertical.paths, true);
	init_state(&new_state, board->horizontal.paths, board->vertical.paths, true);

	lambda_field(board, &board->horizontal, &my_state.horizontal, false);
	lambda_field(board, &board->vertical, &my_state.vertical, false);
	lambda_field(board, &board->horizontal, &new_state.horizontal, false);
	lambda_field(board, &board->vertical, &new_state.vertical, false);

	if (LoadPlayerParameters(pgConn, pid, &pp))
	{
		depth = pp.depth;
		max_moves = pp.max_moves;
		lambda_decay = pp.lambda_decay;
		opponent_decay = pp.opp_decay;
		wspread = pp.spread;
		wzeta = pp.zeta;
	}

	if (orientation == 'H' || orientation == 'h')
		opp_orientation = 'V';

	moves[0] = 0;
	while(!end_of_game)
	{
		if (CheckLive(pgConn, channel, orientation, last_move, moves))
		{
			if (strlen(moves) > 0)
			{
				slot = parse_slot(board, last_move);
				move(board, &my_state, &new_state, slot, opp_orientation);
				move_number++;
				UpdateLiveSignature(pgConn, channel, state_signature(board, &new_state, buffer));
				double dwq = eval_orientation(board, &new_state, orientation, lambda_decay, wpegs, wspread, wzeta, false);
				printf("last move = %s  e= %6.2f %%   tick= %7d     moves =  %s  (%lu)\n", last_move, dwq, tick, moves, strlen(moves)/2);
				//-----------
				if (((orientation == 'H' || orientation == 'h') && empty_field(&new_state.horizontal)) ||
					((orientation == 'V' || orientation == 'v') && empty_field(&new_state.vertical)))
				{
					printf("%c resign  --------   eval   = %5.2f %%\n", orientation, dwq);
					winner = opp_orientation;
					reason = 'R';
					end_of_game = true;
					ResignLive(pgConn, channel, winner, reason);
				} else clone_state(&new_state, &my_state, false);
			}
			//---------------
			if (!end_of_game)
			{
				idb_move = -1;
				alpha_beta_eval = 0.0;
				if ((orientation == 'H' || orientation == 'h') && move_number == 0)
					msid = find_xy(board, offset+rand()%(board->width-2*offset), offset+rand()%(board->height-2*offset));
				else
				{
					if (move_number >= 1 && move_number <= 10)
					{
						if (max_moves % 5 == 1 || max_moves % 5 == 2)
						{
							int nb_book_moves = ListBookMoves(pgConn, board->width, moves, &bm[0], max_moves % 5 == 2);
							if (nb_book_moves > 0)
							{
								int book_slot = parse_slot(board, bm[0].move);
								if (find_move(&my_state, book_slot))
								{
printf("ERROR-GAME-LIVE : book_move %3d/%2s  found in #%s\n", book_slot, bm[0].move, moves);
									exit(-1);
								}
								else
								{
									if (bm[0].ratio > 0.0)
										idb_move = book_slot;
printf("book[%d]= %3d/%2s  = %6.2f %%   %d - %d   %s\n", move_number, book_slot, board->slot[book_slot].code, bm[0].ratio, bm[0].win, bm[0].loss, bm[0].move);
								}
							}
						}
						else if (max_moves % 5 == 3)
						{
							MCTS child_nodes[100];
							char cbr = 'x';
		                			if (mcts_child_nodes(pgConn, board->width, moves, &child_nodes[0]) > 0)
							{
		                				if (child_nodes[0].visits >= mcts_min_visits && child_nodes[0].ratio >= mcts_min_ratio)
								{
									idb_move = find_slot(board, child_nodes[0].move); // move is rotated
									//idb_move = child_nodes[0].sid;
									cbr = '*';
								}
								printf("mcts[%d]= %3d/%2s  %5.2f %%   %6d visits   [%c]\n", move_number, idb_move,
									child_nodes[0].move, 100.0*child_nodes[0].ratio, child_nodes[0].visits, cbr);
							}
							else
							{
								MCTS znode;
								char fmoves[128];
								bool hf, vf;
								int dd2 = strlen(moves);

								strcpy(fmoves, moves);
								FlipMoves(board->width, fmoves, dd2/2, &hf, &vf);
								int inode = search_mcts_node(pgConn, fmoves, &znode);
								if (inode < 0)
								{
									char pmoves[128];
									strcpy(pmoves, fmoves);
									pmoves[dd2 - 2] = 0;

									inode = search_mcts_node(pgConn, pmoves, &znode);

									if (inode >= 0)
									{
										strcpy(pmoves, &fmoves[dd2 - 2]);
										pmoves[2] = 0;
										int nsid = find_slot(board, pmoves);
										if (nsid >= 0)
										{
					int new_node = insert_mcts(pgConn, dd2/2, inode, nsid, pmoves, fmoves, 0, 0.0);
printf("++++ MCTS-NODE-CREATED %s  inode = %d,  parent = %d  depth = %d  sid = %d\n", fmoves, new_node, inode, dd2/2, nsid);
										}
									} else printf("no mcts parent for %s\n", pmoves);
								} else printf("no mcts children for %s, inode = %d\n", fmoves, inode);
							}
						}
					}
					if (idb_move >= 0)
						msid = idb_move;
					else
					{
						printf("%c.think()\n", orientation);
						struct timeval t0, t_end;

						gettimeofday(&t0, NULL);
						msid = think_alpha_beta(board, &my_state, orientation, depth, max_moves,
							lambda_decay, opponent_decay, wpegs, wspread, wzeta, &alpha_beta_eval);

						gettimeofday(&t_end, NULL);
						total_think_duration += (int)(duration(&t0, &t_end));
					}
				}

				move(board, &my_state, &new_state, msid, orientation);

				strcpy(zmove, board->slot[msid].code);
				move_number++;
				//-----------
				if (PlayLive(pgConn, channel, orientation, zmove, moves, state_signature(board, &new_state, buffer)))
				{
					tick = 0;
					double dwq2 = eval_orientation(board, &new_state, orientation, lambda_decay, wpegs, wspread, wzeta, false);
					printf("%c played  : %s(%2d) %6.2f %%   abd = %7.2f %%\n", orientation, zmove, move_number, dwq2, alpha_beta_eval - dwq2);
					//-----------
					if (((orientation == 'H' || orientation == 'h') && winning_field(&new_state.horizontal)) ||
						((orientation == 'V' || orientation == 'v') && winning_field(&new_state.vertical)))
					{
						printf("%c win +++ eval   = %6.2f %%  move =      %s     moves =  %s\n", orientation, dwq2, zmove, moves);
						winner = orientation;
						reason = 'W';
						end_of_game = true;
						WinLive(pgConn, channel, winner, reason);
					} else clone_state(&new_state, &my_state, false);
				}
				else
				{
					printf("%c.PLAY-ERROR channel = %d\n", orientation, channel);
					reason = 'E';
					end_of_game = true;
				}
			}
		}
		else if (CheckResign(pgConn, channel, orientation, moves))
		{
			printf("%c win +++ %c resigned, moves = %s\n", orientation, opp_orientation, moves);
			winner = orientation;
			reason = 'R';
			end_of_game = true;
		}
		else
		{
			tick++;
			total_wait_duration += 1000;
			sleep(1);
			if (tick == 1) printf("%c.wait()\n", orientation);
			if (tick >= timeout)
			{
				printf("%c timeout %d\n", orientation, timeout);
				winner = orientation;
				reason = 'T';
				end_of_game = true;
			}
		}
		if (strlen(moves) >= 120)
		{
			printf("%c draw 60 moves\n", orientation);
			winner = '?';
			reason = 'D';
			end_of_game = true;
		}
	}
	free_state(&my_state);
	free_state(&new_state);
	gettimeofday(&t_end, NULL);
	if ((orientation == 'H' || orientation == 'h' || opid == 999) && reason != 'T')
	{
		int ht = (int)(2 * total_think_duration / move_number);
		int vt = (int)(2 * total_wait_duration / move_number);
        printf("********  %c  pid = %d  opid = %d  winner = %c  ********\n", orientation, pid, opid, winner);
int id1, id2;
char tmp[32];
if (chk_dup_move(moves, tmp, &id1, &id2))
{
    printf("DUP-MOVE  %s(%2d,%2d)  in %-48s\n", tmp, id1, id2, moves);
    exit(-1);
}
        if (orientation == 'H' || orientation == 'h')
        {
        int game_id = insertGame(pgConn, pid, opid, moves, winner, reason, total_think_duration + total_wait_duration, ht, vt);
		printf("live #%d:  winner %c   reason = %c    %2d moves  duration =  %-4d sec     game = %d  (%d vs %d)\n",
			channel, winner, reason, move_number, (int)duration(&t_begin, &t_end)/1000, game_id, pid, opid);

		UpdateRatings(pgConn, pid, opid, winner, 10.0);
        }
        else if (orientation == 'V' || orientation == 'v')
        {
		int game_id = insertGame(pgConn, opid, pid, moves, winner, reason, total_think_duration + total_wait_duration, ht, vt);
		printf("live #%d:  winner %c   reason = %c    %2d moves  duration =  %-4d sec     game = %d  (%d vs %d)\n",
			channel, winner, reason, move_number, (int)duration(&t_begin, &t_end)/1000, game_id, opid, pid);

		UpdateRatings(pgConn, opid, pid, winner, 10.0);
        }
	}
	else
		printf("live #%d:  winner %c   reason = %c    %2d moves  duration =  %-4d sec\n",
			channel, winner, reason, move_number, (int)duration(&t_begin, &t_end)/1000);
}

double RD3(double dbase, double range)
{
	int rnd = rand() % 3;
	if (rnd == 0)
		return dbase;
	else if (rnd == 1)
		return dbase + range;
	else
		return dbase - range;
}

double RD5(double dbase, double range)
{
	int rnd = rand() % 5;
	if (rnd == 0)
		return dbase;
	else if (rnd == 1)
		return dbase + range;
	else if (rnd == 2)
		return dbase + 2*range;
	else if (rnd == 3)
		return dbase - range;
	else
		return dbase - 2*range;
}

void chk_mcts_moves(PGconn *pgConn, BOARD *board)
{
char query[256], move[4], code[128];
int id, sid, parent, depth, visits;

	sprintf(query, "select id, depth, sid, move, code, visits, parent from k2s.mcts where id> 0 order by id");
	
	PGresult *pgres = pgQuery(pgConn, query);
	if (pgres != NULL)
	{
		for (int n = 0 ; n < PQntuples(pgres) ; n++)
		{
			id = atoi(PQgetvalue(pgres, n, 0));
			depth = atoi(PQgetvalue(pgres, n, 1));
			sid = atoi(PQgetvalue(pgres, n, 2));
			strcpy(move, PQgetvalue(pgres, n, 3));
			strcpy(code, PQgetvalue(pgres, n, 4));
			visits = atoi(PQgetvalue(pgres, n, 5));
			parent = atoi(PQgetvalue(pgres, n, 6));

			if (strcmp(board->slot[sid].code, move) != 0)
				printf("ERROR-CHK-MCTS-MOVE  node %9d  depth = %2d  parent = %9d  %8d visits  sid = %3d %s <> %s  %s\n",
					id, depth, parent, visits, sid, move, board->slot[sid].code, code);
			if (strncmp(board->slot[sid].code, &code[2*depth-2], 2) != 0)
				printf("ERROR-CHK-MCTS-CODE  node %9d  depth = %2d  parent = %9d  %8d visits  sid = %3d %s <> %s  %s\n",
					id, depth, parent, visits, sid, move, board->slot[sid].code, code);
		}
		PQclear(pgres);
	}
}

int find_tb_move(PGconn *pgConn, BOARD *board, const char *code, int move_number, int max_depth)
{
TB_NODE ztbnode[64];
bool hf, vf;
char tbkey[128];
int idb_move = -1, d = strlen(code) / 2;

	if (d != move_number)
	{
		printf("ERROR-TB %d %s\n", move_number, code);
		exit(-1);
	}
	else if (move_number <= max_depth)
	{
		strcpy(tbkey, code);
//printf("code  = %s  move_number = %d\n", code, move_number);
		FlipMoves(board->width, tbkey, move_number, &hf, &vf);
//printf("tbkey = %s  %c%c\n", tbkey, hf ? '+' : '-', vf ? '+' : '-');
		int nb_tb_child_nodes = tb_code_child_nodes(pgConn, tbkey, move_number, &ztbnode[0]);
		if (nb_tb_child_nodes > 0 && ztbnode[0].eval > 0.0 && ztbnode[0].deep_eval > 0.0)
		{
//printf("best move = %s\n", ztbnode[0].move);
			FlipString(board->width, ztbnode[0].move, hf, vf);
//printf("best move = %s\n", ztbnode[0].move);
			idb_move = find_slot(board, ztbnode[0].move);
			printf("_TB_[%d]= %3d/%2s  eval = %5.2f %%   deep_eval = %5.2f %%\n", move_number, idb_move,
				ztbnode[0].move, ztbnode[0].eval, ztbnode[0].deep_eval);
		} else printf("no tb children for %c%c%s/%s\n", hf ? '+' : '-', vf ? '+' : '-', tbkey, code);
	}
	return idb_move;
}

// ==========
//    main
// ==========
int main(int argc, char* argv[])
{
int width = 16, height = 16, max_moves = 5, slambda = 10, sdirection = -1, offset = 2, live_timeout = 300, live_loop = 100, wait_live = 60, db_tb_max_depth = 4;
int hp_min = 1, hp_max = 16;
int vp_min = 1, vp_max = 16;
double wpegs = 0.0, wlinks = 0.0, wzeta = 0.0, alpha_beta_eval = 0.0, exploration = 0.4;
double lambda_decay = 0.8, opponent_decay = 0.8, alpha_ratio = 0.4, random_decay = 0.1, elo_coef = 10.0;
struct timeval t0, t_init_board, t_init_wave, t_init_s0, t_clone, t_move, t0_game, tend_game, t0_session, tend_session, t_begin, t_end;
BOARD board;
TRACK zemoves[512];
char buffer_error[512], database_name[32], root[128], buffer[128];
PGconn *pgConn = NULL;

	/*
	int px[256], py[256];
	int nb_tslots = triangle_opposition(width, height, 2, 6, 'H', &px[0], &py[0]);
	exit(0);
	*/

	buffer_error[0] = 0;
#ifdef __ZETA__
	strcpy(buffer_error, "ZETA");
#endif
	printf("version=%s %s  %s\n", __DATE__, __TIME__, buffer_error);
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
	//----------
	int nb_params = 0;
	char paramline[128];
	FILE *fp = fopen("./parameters.default", "r");
	if (fp != NULL)
	{
		while (fgets(paramline, 120, fp) != NULL)
		{
			if (strlen(paramline) > 0)
			{
				int plen = strlen(paramline);
				char *pvalue = NULL;
				for (int ic = 0 ; ic < plen ; ic++)
				{
					if (paramline[ic] == '=')
					{
						paramline[ic] = 0;
						pvalue = &paramline[ic+1];
					}
				}
				if (pvalue != NULL && *pvalue != 0)
				{
					double dvalue = atof(pvalue);
					int ivalue = atoi(pvalue);
					bool bp = true;
					if (strcmp(paramline, "lambda-decay") == 0) lambda_decay = dvalue;
					else if (strcmp(paramline, "opponnent-decay") == 0) opponent_decay = dvalue;
					else if (strcmp(paramline, "max-moves") == 0) max_moves = ivalue;
					else if (strcmp(paramline, "wlinks") == 0) wlinks = dvalue;
					else if (strcmp(paramline, "wzeta") == 0) wzeta = dvalue;
					else if (strcmp(paramline, "hp-min") == 0) hp_min = ivalue;
					else if (strcmp(paramline, "hp-max") == 0) hp_max = ivalue;
					else if (strcmp(paramline, "vp-min") == 0) vp_min = ivalue;
					else if (strcmp(paramline, "vp-max") == 0) vp_max = ivalue;
					else if (strcmp(paramline, "offset") == 0) offset = ivalue;
					else if (strcmp(paramline, "live-timeout") == 0) live_timeout = ivalue;
					else if (strcmp(paramline, "live-loop") == 0) live_loop = ivalue;
					else if (strcmp(paramline, "wait-live") == 0) wait_live = ivalue;
					else if (strcmp(paramline, "alpha-ratio") == 0) alpha_ratio = dvalue;
					else if (strcmp(paramline, "random-decay") == 0) random_decay = dvalue;
					else if (strcmp(paramline, "elo-coef") == 0) elo_coef = dvalue;
					else if (strcmp(paramline, "exploration") == 0) exploration = dvalue;
					else if (strcmp(paramline, "tb-depth") == 0) db_tb_max_depth = ivalue;
                    else if (strcmp(paramline, "mcts-min-ratio") == 0) mcts_min_ratio = dvalue;
                    else if (strcmp(paramline, "mcts-min-visits") == 0) mcts_min_visits = ivalue; // 
					nb_params++;
				}
			}
		}
		printf("\nparameters.default file loaded, %d parameters set.\n\n", nb_params);
	}
	//----------
	root[0] = 0;
	unsigned long ul_allocated = init_board(&board, width, height, slambda, sdirection);
	if (ul_allocated > 0)
	{
		gettimeofday(&t_init_board, NULL);
		printf("init.board %8.2f ms        size = %6ld MB  complexity = %10d\n",
			duration(&t0, &t_init_board), ul_allocated, board.horizontal.paths+board.vertical.paths);

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
			char *pcommand = fgets(command, 256, stdin);
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
				else if (strcmp("load", action) == 0)
				{
					int plen = strlen(parameters);
					char *pvalue = NULL;
					for (int ic = 0 ; ic < plen ; ic++)
					{
						if (parameters[ic] == '/')
						{
							parameters[ic] = 0;
							pvalue = &parameters[ic+1];
						}
					}
					if (pvalue != NULL && *pvalue != 0)
					{
						char game_moves[128];
						int nb_moves = atoi(pvalue);
						int game_id = atoi(parameters);

						if (LoadGame(pgConn, game_id, game_moves))
						{
							int total_moves = strlen(game_moves)/2;
							printf("game %d loaded, %d/%d moves\n", game_id, nb_moves, total_moves);
							if (nb_moves > total_moves) nb_moves = total_moves;
							if (nb_moves < total_moves)
								game_moves[2*nb_moves] = 0;

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
							//apply moves
							for (int imove = 0 ; imove < nb_moves ; imove++)
							{
								char smove[4];
								smove[0] = game_moves[2*imove];
								smove[1] = game_moves[2*imove+1];
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
						else
							printf("game %d not found\n", game_id);
					}
				}
				else if (strcmp("move", action) == 0)
				{
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
				else if (strcmp("think", action) == 0)
				{
					int depth = 0, plen = strlen(parameters);
					if (plen > 0)
					{
						char *pvalue = NULL;
						for (int ic = 0 ; ic < plen ; ic++)
						{
							if (parameters[ic] == '/')
							{
								parameters[ic] = 0;
								pvalue = &parameters[ic+1];
							}
						}
						depth = atoi(parameters);
						if (pvalue != NULL && *pvalue != 0)
							max_moves = atoi(pvalue);
					}
					int msid = think_alpha_beta(&board, current_state, orientation, depth, max_moves,
							lambda_decay, opponent_decay, wpegs, wlinks, wzeta, &alpha_beta_eval);
					printf("think-move(%d,%2d) = %3d/%s\n", depth, max_moves, msid, board.slot[msid].code);
				}
				else if (strcmp("play", action) == 0)
				{
					int depth = 0, plen = strlen(parameters);
					if (plen > 0)
					{
						char *pvalue = NULL;
						for (int ic = 0 ; ic < plen ; ic++)
						{
							if (parameters[ic] == '/')
							{
								parameters[ic] = 0;
								pvalue = &parameters[ic+1];
							}
						}
						depth = atoi(parameters);
						if (pvalue != NULL && *pvalue != 0)
							max_moves = atoi(pvalue);
					}
					int msid = think_alpha_beta(&board, current_state, orientation, depth, max_moves,
						lambda_decay, opponent_decay, wpegs, wlinks, wzeta, &alpha_beta_eval);

					if (orientation == 'H')
					{
						move(&board, &state_h, &state_v, msid, orientation);
						orientation = 'V';
						current_state = &state_v;
					}
					else
					{
						move(&board, &state_v, &state_h, msid, orientation);
						orientation = 'H';
						current_state = &state_h;
					}
					strcat(current_game_moves, board.slot[msid].code);
					move_number++;
					printf("move %d: %d/%s played\n", move_number, msid, board.slot[msid].code);
					eval_orientation(&board, current_state, orientation, lambda_decay, wpegs, wlinks, wzeta, false);
				}
				else if (strcmp("game", action) == 0)
				{
					int depth = 0, plen = strlen(parameters);
					if (plen > 0)
					{
						char *pvalue = NULL;
						for (int ic = 0 ; ic < plen ; ic++)
						{
							if (parameters[ic] == '/')
							{
								parameters[ic] = 0;
								pvalue = &parameters[ic+1];
							}
						}
						depth = atoi(parameters);
						if (pvalue != NULL && *pvalue != 0)
							max_moves = atoi(pvalue);
					}

					bool end_of_game = false;
					char winner = ' ', reason = '?';
					gettimeofday(&t0_game, NULL);
					while (!end_of_game && move_number < 60)
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
int msid = think_alpha_beta(&board, current_state, orientation, depth, max_moves, lambda_decay, opponent_decay, wpegs, wlinks, wzeta, &alpha_beta_eval);

							if (orientation == 'H')
							{
								move(&board, &state_h, &state_v, msid, orientation);
								orientation = 'V';
								current_state = &state_v;
							}
							else
							{
								move(&board, &state_v, &state_h, msid, orientation);
								orientation = 'H';
								current_state = &state_h;
							}
							strcat(current_game_moves, board.slot[msid].code);
							move_number++;
							printf("move %d:  %d/%s played\n", move_number, msid, board.slot[msid].code);
							eval_orientation(&board, current_state, orientation, lambda_decay, wpegs, wlinks, wzeta, false);
							printf("---------- next move %c ----------\n", orientation);
						}
					}
					gettimeofday(&tend_game, NULL);
					if (move_number >= 60)
					{
						winner = '?';
						reason = 'D';
					}
					printf("========== winner %c  reason=%c  in %d moves : %s  duration = %5.2f s  average = %5.1f ms/move\n",
						winner, reason, move_number, current_game_moves,
						duration(&t0_game, &tend_game)/1000, duration(&t0_game, &tend_game)/move_number);

					int game_id = insertGame(pgConn, 0, 0, current_game_moves, winner, reason, duration(&t0_game, &tend_game), 0, 0);
					printf("saved as game_id = %d\n", game_id);

				}
				else if (strcmp("save", action) == 0)
				{
					int game_id = insertGame(pgConn, 0, 0, current_game_moves, orientation, '?', 0, 0, 0);
					printf("saved as game_id = %d\n", game_id);
				}
				else if (strcmp("live", action) == 0)
				{
					if (strlen(parameters) >= 2)
					{
						int player_id = 10;
						if (strlen(parameters) >= 4 && parameters[2] == '/')
							player_id = atoi(&parameters[3]);
						char orient = parameters[1];
						parameters[1] = 0;
						int channel = atoi(parameters);
						if (orient == 'H' || orient == 'h') // register
						{
							gettimeofday(&t_begin, NULL);
							for (int iloop = 0 ; iloop < live_loop ; iloop++)
							{
								if (RegisterLive(pgConn, channel, player_id))
								{
					printf("\nregistered live channel id = %d, hp = %3d        iloop =  %d/%d   wait = %d\n",
										channel, player_id, iloop+1, live_loop, wait_live);
									// wait for V player to join
									int nb_wait = 0, vp = 0;
									do
									{
										sleep(1);
										nb_wait++;
										vp = WaitLive(pgConn, channel);
									} while (vp <= 0 && nb_wait < wait_live);
									if (vp > 0)
									{
										printf("vp %d joined live channel %d  (wait = %d)\n", vp, channel, nb_wait);
										GameLive(pgConn, &board, channel, toupper(orient), player_id, live_timeout, offset, vp);
										sleep(10);
									} else printf("wait aborted after %d seconds.\n", wait_live);
									if (orient == 'H') DeleteLive(pgConn, channel);
									sleep(5);
								}
								else printf("cannot-register-live-channel %d\n", channel);
							}
							gettimeofday(&t_end, NULL);
							printf("session live channel id = %d, hp = %3d        duration =  %-3d min     count = %d\n",
								channel, player_id, (int)duration(&t_begin, &t_end)/60000, live_loop);
							srand(t_end.tv_sec);
						}
						else if (orient == 'V' || orient == 'v') // join
						{
							gettimeofday(&t_begin, NULL);
							for (int iloop = 0 ; iloop < live_loop ; iloop++)
							{
								if (JoinLive(pgConn, channel, player_id))
								{
                                    int vpp2 = 0, hpp2 = 0;
                                    LivePlayers(pgConn, channel, &hpp2, &vpp2);
									printf("\njoined live channel id = %d, vp = %3d      hp = %3d      iloop =  %4d / %-4d\n",
                                           channel, player_id, hpp2, iloop+1, live_loop);
									GameLive(pgConn, &board, channel, toupper(orient), player_id, live_timeout, offset, hpp2);
                                    sleep(10);
                                    if (orient == 'V') DeleteLive(pgConn, channel);
								}
								else printf("cannot-join-live-channel %d  iloop =  %4d / %-4d\n", channel, iloop+1, live_loop);
								sleep(15);
							}
							gettimeofday(&t_end, NULL);
							printf("session live channel id = %d, vp = %3d        duration =  %-3d min     count = %d\n",
								channel, player_id, (int)duration(&t_begin, &t_end)/60000, live_loop);
							srand(t_end.tv_sec);
						}
						else if (orient == 'D') // delete
						{
							if (DeleteLive(pgConn, channel))
							{
								printf("deleted live channel id = %d\n", channel);
							}
							else printf("cannot-delete-live-channel %d\n", channel);
						}
						else if (orient == 'P') // print
						{
							PrintLive(pgConn, channel);
						}
					}
				}
				else if (strcmp("session", action) == 0)
				{
					int nb_loops = 100, msh = 0, msv = 0, msid = 63;
					if (strlen(parameters) > 0)
						nb_loops = atoi(parameters);
					bool end_of_game = false;
					char winner = ' ', reason = '?';
					PLAYER_PARAMETERS hpp, vpp;
					gettimeofday(&t0_session, NULL);
					for (int iloop = 1 ; iloop <= nb_loops ; iloop++)
					{
						msh = msv = 0;
						while (!LoadPlayerParameters(pgConn, hp_min + rand() % (hp_max - hp_min + 1), &hpp));
						while (!LoadPlayerParameters(pgConn, vp_min + rand() % (vp_max - vp_min + 1), &vpp) || vpp.pid == hpp.pid);
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
printf("===========================  %4d / %-4d       [   %d / %6.2f    vs    %d / %6.2f   ]\n", iloop, nb_loops, hpp.pid, hpp.rating, vpp.pid, vpp.rating);
						while (!end_of_game)
						{
							gettimeofday(&t_begin, NULL);
							if (move_number >= 60)
							{
								printf("DRAW 60 moves\n");
								winner = ' ';
								reason = 'M';
								end_of_game = true;
							}
							else
							{
								if (orientation == 'H')
								{
		eval_orientation(&board, current_state, orientation, RD3(hpp.lambda_decay, random_decay), wpegs, hpp.spread, hpp.zeta, true);
									if (empty_field(&state_h.horizontal))
									{
										printf("H resign\n");
										winner = 'V';
										reason = 'R';
										end_of_game = true;
									}
									else if (empty_field(&state_h.vertical))
									{
										printf("H win\n");
										winner = 'H';
										reason = 'W';
										end_of_game = true;
									}
								}
								else
								{
		eval_orientation(&board, current_state, orientation, RD3(vpp.lambda_decay, random_decay), wpegs, vpp.spread, vpp.zeta, true);
									if (empty_field(&state_v.vertical))
									{
										printf("V resign\n");
										winner = 'H';
										reason = 'R';
										end_of_game = true;
									}
									else if (empty_field(&state_v.horizontal))
									{
										printf("V win\n");
										winner = 'V';
										reason = 'W';
										end_of_game = true;
									}
								}
							}
							if (!end_of_game)
							{
								int idx_move = 0;
								if (orientation == 'H')
								{
                                    int idb_move = -1;
                                    //==================
    if ((move_number == 2 || move_number == 4 || move_number == 6 || move_number == 8 || move_number == 10) &&
                            (hpp.max_moves % 5 == 1 || hpp.max_moves % 5 == 2 || hpp.max_moves % 5 == 3))
                                    {
					if (hpp.max_moves % 5 == 3)
					{
						idb_move = find_tb_move(pgConn, &board, current_game_moves, move_number, db_tb_max_depth - hpp.depth);
					}
					else
					{
                                        BOOK_MOVE bm[100];
                                        int nb_book_moves = ListBookMoves(pgConn, board.width, current_game_moves, &bm[0], hpp.max_moves % 5 == 2);
                                        if (nb_book_moves > 0)
                                        {
						int book_slot = parse_slot(&board, bm[0].move);
						if (find_move(current_state, book_slot))
						{
printf("ERROR-SESSION : book_move %3d/%2s  found in #%s\n", book_slot, bm[0].move, current_game_moves);
exit(-1);
						}
						else
						{
	                                		if (bm[0].ratio > 0.0)
	                   					idb_move = book_slot;
printf("book[%d]= %3d/%2s  = %6.2f %%   %d - %d   %s\n", move_number, book_slot, board.slot[book_slot].code, bm[0].ratio, bm[0].win, bm[0].loss, bm[0].move);
						}
                                        }
                                    }
                                    }
                                    //===================
					if (move_number == 0)
					{
						if (strlen(root) >= 2)
						{
						strcpy(buffer, root);
						buffer[2] = 0;
						msid = parse_slot(&board, buffer);
						printf("root[%d]= %3d/%s\n", move_number, msid, board.slot[msid].code);
						}
						else
msid = find_xy(&board, offset+rand()%(width-2*offset), offset+rand()%(height-2*offset));
					}
					else
					{
						if (strlen(root) > 2 * move_number)
						{
						strcpy(buffer, &root[2 * move_number]);
						buffer[2] = 0;
						msid = parse_slot(&board, buffer);
						printf("root[%d]= %3d/%s\n", move_number, msid, board.slot[msid].code);
						}
					else if (idb_move >= 0)
                                    {
                                        msid = idb_move;
                                    }
					else
		msid = think_alpha_beta(&board, current_state, orientation, hpp.depth, hpp.max_moves,
				RD3(hpp.lambda_decay, random_decay), RD3(hpp.opp_decay, random_decay),
				wpegs, hpp.spread, hpp.zeta, &alpha_beta_eval);
									}
									move(&board, &state_h, &state_v, msid, orientation);
									orientation = 'V';
									current_state = &state_v;
								}
								else
								{
                                    int idb_move = -1;
                                    //==================
                                    if ((move_number == 1 || move_number == 3 || move_number == 5 || move_number == 7 || move_number == 9) &&
                                        (vpp.max_moves % 5 == 1 || vpp.max_moves % 5 == 2 || vpp.max_moves % 5 == 3))
                                    {
					if (vpp.max_moves % 5 == 3)
					{
						idb_move = find_tb_move(pgConn, &board, current_game_moves, move_number, db_tb_max_depth - vpp.depth);
					}
					else
					{
		                                BOOK_MOVE bm[100];
		                                int nb_book_moves = ListBookMoves(pgConn, board.width, current_game_moves, &bm[0], vpp.max_moves % 5 == 2);
		                                if (nb_book_moves > 0)
		                                {
							int book_slot = parse_slot(&board, bm[0].move);
							if (find_move(current_state, book_slot))
							{
printf("ERROR-SESSION : book_move %3d/%2s  found in #%s\n", book_slot, bm[0].move, current_game_moves);
exit(-1);
							}
							else
							{
		                                    		if (bm[0].ratio > 0.0)
		                                        		idb_move = book_slot;
printf("book[%d]= %3d/%2s  = %6.2f %%   %d - %d   %s\n", move_number, book_slot, board.slot[book_slot].code, bm[0].ratio, bm[0].win, bm[0].loss, bm[0].move);
							}
		                                }
					}
                                    }
                                    //===================
					if (move_number == 1 && alpha_ratio >= 0.2 && alpha_ratio <= 0.8)
					{
						int tslots[256];
						int first_sid = msid;
						int x0 = board.slot[msid].x;
						int y0 = board.slot[msid].y;
						int nb_tslots = triangle_opposition_board(&board, x0, y0, 'H', alpha_ratio, &tslots[0]);
						msid = tslots[rand() % nb_tslots];
						printf("tos/%02d = %3d/%s    id = %3d   x0 = %d  y0 = %d\n", nb_tslots, msid, board.slot[msid].code, first_sid, x0, y0);
					}
					else
					{
						if (strlen(root) > 2 * move_number)
						{
						strcpy(buffer, &root[2 * move_number]);
						buffer[2] = 0;
						msid = parse_slot(&board, buffer);
						printf("root[%d]= %3d/%s\n", move_number, msid, board.slot[msid].code);
						}
					else if (idb_move >= 0)
                                    {
                                        msid = idb_move;
                                    }
										else
			msid = think_alpha_beta(&board, current_state, orientation, vpp.depth, vpp.max_moves,
					RD3(vpp.lambda_decay, random_decay), RD3(vpp.opp_decay, random_decay),
					wpegs, vpp.spread, vpp.zeta, &alpha_beta_eval);
									}
									move(&board, &state_v, &state_h, msid, orientation);
									orientation = 'H';
									current_state = &state_h;
								}
								strcat(current_game_moves, board.slot[msid].code);
								move_number++;
								printf("play %d : %3d/%s\n", move_number, msid, board.slot[msid].code);
								if (orientation == 'H')
									eval_orientation(&board, current_state, orientation, lambda_decay, wpegs, hpp.spread, hpp.zeta, false);
								else
									eval_orientation(&board, current_state, orientation, lambda_decay, wpegs, vpp.spread, vpp.zeta, false);
								printf("---------- move  %c --------\n", orientation);
							}
							gettimeofday(&t_end, NULL);
							if (orientation == 'H')
								msv += duration(&t_begin, &t_end);
							else
								msh += duration(&t_begin, &t_end);
						}
						// ---
						// EOG
						// ---
						gettimeofday(&tend_game, NULL);
						printf("===========  %4d / %-4d  + winner %c   reason %c   %d # %s\n",
							iloop, nb_loops, winner, reason, move_number, current_game_moves);
						printf("duration  = %6d sec\n", (int)duration(&t0_game, &tend_game)/1000);

						UpdateRatings(pgConn, hpp.pid, vpp.pid, winner, elo_coef);

						int mh = msh/((move_number+1)/2);
						int mv = msv/(move_number/2);
						printf("average H = %6d ms/move\n", mh);
						printf("average V = %6d ms/move\n", mv);

						if (strlen(current_game_moves) > 128)
							current_game_moves[128] = 0;
            int id1, id2;
            char tmp[32];
if (chk_dup_move(current_game_moves, tmp, &id1, &id2))
{
    printf("DUP-MOVE  %s(%2d,%2d)  in %-48s\n", tmp, id1, id2, current_game_moves);
    exit(-1);
}
	int game_id = insertGame(pgConn, hpp.pid, vpp.pid, current_game_moves, winner, reason, duration(&t0_game, &tend_game), mh, mv);
						printf("saved as  = %6d\n\n", game_id);
					}
					// ---
					// EOS
					// ---
					gettimeofday(&tend_session, NULL);
					printf("===========  End of Session: %d games in %.2f sec, avg = %.2f sec/game\n",
					nb_loops, duration(&t0_session, &tend_session)/1000, 1.0*duration(&t0_session, &tend_session)/(1000.0*nb_loops));
				}
				else if (strcmp("nodes", action) == 0)
				{
					if (strlen(parameters) == 0)
					{
						int nb_nodes = pgGetCount(pgConn, "k2s.mcts", "");
						int max_depth = pgGetMax(pgConn, "depth", "k2s.mcts", "");
						int nb_visits = pgGetSum(pgConn, "visits", "k2s.mcts", "");
						printf("depth = %d, %d nodes, %d visits\n", max_depth, nb_nodes, nb_visits);
					}
					else
					{
						MCTS znode, child_node[100];
						int inode = 0, nb_child_nodes = 0;
						if (parameters[0] == '.')
						{
							find_mcts_node(pgConn, 0, &znode);
							nb_child_nodes = mcts_children(pgConn, 0, &child_node[0]);
						}
						else
						{
							inode = search_mcts_node(pgConn, parameters, &znode);
							nb_child_nodes = mcts_child_nodes(pgConn, board.width, parameters, &child_node[0]);
						}
						printf(" inode  =    %9d   %8d visits   %5.2f %%\n", znode.id, znode.visits, 100.0 * znode.ratio);
						for (int inode = 0 ; inode < nb_child_nodes ; inode++)
printf("%2d:  %3d/%s    %5.2f %%   %8d\n", inode, child_node[inode].sid, child_node[inode].move, 100.0 * child_node[inode].ratio, child_node[inode].visits);
					}
				}
				else if (strcmp("inode", action) == 0)
				{
					MCTS znode;
					if (find_mcts_node(pgConn, atoi(parameters), &znode))
						printf("depth = %d, parent = %d, %d visits   %5.2f %%   %3d/%s  %s\n",
							znode.depth, znode.parent, znode.visits, 100.0*znode.ratio, znode.sid, znode.move, znode.code);
					else
						printf("node %d not found\n", atoi(parameters));
				}
				else if (strcmp("cnode", action) == 0)
				{
					MCTS znode;
					int inode = search_mcts_node(pgConn, parameters, &znode);
					if (inode >= 0)
						printf("node %s exists, inode = %d\n", parameters, inode);
					else
					{
						char buffer_parent[32], new_move[4];
						strcpy(buffer_parent, parameters);
						int ndepth = strlen(parameters)/2;
						buffer_parent[2 * ndepth - 2] = 0;
						strcpy(new_move, &parameters[2 * ndepth - 2]);
						int iparent = search_mcts_node(pgConn, buffer_parent, &znode);
						if (iparent < 0)
							printf("node %s does not exist\n", buffer_parent);
						else
						{
							int nsid = find_slot(&board, new_move);
							if (nsid >= 0)
							{
							int new_node = insert_mcts(pgConn, ndepth, iparent, nsid, new_move, parameters, 0, 0.0);
					printf("node %s created, inode = %d, parent = %d  depth = %d  sid = %d\n", parameters, new_node, iparent, ndepth, nsid);
							}
							else printf("slot %s is invalid\n", new_move);
						}
					}
				}
				else if (strcmp("tbs", action) == 0) // ZE-TBS
				{
					TB_STATS tbs;
                    			int tb_threshold = atoi(parameters);
					int tb_max_depth = pgGetMax(pgConn, "depth", "k2s.tb", "");
					int tb_up = 0, tb_down = 0;
					char buftbs[64];
					for (int tbsd = 1 ; tbsd <= tb_max_depth ; tbsd++)
					{
						sprintf(buftbs, "depth = %d and eval >= %d.0", tbsd, 100 - tb_threshold);
						tb_up = pgGetCount(pgConn, "k2s.tb", buftbs);
						sprintf(buftbs, "depth = %d and eval < %d.0", tbsd, tb_threshold);
						tb_down = pgGetCount(pgConn, "k2s.tb", buftbs);
						if (tb_stats(pgConn, tbsd, &tbs))
							printf("%2d / %9d  [ %9d - %-9d ]  { %5.2f - %5.2f }  %7d / %5.2f %%  ( %7d + %-7d )\n",
								tbs.depth, tbs.count, tbs.min_id, tbs.max_id, tbs.min_eval, tbs.max_eval,
								tb_up + tb_down, 100.0 * (double)(tb_up + tb_down)/tbs.count, tb_up, tb_down);
					}
				}
				else if (strcmp("tbd", action) == 0) // ZE-TBD
				{
                    			int tb_depth = atoi(parameters);
					tb_init_deep_evals(pgConn, tb_depth);
                    			tb_update_deep_evals(pgConn, tb_depth);
				}
				else if (strcmp("tbq", action) == 0) // ZE-TBQ
				{
                    			TB_NODE ztbnode[64];
					bool tbhf = false, tbvf = false;
					FlipBuffer(board.width, parameters, &tbhf, &tbvf);
					int inode = tb_node(pgConn, parameters, &ztbnode[0]);
					if (inode > 0)
					{
						printf("%8d  %c%c  %6.2f %%  %6.2f %%   %d / %s\n",
							inode, tbhf ? '+' : '-', tbvf ? '+' : '-',
							ztbnode[0].eval, ztbnode[0].deep_eval,
					       		(int)(strlen(parameters) / 2), parameters);
						//int nb_tb_child_nodes = tb_nodes(pgConn, (int)(strlen(parameters) / 2), &ztbnode[0]);
						int nb_tb_child_nodes = tb_child_nodes(pgConn, inode, &ztbnode[0]);
						for (int ichild = 0 ; ichild < nb_tb_child_nodes ; ichild++)
						{
							FlipString(board.width, ztbnode[ichild].move, tbhf, tbvf);
						    printf("%8d  %s  %6.2f %%  %6.2f %%\n", ztbnode[ichild].id,
							   ztbnode[ichild].move,
							   ztbnode[ichild].eval, ztbnode[ichild].deep_eval);
						}
					} else printf("TB node code %s not found\n", parameters);
				}
				else if (strcmp("tbb", action) == 0) // ZE-TBB
				{
					char tb_move[32], btbuff[256];
					double dv0 = 0.0;
					int tb_updated = 0, tb_errors = 0, tb_depth = 0, tb_depth_count = 0, lenp = strlen(parameters);
                    int tb_start = 0, tb_end = 999999999;
					if (lenp <= 2)
					{
						tb_depth = atoi(parameters);

						printf("==== TABLE BASE [ depth = %d ]\n", tb_depth);
                        printf("from node id : ");
                        tb_start = atoi(fgets(btbuff, 16, stdin));
                        printf("to node id   : ");
                        tb_end = atoi(fgets(btbuff, 16, stdin));
                        tb_depth_count = tb_count(pgConn, tb_depth, tb_start, tb_end);
                            
						TB_NODE *tbnode = (TB_NODE *)malloc((tb_depth_count+10) * sizeof(TB_NODE));
						int tb_slot = 0, nb_valuated = 0;
                        int nb_tb_nodes = tb_nodes(pgConn, tb_depth, tb_start, tb_end, &tbnode[0]);
						printf("valuating %d nodes from %d to %d ...\n", nb_tb_nodes, tb_start, tb_end);
						gettimeofday(&t0_game, NULL);

						for (int tbidx = 0 ; tbidx < nb_tb_nodes ; tbidx++)
						{
                if (tbnode[tbidx].id >= tb_start && tbnode[tbidx].id <= tb_end & tbnode[tbidx].eval == 0.0)
							{
								//----- reset states
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
								//------
								for (int d = 0 ; d < tb_depth ; d++)
								{
									strcpy(tb_move, &tbnode[tbidx].code[2 * d]);
									tb_move[2] = 0;
									tb_slot = find_slot(&board, tb_move);
									if (tb_slot < 0)
									{
										printf("move %s not found, depth = %d, node %d/%d\n", tb_move, d, tbidx, nb_tb_nodes);
										exit(-1);
									}
									if (orientation == 'H')
									{
										if (move(&board, &state_h, &state_v, tb_slot, orientation))
										{
											orientation = 'V';
											current_state = &state_v;
										}
										else
										{
											printf("TB-ERROR depth = %d/%d  H sid = %d\n", d, tb_depth, tb_slot);
											exit(-1);
										}
									}
									else
									{
										if (move(&board, &state_v, &state_h, tb_slot, orientation))
										{
											orientation = 'H';
											current_state = &state_h;
										}
										else
										{
											printf("TB-ERROR depth = %d/%d  V sid = %d\n", d, tb_depth, tb_slot);
											exit(-1);
										}
									}
									move_number++;
									//printf("board move #%d : %3d / %s\n", move_number, mcts_root[d], board.slot[mcts_root[d]].code);
								}
								//------
								dv0 = 100.0 - eval_orientation(&board, current_state, orientation, RD3(lambda_decay, random_decay), 0.0, 0.0, 0.0, false);
								if (tb_update_eval(pgConn, tbnode[tbidx].id, dv0))
									tb_updated++;
									//printf("id %d - %s : %.2f\n", tbnode[tbidx].id, tbnode[tbidx].code, dv0);
								else
									tb_errors++;
									//printf("TB-ERROR save %d\n", tbnode[tbidx].id);
								//int nb_moves = state_moves(&board, current_state, orientation, opponent_decay, &zemoves[0]);
								//------
								nb_valuated++;
							}
if (nb_tb_nodes > 5000 && tbidx % 1000 == 0 && tbidx > 0)
    printf("---- TB %7d / %-7d  %6.2f %%  [ %d <= %8d <= %d]\n",
           tbidx, nb_tb_nodes, 100.0*tbidx/nb_tb_nodes,
           tb_start, tbnode[tbidx].id, tb_end);
						}
						free(tbnode);
						gettimeofday(&tend_game, NULL);
                        printf("%d/%d nodes valuated, %d updated, %d errors  ( %s )  ( %5.1f n/s )\n",
                            nb_valuated, nb_tb_nodes, tb_updated, tb_errors,
                            format_duration(btbuff, (int)duration(&t0_game, &tend_game)/1000),
                            (double)tb_updated / (double)nb_tb_nodes);
					}
					else if (lenp >= 7)
					{
						char child_code[64];
						parameters[lenp - 6] = 0;
						tb_depth = atoi(&parameters[0]);
						parameters[lenp - 3] = 0;
						int tb_moves = atoi(&parameters[lenp - 5]);
						int tb_threshold = atoi(&parameters[lenp - 2]);

						printf("==== TABLE BASE [ depth = %d  moves = %d  threshold = %d ]\n",
                               tb_depth, tb_moves, tb_threshold);

                        printf("from node id : ");
                        tb_start = atoi(fgets(btbuff, 16, stdin));
                        printf("to node id   : ");
                        tb_end = atoi(fgets(btbuff, 16, stdin));
                        tb_depth_count = tb_count(pgConn, tb_depth, tb_start, tb_end);

						TB_NODE *tbnode = (TB_NODE *)malloc((tb_depth_count+10) * sizeof(TB_NODE));
						int tb_slot = 0, tb_created = 0, tb_ignored = 0, nb_expanded = 0, tb_total_nodes = 0;
                        int nb_tb_nodes = tb_nodes(pgConn, tb_depth, tb_start, tb_end, &tbnode[0]);
						printf("expanding %d nodes from %d to %d...\n", nb_tb_nodes, tb_start, tb_end);
						gettimeofday(&t0_game, NULL);

						for (int tbidx = 0 ; tbidx < nb_tb_nodes ; tbidx++)
						{
							if (tbnode[tbidx].id >= tb_start && tbnode[tbidx].id <= tb_end &
                                tbnode[tbidx].eval >= tb_threshold && tbnode[tbidx].eval <= 100.0 - tb_threshold)
							{
								//----- reset states
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
								//------
								for (int d = 0 ; d < tb_depth ; d++)
								{
									strcpy(tb_move, &tbnode[tbidx].code[2 * d]);
									tb_move[2] = 0;
									tb_slot = find_slot(&board, tb_move);
									if (tb_slot < 0)
									{
										printf("move %s not found, depth = %d, node %d/%d\n", tb_move, d, tbidx, nb_tb_nodes);
										exit(-1);
									}
									if (orientation == 'H')
									{
										if (move(&board, &state_h, &state_v, tb_slot, orientation))
										{
											orientation = 'V';
											current_state = &state_v;
										}
										else
										{
											printf("TB-ERROR depth = %d/%d  H sid = %d\n", d, tb_depth, tb_slot);
											exit(-1);
										}
									}
									else
									{
										if (move(&board, &state_v, &state_h, tb_slot, orientation))
										{
											orientation = 'H';
											current_state = &state_h;
										}
										else
										{
											printf("TB-ERROR depth = %d/%d  V sid = %d\n", d, tb_depth, tb_slot);
											exit(-1);
										}
									}
									move_number++;
									//printf("board move #%d : %3d / %s\n", move_number, mcts_root[d], board.slot[mcts_root[d]].code);
								}
								//------
								dv0 = 100.0 - eval_orientation(&board, current_state, orientation, RD3(lambda_decay, random_decay), 0.0, 0.0, 0.0, true);
								int tb_nb_moves = state_moves(&board, current_state, orientation, opponent_decay, &zemoves[0]);
								for (int itbm = 0 ; itbm < tb_nb_moves && itbm < tb_moves ; itbm++)
								{
									sprintf(child_code, "%s%s", tbnode[tbidx].code, board.slot[zemoves[itbm].idx].code);
									if (tb_insert_node(pgConn, tb_depth + 1, tbnode[tbidx].id, board.slot[zemoves[itbm].idx].code, child_code))
										tb_created++;
										//printf("id %d - %s : %.2f\n", tbnode[tbidx].id, tbnode[tbidx].code, dv0);
									else
										tb_errors++;
										//printf("TB-ERROR save %d\n", tbnode[tbidx].id);
								}
								//------
								nb_expanded++;
							}
							else tb_ignored++;
                            
if (nb_tb_nodes > 5000 && tbidx % 1000 == 0 && tbidx > 0)
    printf("---- TB %7d / %-7d  %6.2f %%  [ %d <= %8d <= %d]\n",
           tbidx, nb_tb_nodes, 100.0*tbidx/nb_tb_nodes,
           tb_start, tbnode[tbidx].id, tb_end);
						}
						free(tbnode);
						gettimeofday(&tend_game, NULL);
printf("%d/%d nodes expanded, %d ignored, %d created, %d errors  ( %s )  ( %5.1f / %5.1f n/s )\n",
    nb_expanded, nb_tb_nodes, tb_ignored, tb_created, tb_errors,
    format_duration(btbuff, (int)duration(&t0_game, &tend_game)/1000),
    (double)nb_expanded / (double)nb_tb_nodes, (double)tb_created / (double)nb_tb_nodes
    );
					}
                } // ========== END TABLE BASE ==========
				else if (strcmp("mcts", action) == 0) // ZE-MCTS
				{
					MCTS    root_node, znode, best_child, node_2_simulate;
					int     mcts_root[32], mcts_node[32], nb_mcts = 100;

					if (strlen(parameters) > 0)
						nb_mcts = atoi(parameters);
					gettimeofday(&t0_session, NULL);

				    for (int iloop = 0 ; iloop < nb_mcts ; iloop++)
				    {
					if (!find_mcts_node(pgConn, 0, &root_node))
					{
						printf("MCTS-ERROR cannot find root node iloop = %d\n", iloop);
						exit(-1);
					}
				        memcpy(&znode, &root_node, sizeof(MCTS));
if (nb_mcts < 100)
	printf("---- mcts %4d / %-4d ----\n", iloop, nb_mcts);
				        //----- reset states
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
				        //------
				        mcts_node[0] = 0;
				        while (best_ucb_child(pgConn, &znode, exploration, &best_child))
				        {
						if (strlen(root) >= 2 * best_child.depth)
						{
							char root_node[64];
							strcpy(root_node, &root[2 * best_child.depth - 2]);
							root_node[2] = 0;
							int nsid = find_slot(&board, root_node);
							if (nsid >= 0)
							{
								mcts_root[best_child.depth] = nsid;
								mcts_node[best_child.depth] = find_children(pgConn, best_child.parent, nsid, &znode);
	//printf("mcts root[%d]   node = %d  sid = %d  %s\n", best_child.depth, mcts_node[best_child.depth], nsid, root_node);
								if (nsid < 0 || mcts_node[best_child.depth] < 0)
								{
									mcts_root[best_child.depth] = best_child.sid;
									mcts_node[best_child.depth] = best_child.id;
									memcpy(&znode, &best_child, sizeof(MCTS));
								}
							}
							else
								printf("MCTS-ERROR: root node %s NOT FOUND, root = %s\n", root_node, root);
						}
						else
						{
							mcts_root[best_child.depth] = best_child.sid;
							mcts_node[best_child.depth] = best_child.id;
							memcpy(&znode, &best_child, sizeof(MCTS));
						}
				        }
if (nb_mcts < 100)
	printf("node %d : score = %.4f  visits = %4d  depth = %d   %s : %s\n", znode.id, znode.score, znode.visits, znode.depth, znode.move, znode.code);
				        //--------------------------------------------------------------
				        for (int d = 1 ; d <= znode.depth ; d++)
				        {
						if (strncmp(board.slot[mcts_root[d]].code, &znode.code[2*d-2], 2) != 0)
						{
							printf("MCTS-ERROR-DEPTH-SID d = %d  sid = %d/%s  %s\n",
								d, mcts_root[d], board.slot[mcts_root[d]].code, znode.code);
							exit(-1);
						}
						if (orientation == 'H')
						{
							if (move(&board, &state_h, &state_v, mcts_root[d], orientation))
							{
								orientation = 'V';
								current_state = &state_v;
							}
							else
							{
								printf("MCTS-ERROR depth = %d/%d  H sid = %d\n", d, znode.depth, mcts_root[d]);
								printf("nodes\n");
								for (int ddd = 1 ; ddd <= znode.depth ; ddd++) printf("%d ", mcts_node[ddd]);
								printf("slots\n");
								for (int ddd = 1 ; ddd <= znode.depth ; ddd++) printf("%d ", mcts_root[ddd]);
								exit(-1);
							}
						}
						else
						{
							if (move(&board, &state_v, &state_h, mcts_root[d], orientation))
							{
								orientation = 'H';
								current_state = &state_h;
							}
							else
							{
								printf("MCTS-ERROR depth = %d/%d  V sid = %d\n", d, znode.depth, mcts_root[d]);
								printf("nodes\n");
								for (int ddd = 1 ; ddd <= znode.depth ; ddd++) printf("%d ", mcts_node[ddd]);
								printf("slots\n");
								for (int ddd = 1 ; ddd <= znode.depth ; ddd++) printf("%d ", mcts_root[ddd]);
								exit(-1);
							}
						}
						move_number++;
						//printf("board move #%d : %3d / %s\n", move_number, mcts_root[d], board.slot[mcts_root[d]].code);
				        }
				        //--------------------------------------------------------------
				        if (znode.depth > 0 && ((znode.visits == 0 || znode.ratio >= 0.98) ||
                            (znode.visits > 0 || znode.ratio <= 0.02))
                        ) // simulation
				        {
                            memcpy(&node_2_simulate, &znode, sizeof(MCTS));
                            mcts_node[znode.depth] = znode.id;
				        }
				        else // node expansion
				        {
                            double dv0 = eval_orientation(&board, current_state, orientation,
                                                    RD3(lambda_decay, random_decay), 0.0, 0.0, 0.0, true);
                            int nb_moves = state_moves(&board, current_state, orientation, opponent_decay, &zemoves[0]);
                            int fchild = 0, nb_new_moves = 0;
                            for (int m = 0 ; m < nb_moves && nb_new_moves < max_moves ; m++)
                            {
bool mfound = false;
int imov = 0;
while (!mfound && imov < m)
{
if (zemoves[imov].idx == zemoves[m].idx)
{
printf("MCTS-ERROR-MOVE-DUPLICATE %d/%d  %s\n", imov, m, znode.code);
mfound = true;
}
imov++;
}
if (zemoves[m].idx < 0 || zemoves[m].idx >= board.slots)
{
	printf("MCTS-ERROR  sid = %d\n", zemoves[m].idx);
	exit(-1);
}
                            if (!find_move(current_state, zemoves[m].idx))
                            //if (strstr(znode.code, board.slot[zemoves[m].idx].code) == NULL)
                            {
                                char strmove[8];
                                strcpy(strmove, board.slot[zemoves[m].idx].code);
                                strmove[2] = 0;
if (strmove[0] > 'L' || strmove[0] < 'A' || strmove[1] > 'L' || strmove[1] < 'A')
{
    printf("MCTS-ERROR  sid = %d  %s\n", zemoves[m].idx, strmove);
    exit(-1);
}
                                sprintf(current_game_moves, "%s%s", znode.code, strmove);
                                int c = insert_mcts(pgConn, znode.depth+1, znode.id, zemoves[m].idx,
                                    strmove, current_game_moves, 0, 0.0);
                                if (fchild == 0) fchild = c;
                                nb_new_moves++;
                            }
                            else
                                printf("MCTS-ERROR: %3d/%s in node %d / %s\n",
                                        zemoves[m].idx, board.slot[zemoves[m].idx].code, znode.id, znode.code);
                            }
if (nb_mcts < 100)
	printf("++++ node %d / %s expanded, first child = %d    depth = %d  visits = %d  orientation = %c\n", znode.id, znode.code, fchild, znode.depth, znode.visits, orientation);
						mcts_node[znode.depth+1] = fchild;
						find_mcts_node(pgConn, fchild, &node_2_simulate);
						mcts_root[znode.depth+1] = node_2_simulate.sid;

                            if (orientation == 'H')
                            {
                                move(&board, &state_h, &state_v, mcts_root[znode.depth+1], orientation);
                                orientation = 'V';
                                current_state = &state_v;
                            }
                            else
                            {
                                move(&board, &state_v, &state_h, mcts_root[znode.depth+1], orientation);
                                orientation = 'H';
                                current_state = &state_h;
                            }
                            move_number++;
                            //printf("board move #%2d : %3d / %s\n", move_number, mcts_root[znode.depth+1], board.slot[mcts_root[znode.depth+1]].code);
				        } // end node expansion
				        //--------------------------------------------------------------
				        // call run simulation node + params hpmin-max vp-min-max_lambda
				        //--------------------------------------------------------------
				        //sleep(1);
                        double score_simulation = eval_orientation(&board, current_state, orientation,
                                                        lambda_decay, 0.0, 0.0, 0.0, false);
if (nb_mcts < 100)
	printf("simulate %d / %s  depth = %d  orientation = %c  score = %6.2f\n", node_2_simulate.id, node_2_simulate.code, node_2_simulate.depth, orientation, score_simulation);
				        //--------------------------------------------------------------
				        // retro propagate branch
				        //--------------------------------------------------------------
                        int iddx = 0;
                        for (int dd = node_2_simulate.depth ; dd >= 0 ; dd--)
                        {
                                if (find_mcts_node(pgConn, mcts_node[dd], &znode))
                                {
                                    if (update_mcts(pgConn, mcts_node[dd],
iddx % 2 == 0 ? znode.score + (100.0 - score_simulation)/100.0 : znode.score + score_simulation/100.0,
                                        znode.visits)) {
                                            //printf("node %3d updated\n", mcts_node[dd]);
                                    }
                                    else printf("ERROR: node %d NOT updated !!!\n", mcts_node[dd]);
                                }
                                iddx++;
                        }
if (nb_mcts > 100 && iloop % 100 == 0 && iloop > 0)
    printf("---- mcts %7d / %-7d  %6.2f %%\n", iloop, nb_mcts, 100.0*iloop/nb_mcts);
				    }
					// ---
					gettimeofday(&tend_session, NULL);
					printf("\n---- MCTS %4d in %.2f sec, avg = %.2f sec/iteration\n",
					nb_mcts, duration(&t0_session, &tend_session)/1000, 1.0*duration(&t0_session, &tend_session)/(1000.0*nb_mcts));
				}
				else if (strcmp("chkmcts", action) == 0)
				{
					chk_mcts_moves(pgConn, &board);
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
						if (pvalue != NULL && *pvalue != 0)
						{
							double dvalue = atof(pvalue);
							int ivalue = atoi(pvalue);
							bool bp = true;
							if (strcmp(parameters, "lambda-decay") == 0) lambda_decay = dvalue;
							else if (strcmp(parameters, "opponnent-decay") == 0) opponent_decay = dvalue;
							else if (strcmp(parameters, "max-moves") == 0) max_moves = ivalue;
							else if (strcmp(parameters, "wpegs") == 0) wpegs = dvalue;
							else if (strcmp(parameters, "wlinks") == 0) wlinks = dvalue;
							else if (strcmp(parameters, "wzeta") == 0) wzeta = dvalue;
							else if (strcmp(parameters, "hp-min") == 0) hp_min = ivalue;
							else if (strcmp(parameters, "hp-max") == 0) hp_max = ivalue;
							else if (strcmp(parameters, "vp-min") == 0) vp_min = ivalue;
							else if (strcmp(parameters, "vp-max") == 0) vp_max = ivalue;
							else if (strcmp(parameters, "offset") == 0) offset = ivalue;
							else if (strcmp(parameters, "live-timeout") == 0) live_timeout = ivalue;
							else if (strcmp(parameters, "live-loop") == 0) live_loop = ivalue;
							else if (strcmp(parameters, "wait-live") == 0) wait_live = ivalue;
							else if (strcmp(parameters, "alpha-ratio") == 0) alpha_ratio = dvalue;
							else if (strcmp(parameters, "random-decay") == 0) random_decay = dvalue;
							else if (strcmp(parameters, "elo-coef") == 0) elo_coef = dvalue;
							else if (strcmp(parameters, "exploration") == 0) exploration = dvalue;
							else if (strcmp(parameters, "mcts-min-ratio") == 0) mcts_min_ratio = dvalue;
							else if (strcmp(parameters, "mcts-min-visits") == 0) mcts_min_visits = ivalue;
							else if (strcmp(parameters, "tb-depth") == 0) db_tb_max_depth = ivalue;
							else bp = false;
							if (bp) printf("parameter %s set to %6.3f\n", parameters, dvalue);
						}
					}
				}
				else if (strcmp("root", action) == 0)
					strcpy(root, parameters);
				else if (strcmp("parameters", action) == 0)
				{
					printf("lambda-decay   = %6.3f\n", lambda_decay);
					printf("opponent-decay = %6.3f\n", opponent_decay);
					printf("max-moves      = %3d\n", max_moves);
					printf("wpegs          = %6.3f\n", wpegs);
					printf("wlinks         = %6.3f\n", wlinks);
					printf("wzeta          = %6.3f\n", wzeta);
					printf("hp-min         = %3d\n", hp_min);
					printf("hp-max         = %3d\n", hp_max);
					printf("vp-min         = %3d\n", vp_min);
					printf("vp-max         = %3d\n", vp_max);
					printf("offset         = %2d\n", offset);
					printf("live-timeout   = %3d\n", live_timeout);
					printf("live-loop      = %3d\n", live_loop);
					printf("wait-live      = %4d\n", wait_live);
					printf("root           = %s\n", root);
					printf("alpha-ratio    = %5.2f\n", alpha_ratio);
					printf("random-decay   = %5.2f\n", random_decay);
					printf("elo-coef       = %5.2f\n", elo_coef);
					printf("exploration    = %5.2f\n", exploration);
					printf("mcts-min-visits= %5d\n", mcts_min_visits);
					printf("mcts-min-ratio = %5.2f\n", mcts_min_ratio);
					printf("tb-depth       = %4d\n", db_tb_max_depth);
				}
				else if (strcmp("position", action) == 0) //
				{
					char zsmov[512];
					int lenp = strlen(current_game_moves)/2;
					printf("moves = %s  (%d)\n", current_game_moves, lenp);
					printf("trait = %c\n", orientation);
					printf("      {");
					for (int imove = 0 ; imove < lenp ; imove++)
					{
						zsmov[0] = current_game_moves[2*imove];
						zsmov[1] = current_game_moves[2*imove+1];
						zsmov[2] = 0;
						printf(" %d-%s", imove+1, zsmov);
					}
					printf(" }\n");
					printf("sign  = %s\n\n", state_signature(&board, current_state, zsmov));
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
	printf("lambda %02d:  [%6.4f] x    %6.2f %%  = (%7dw%7dp%7dl  %6.4f %6.4f)   /  %6.2f %%  = (%7dw%7dp%7dl  %6.4f %6.4f)  : %02d\n",
		lambda,
		current_state->horizontal.lambda[lambda].weight,
		current_state->horizontal.lambda[lambda].score, current_state->horizontal.lambda[lambda].waves,
		current_state->horizontal.lambda[lambda].pegs, current_state->horizontal.lambda[lambda].links,
		current_state->horizontal.lambda[lambda].spread, current_state->horizontal.lambda[lambda].cross,
		current_state->vertical.lambda[lambda].score, current_state->vertical.lambda[lambda].waves,
		current_state->vertical.lambda[lambda].pegs, current_state->vertical.lambda[lambda].links,
		current_state->vertical.lambda[lambda].spread, current_state->vertical.lambda[lambda].cross,
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
					printf("slots        = %-4d      %6ld MB  %8d\n", board.slots, sz_slots, slots_max);
					printf("steps        = %-4d      %6ld MB  %8d\n", board.steps, sz_steps, steps_max);
					printf("total        =           %6ld MB\n", (board.horizontal.paths + board.vertical.paths) * sizeof(PATH) / 1000000 + sz_slots + sz_steps);
					printf("field        =           %6ld MB\n", (board.horizontal.paths + board.vertical.paths) * sizeof(WAVE) / 1000000);

				}
				else if (strcmp("help", action) == 0)
				{
					printf("\tmove XY\n");
					printf("\tthink <depth>/<max-moves>\n");
					printf("\tplay <depth>/<max-moves>\n");
					printf("\tgame <depth>/<max-moves>\n");
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
					printf("\tload <game>/<moves>\n");
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
