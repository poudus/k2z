
#include <string.h>
#include <stdio.h>
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

	clone_state(state_from, state_to);
	gettimeofday(&t0, NULL);
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
	if (init_board(&board, width, height, 10, -1))
	{
		gettimeofday(&t_init_board, NULL);
		printf("init.board %6.2f ms  complexity = %d\n", duration(&t0, &t_init_board), board.horizontal.paths+board.vertical.paths);

		STATE state_h, state_v, *current_state;
		current_state = &state_h;
		long size_s0 = init_state(&state_h, board.horizontal.paths, board.vertical.paths);
		gettimeofday(&t_init_s0, NULL);
		printf("init.s0     %6.2f ms        size = %ld MB\n", duration(&t_init_board, &t_init_s0), size_s0 / ONE_MILLION);

		char command[256], action[256], parameters[256], orientation;
		int move_number = 0;
		printf("k2z> ");
		orientation = 'H';
		while (1)
		{
			fgets(command, 256, stdin);
			sscanf(command, "%s %s", action, parameters);
			if (strcmp("quit", action) == 0 || strcmp("exit", action) == 0)
				break;
			else if (strcmp("move", action) == 0)
			{
				int slot = atoi(parameters);
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
			else if (strcmp("slot", action) == 0)
			{
				int slot = atoi(parameters);
				printSlot(&board.slot[slot]);
			}
			else if (strcmp("step", action) == 0)
			{
				int step = atoi(parameters);
				printStep(&board.step[step]);
			}
			printf("k2z> ");
		}
		printf("bye.\n");
		/*move(&board, &s0, &s1, 40, 'H');
		move(&board, &s1, &s2, 42, 'V');
		move(&board, &s2, &s3, 54, 'H');
		move(&board, &s3, &s4, 65, 'V');*/

		//printStep(STEP *pstep);

	}
	else
	{
		printf("error.init\n");
	}
}
