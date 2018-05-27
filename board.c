

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>

#include "board.h"


static int gslots = 0, gsteps = 0;

int find_slot(BOARD *board, char *pslot)
{
int slot = -1;

	for (int s = 0 ; s < board->slots ; s++)
	{
		if (strcmp(pslot, board->slot[s].code) == 0)
		{
			slot = s;
			break;
		}
	}
	return slot;
}

int find_xy(BOARD *board, int sx, int sy)
{
int slot = -1;

	for (int s = 0 ; s < board->slots ; s++)
	{
		if (sx == board->slot[s].x && sy == board->slot[s].y)
		{
			slot = s;
			break;
		}
	}
	return slot;
}

void printSlot(BOARD *board, SLOT *pslot)
{
char neighbors[128], buf[32];

	neighbors[0] = 0;
	for (int n = 0 ; n < pslot->neighbors ; n++)
	{
		if (n == 0)
			sprintf(buf, "%d/%d/%s", pslot->neighbor[n], pslot->link[n], board->slot[pslot->neighbor[n]].code);
		else
			sprintf(buf, ", %d/%d/%s", pslot->neighbor[n], pslot->link[n], board->slot[pslot->neighbor[n]].code);
		strcat(neighbors, buf);
	}
	printf("slot %3d: x=%02d y=%02d  code=%s  %d neighbors {%s}  hpaths=%d  vpaths=%d\n", pslot->idx, pslot->x, pslot->y, pslot->code,
		pslot->neighbors, neighbors, pslot->hpaths, pslot->vpaths);
}

void debugStep(STEP *pstep)
{
char cuts[64], buf[16];

	cuts[0] = 0;
	for (int n = 0 ; n < pstep->cuts ; n++)
	{
		if (n == 0)
			sprintf(buf, "%d", pstep->cut[n]);
		else
			sprintf(buf, ", %d", pstep->cut[n]);
		strcat(cuts, buf);
	}
	printf("step: %d/%s  sx=%d sy=%d  sign=%d  from=%d to=%d  %d cuts {%s}\n",
		pstep->idx, pstep->code, pstep->sx, pstep->sy, pstep->sign,
		pstep->from, pstep->to, pstep->cuts, cuts);
}

void printStep(BOARD *board, int step)
{
char cuts[128], buf[32];
STEP *pstep = &board->step[step];

	cuts[0] = 0;
	for (int n = 0 ; n < pstep->cuts ; n++)
	{
		if (n == 0)
			sprintf(buf, "%d/%s", pstep->cut[n], board->step[pstep->cut[n]].code);
		else
			sprintf(buf, ", %d/%s", pstep->cut[n], board->step[pstep->cut[n]].code);
		strcat(cuts, buf);
	}
	printf("step %d/%s : %d cut {%s}\n",
		pstep->idx, pstep->code, pstep->cuts, cuts);
}

void printPath(BOARD *board, PATH *path)
{
char slots[128], steps[256], buf[32];

	slots[0] = 0;
	for (int s = 0 ; s < path->slots ; s++)
	{
		if (s == 0)
			sprintf(buf, "%3d/%-4s", path->slot[s], board->slot[path->slot[s]].code);
		else
			sprintf(buf, ", %3d/%-4s", path->slot[s], board->slot[path->slot[s]].code);
		strcat(slots, buf);
	}
	printf("%2d slots : {%s}\n", path->slots, slots);
	steps[0] = 0;
	for (int s = 0 ; s < path->steps ; s++)
	{
		if (s == 0)
			sprintf(buf, "%3d/%4s", path->step[s], board->step[path->step[s]].code);
		else
			sprintf(buf, ", %3d/%4s", path->step[s], board->step[path->step[s]].code);
		strcat(steps, buf);
	}
	printf("%2d steps : {%s}\n", path->steps, steps);
}

int SearchPathSlot(PATH *path, unsigned short slot)
{
int idx = -1;

	for (int s = 0 ; s < path->slots ; s++)
	{
		if (path->slot[s] == slot)
		{
			idx = s;
			break;
		}
	}
	return idx;
}

int SearchPathStep(PATH *path, unsigned short step)
{
int idx = -1;

	for (int s = 0 ; s < path->steps ; s++)
	{
		if (path->step[s] == step)
		{
			idx = s;
			break;
		}
	}
	return idx;
}

void free_board(BOARD *board)
{
	for (int slot = 0 ; slot < board->slots ; slot++)
	{
		if (board->slot[slot].hpaths > 0 && board->slot[slot].hpath != NULL)
		{
			free(board->slot[slot].hpath);
			board->slot[slot].hpaths = 0;
		}
		if (board->slot[slot].vpaths > 0 && board->slot[slot].vpath != NULL)
		{
			free(board->slot[slot].vpath);
			board->slot[slot].vpaths = 0;
		}
	}
	for (int step = 0 ; step < board->steps ; step++)
	{
		if (board->step[step].hpaths > 0 && board->step[step].hpath != NULL)
		{
			free(board->step[step].hpath);
			board->step[step].hpaths = 0;
		}
		if (board->step[step].vpaths > 0 && board->step[step].vpath != NULL)
		{
			free(board->step[step].vpath);
			board->step[step].vpaths = 0;
		}
	}
}

void checkSlotPaths(BOARD *board, int slot)
{
int hpf = 0, vpf = 0, s;
bool bf = false;

	for (int ihp = 0 ; ihp < board->slot[slot].hpaths ; ihp++)
	{
		int hp = board->slot[slot].hpath[ihp];
		bf = false;
		s = 0;
		while (!bf && s < board->horizontal.path[hp].slots)
		{
			if (board->horizontal.path[hp].slot[s] == slot) bf = true;
			s++;
		}
		if (bf) hpf++;
	}
	for (int ivp = 0 ; ivp < board->slot[slot].vpaths ; ivp++)
	{
		int vp = board->slot[slot].vpath[ivp];
		bf = false;
		s = 0;
		while (!bf && s < board->vertical.path[vp].slots)
		{
			if (board->vertical.path[vp].slot[s] == slot) bf = true;
			s++;
		}
		if (bf) vpf++;
	}
	printf("checkSlotPaths(%d): hp %d/%d  vp %d/%d\n", slot, hpf, board->slot[slot].hpaths, vpf, board->slot[slot].vpaths);
}

void search_path_v(BOARD *board, int slot, int step, int depth,
			int slots, unsigned short *pslot,
			int steps, unsigned short *pstep,
			int max_depth, int min_dy)
{
	for (int s = 0 ; s < slots ; s++)
	{
		if (pslot[s] == slot) return;
	}
	pslot[slots] = slot;
	slots++;
	if (step >= 0)
	{
		pstep[steps] = step;
		steps++;
	}

	if (board->slot[slot].y == board->height-1)
	{
		board->vertical.path[board->vertical.paths].slots = slots;
		for (int s = 0 ; s < slots ; s++)
		{
			int sslot = pslot[s];
			if (sslot < 0 || sslot >= board->slots)
				printf("error slot %d %d\n", s, sslot);
			else
			{
				board->vertical.path[board->vertical.paths].slot[s] = sslot;
				// slots array
				board->slot[sslot].vpath[board->slot[sslot].vpaths] = board->vertical.paths;
				board->slot[sslot].vpaths++;
				if (board->slot[sslot].vpaths > MAX_PATH_PER_SLOT)
					printf("error.vpath.slot %d: %d", sslot, board->slot[sslot].vpaths);
			}
			gslots++;
		}
		// sort slots ?
		board->vertical.path[board->vertical.paths].steps = steps;
		for (int s = 0 ; s < steps ; s++)
		{
			int sstep = pstep[s];
			if (sstep < 0 || sstep >= board->steps)
				printf("error step %d %d\n", s, sstep);
			else
			{
				board->vertical.path[board->vertical.paths].step[s] = sstep;
				// steps array
				board->step[sstep].vpath[board->step[sstep].vpaths] = board->vertical.paths;
				board->step[sstep].vpaths++;
				if (board->step[sstep].vpaths > MAX_PATH_PER_STEP)
					printf("error.vpath.step %d: %d", sstep, board->step[sstep].vpaths);
			}
			gsteps++;
		}
		// sort steps ?

		board->vertical.paths++;
	}
	else if (board->slot[slot].x == 0 || board->slot[slot].x == board->width-1 ||
		(board->height - board->slot[slot].y -1 ) > 2 * (max_depth - depth))
	{
		return;
	}
	else if (depth < max_depth)
	{
		for (int n = 0 ; n < board->slot[slot].neighbors ; n++)
		{
			int sn = board->slot[slot].neighbor[n];
			if (board->slot[sn].y - board->slot[slot].y >= min_dy)
			{
search_path_v(board, sn, board->slot[slot].link[n], depth+1, slots, pslot, steps, pstep, max_depth, min_dy);
			}
		}
	}
}

void search_path_h(BOARD *board, int slot, int step, int depth,
			int slots, unsigned short *pslot,
			int steps, unsigned short *pstep,
			int max_depth, int min_dx)
{
	for (int s = 0 ; s < slots ; s++)
	{
		if (pslot[s] == slot) return;
	}
	pslot[slots] = slot;
	slots++;
	if (step >= 0)
	{
		pstep[steps] = step;
		steps++;
	}

	if (board->slot[slot].x == board->width-1)
	{
		board->horizontal.path[board->horizontal.paths].slots = slots;
		for (int s = 0 ; s < slots ; s++)
		{
			int sslot = pslot[s];
			if (sslot < 0 || sslot >= board->slots)
				printf("error slot %d %d\n", s, sslot);
			else
			{
				board->horizontal.path[board->horizontal.paths].slot[s] = sslot;
				// slots array
				board->slot[sslot].hpath[board->slot[sslot].hpaths] = board->horizontal.paths;
				board->slot[sslot].hpaths++;
				if (board->slot[sslot].hpaths > MAX_PATH_PER_SLOT)
					printf("error.hpath.slot %d: %d", sslot, board->slot[sslot].hpaths);
			}
			gslots++;
		}
		// sort slots ?
		board->horizontal.path[board->horizontal.paths].steps = steps;
		for (int s = 0 ; s < steps ; s++)
		{
			int sstep = pstep[s];
			if (sstep < 0 || sstep >= board->steps)
				printf("error step %d %d\n", s, sstep);
			else
			{
				board->horizontal.path[board->horizontal.paths].step[s] = sstep;
				// steps array
				board->step[sstep].hpath[board->step[sstep].hpaths] = board->horizontal.paths;
				board->step[sstep].hpaths++;
				if (board->step[sstep].hpaths > MAX_PATH_PER_STEP)
					printf("error.hpath.step %d: %d", sstep, board->step[sstep].hpaths);
			}
			gsteps++;
		}
		// sort steps ?

		board->horizontal.paths++;
	}
	else if (board->slot[slot].y == 0 || board->slot[slot].y == board->height-1 ||
		(board->width - board->slot[slot].x -1 ) > 2 * (max_depth - depth))
	{
		return;
	}
	else if (depth < max_depth)
	{
		for (int n = 0 ; n < board->slot[slot].neighbors ; n++)
		{
			int sn = board->slot[slot].neighbor[n];
			if (board->slot[sn].x - board->slot[slot].x >= min_dx)
			{
search_path_h(board, sn, board->slot[slot].link[n], depth+1, slots, pslot, steps, pstep, max_depth, min_dx);
			}
		}
	}
}

bool isInPerimeter(STEP *pstep, int sx, int sy)
{
	return sx >= 2*pstep->xmin && sx <= 2*pstep->xmax && sy >= 2*pstep->ymin && sy <= 2*pstep->ymax;
}

int minPegDistance(STEP *pstep, int sx, int sy)
{
	int sqd1 = (sx - 2*pstep->x1)*(sx - 2*pstep->x1) + (sy - 2*pstep->y1)*(sy - 2*pstep->y1);
	int sqd2 = (sx - 2*pstep->x2)*(sx - 2*pstep->x2) + (sy - 2*pstep->y2)*(sy - 2*pstep->y2);

	if (sqd1 < sqd2)
		return sqd1;
	else
		return sqd2;
}

unsigned long init_board(BOARD *board, int width, int height, int depth, int min_direction)
{
	bzero(board, sizeof(BOARD));
	board->width = width;
	board->height = height;

	// slots
	board->slots = 0;
	long size_slots = 0;
	for (int y = 0 ; y < height ; y++)
	{
		for (int x = 0 ; x < width ; x++)
		{
			bool corner = ((x == 0 && y == 0) || (x == 0 && y == height - 1) ||
				(x == width - 1 && y == 0) || (x == width - 1 && y == height - 1));
			if (!corner)
			{
				board->slot[board->slots].x = x;
				board->slot[board->slots].y = y;
				board->slot[board->slots].idx = board->slots;
				board->slot[board->slots].neighbors = 0;
				board->slot[board->slots].hpaths = 0;
				board->slot[board->slots].hpath = malloc(MAX_PATH_PER_SLOT * sizeof(unsigned int));
				size_slots += MAX_PATH_PER_SLOT * sizeof(unsigned int);
				board->slot[board->slots].vpaths = 0;
				board->slot[board->slots].vpath = malloc(MAX_PATH_PER_SLOT * sizeof(unsigned int));
				size_slots += MAX_PATH_PER_SLOT * sizeof(unsigned int);
				sprintf(board->slot[board->slots].code, "%c%c", 'A'+x, 'A'+y);
				board->slots++;
			}
		}
	}
	printf("debug.slots = %5d  size = %ld MB\n", board->slots, size_slots/1000000);
	long size_steps = 0;
	// neighbors
	board->steps = 0;
	for (int s1 = 0 ; s1 < board->slots -1 ; s1++)
	{
		for (int s2 = s1 +1 ; s2 < board->slots ; s2++)
		{
			if ((board->slot[s2].x - board->slot[s1].x)*(board->slot[s2].x - board->slot[s1].x) + (board->slot[s2].y - board->slot[s1].y)*(board->slot[s2].y - board->slot[s1].y) == 5)
			{
				board->slot[s1].neighbor[board->slot[s1].neighbors] = s2;
				board->slot[s1].link[board->slot[s1].neighbors] = board->steps;
				board->slot[s1].neighbors++;

				board->slot[s2].neighbor[board->slot[s2].neighbors] = s1;
				board->slot[s2].link[board->slot[s2].neighbors] = board->steps;
				board->slot[s2].neighbors++;

				board->step[board->steps].from = s1;
				board->step[board->steps].to = s2;
				board->step[board->steps].idx = board->steps;
				board->step[board->steps].sx = board->slot[s2].x + board->slot[s1].x;
				board->step[board->steps].sy = board->slot[s2].y + board->slot[s1].y;
				double slope = (double)(board->slot[s2].y - board->slot[s1].y) / (double)(board->slot[s2].x - board->slot[s1].x);
				if (slope > 0.0)
					board->step[board->steps].sign = 1;
				else
					board->step[board->steps].sign = 2;

				board->step[board->steps].cuts = 0;
				board->step[board->steps].hpaths = 0;
				board->step[board->steps].hpath = malloc(MAX_PATH_PER_STEP * sizeof(unsigned int));
				size_steps += MAX_PATH_PER_STEP * sizeof(unsigned int);
				board->step[board->steps].vpaths = 0;
				board->step[board->steps].vpath = malloc(MAX_PATH_PER_STEP * sizeof(unsigned int));
				size_steps += MAX_PATH_PER_STEP * sizeof(unsigned int);
				sprintf(board->step[board->steps].code, "%c%c%c%c",
					'A'+board->slot[s1].x, 'A'+board->slot[s1].y,
					'A'+board->slot[s2].x, 'A'+board->slot[s2].y);

				if (board->slot[s1].x < board->slot[s2].x)
				{
					board->step[board->steps].xmin = board->slot[s1].x;
					board->step[board->steps].xmax = board->slot[s2].x;
				}
				else
				{
					board->step[board->steps].xmin = board->slot[s2].x;
					board->step[board->steps].xmax = board->slot[s1].x;
				}
				if (board->slot[s1].y < board->slot[s2].y)
				{
					board->step[board->steps].ymin = board->slot[s1].y;
					board->step[board->steps].ymax = board->slot[s2].y;
				}
				else
				{
					board->step[board->steps].ymin = board->slot[s2].y;
					board->step[board->steps].ymax = board->slot[s1].y;
				}
				board->step[board->steps].x1 = board->slot[s1].x;
				board->step[board->steps].x2 = board->slot[s2].x;
				board->step[board->steps].y1 = board->slot[s1].y;
				board->step[board->steps].y2 = board->slot[s2].y;

				board->steps++;
			}
		}
	}
	printf("debug.steps = %5d  size = %ld MB\n", board->steps, size_steps/1000000);
	int max_neighbors = 0;
	for (int s = 0 ; s < board->slots ; s++)
	{
		if (board->slot[s].neighbors > max_neighbors) max_neighbors = board->slot[s].neighbors;
	}
	printf("debug.max_neighbors = %d\n", max_neighbors);
	// cuts
	for (int s1 = 0 ; s1 < board->steps - 1; s1++)
	{
		for (int s2 = s1 + 1 ; s2 < board->steps ; s2++)
		{
			if (s1 != s2)
			{
int sqd = (board->step[s1].sx - board->step[s2].sx) * (board->step[s1].sx - board->step[s2].sx);
sqd += (board->step[s1].sy - board->step[s2].sy) * (board->step[s1].sy - board->step[s2].sy);
int mpd1 = minPegDistance(&board->step[s1], board->step[s2].sx, board->step[s2].sy);
int mpd2 = minPegDistance(&board->step[s2], board->step[s1].sx, board->step[s1].sy);

				if (
					(sqd == 0 && board->step[s1].sign != board->step[s2].sign)
					||
					(sqd == 4 &&
						isInPerimeter(&board->step[s1], board->step[s2].sx, board->step[s2].sy) &&
						isInPerimeter(&board->step[s2], board->step[s1].sx, board->step[s1].sy) &&
						board->step[s1].sign != board->step[s2].sign
					)
					||
					(sqd == 2 && ((mpd1 == 1 && mpd2 == 1) || board->step[s1].sign != board->step[s2].sign)
					)
				    )
				{
/*if (s1 == 311 && s2 == 313)
{
printf("sqd = %d  mpd1 = %d  mpd2 = %d  sign1 = %d  sign2 = %d\n", sqd, mpd1, mpd2, board->step[s1].sign, board->step[s2].sign);
printf("s1x = %d  s1y = %d   s2x = %d  s2y = %d\n", board->step[s1].sx, board->step[s1].sy, board->step[s2].sx, board->step[s2].sy);
printf("xmin1 = %d  xmax1 = %d   ymin1 = %d  ymax1 = %d\n", board->step[s1].xmin, board->step[s1].xmax, board->step[s1].ymin, board->step[s1].ymax);
printf("xmin2 = %d  xmax2 = %d   ymin2 = %d  ymax2 = %d\n", board->step[s2].xmin, board->step[s2].xmax, board->step[s2].ymin, board->step[s2].ymax);
}*/
					board->step[s1].cut[board->step[s1].cuts] = s2;
					board->step[s1].cuts++;
					board->step[s2].cut[board->step[s2].cuts] = s1;
					board->step[s2].cuts++;
				}
			}
		}
	}

	/*printf("mpd1  = %d\n", mpd1);
	printf("mpd5  = %d\n", mpd5);
	printf("mpd9  = %d\n", mpd9);
	printf("mpd13 = %d\n", mpd13);
	printf("mpdx  = %d\n", mpdx);*/

	int max_cuts = 0, sign = 0, sx = 0, sy = 0;
	for (int s = 0 ; s < board->steps ; s++)
	{
		sign += board->step[s].sign;
		sx += board->step[s].sx;
		sy += board->step[s].sy;
		if (board->step[s].cuts > max_cuts) max_cuts = board->step[s].cuts;
//printStep(&board->step[s]);
//printf("step[%03d].cuts = %d:", s, board->step[s].cuts);
//for (int icut = 0 ; icut < board->step[s].cuts ; icut++)
//	printf(" %d", board->step[s].cut[icut]);
//printf("\n");
	}
	printf("debug.max_cuts = %d\n", max_cuts);
	//printf("debug.avg_sign = %6.4f\n", (double)sign/board->steps);
	//printf("debug.avg_sx = %6.4f\n", (double)sx/board->steps);
	//printf("debug.avg_sy = %6.4f\n", (double)sy/board->steps);

	// paths
	unsigned short *slots = malloc(PATH_MAX_LENGTH*sizeof(unsigned short));
	unsigned short *steps = malloc(PATH_MAX_LENGTH*sizeof(unsigned short));

	long size_hpaths = MAX_PATHS * sizeof(PATH);
	board->horizontal.path = malloc(size_hpaths);
	board->horizontal.paths = 0;
	for (int s = 0 ; s < board->slots ; s++)
	{
		if (board->slot[s].x == 0)
			search_path_h(board, s, -1, 0, 0, slots, 0, steps, depth-1, min_direction);
	}
	printf("debug.horizontal_paths  = %10d             gslots = %10d  size = %ld MB\n",
			board->horizontal.paths, gslots, size_hpaths/ONE_MILLION);

	// slots paths
	int nb_slots_path = 0, nb_steps_path = 0;
	for (int p = 0 ; p < board->horizontal.paths ; p++)
	{
		nb_slots_path += board->horizontal.path[p].slots;
		nb_steps_path += board->horizontal.path[p].steps;
	}
	printf("debug.nb_slots_path                                     = %10d\n", nb_slots_path);

	int max_path_per_slot = 0, smax = 0, sump = 0, nbw = 0;
	for (int s = 0 ; s < board->slots ; s++)
	{
		if (board->slot[s].hpaths > max_path_per_slot)
		{
			max_path_per_slot = board->slot[s].hpaths;
			smax = s;
		}
		sump += board->slot[s].hpaths;
		if (board->slot[s].hpaths < 0) nbw++;
	}
	printf("debug.max_path_per_slot = %10d  %s   %5.2f %%  sum = %10d  nbw  = %d\n\n", max_path_per_slot,
			board->slot[smax].code, 100.0 * max_path_per_slot / board->horizontal.paths, sump, nbw);

	printf("debug.horizontal_paths  = %10d             gsteps = %10d\n", board->horizontal.paths, gsteps);
	printf("debug.nb_steps_path                                     = %10d\n", nb_steps_path);
	int max_path_per_step = 0;
	sump = 0; nbw = 0;
	for (int s = 0 ; s < board->steps ; s++)
	{
		if (board->step[s].hpaths > max_path_per_step)
		{
			max_path_per_step = board->step[s].hpaths;
			smax = s;
		}
		sump += board->step[s].hpaths;
		if (board->step[s].hpaths < 0) nbw++;
	}
	printf("debug.max_path_per_step = %10d  %s %5.2f %%  sum = %10d  nbw  = %d\n\n", max_path_per_step,
			board->step[smax].code, 100.0 * max_path_per_step / board->horizontal.paths, sump, nbw);


	// vertical

	gslots = 0; gsteps = 0;
	long size_vpaths = MAX_PATHS * sizeof(PATH);
	board->vertical.path = malloc(size_vpaths);
	board->vertical.paths = 0;
	for (int s = 0 ; s < board->slots ; s++)
	{
		if (board->slot[s].y == 0)
			search_path_v(board, s, -1, 0, 0, slots, 0, steps, depth-1, min_direction);
	}
	printf("debug.vertical_paths    = %10d             gslots = %10d  size = %ld MB\n",
			board->vertical.paths, gslots, size_vpaths/ONE_MILLION);

	// slots paths
	nb_slots_path = 0; nb_steps_path = 0;
	for (int p = 0 ; p < board->vertical.paths ; p++)
	{
		nb_slots_path += board->vertical.path[p].slots;
		nb_steps_path += board->vertical.path[p].steps;
	}
	printf("debug.nb_slots_path                                     = %10d\n", nb_slots_path);

	max_path_per_slot = 0; smax = 0; sump = 0; nbw = 0;
	for (int s = 0 ; s < board->slots ; s++)
	{
		if (board->slot[s].vpaths > max_path_per_slot)
		{
			max_path_per_slot = board->slot[s].vpaths;
			smax = s;
		}
		sump += board->slot[s].vpaths;
		if (board->slot[s].vpaths < 0) nbw++;
	}
	printf("debug.max_path_per_slot = %10d  %s   %5.2f %%  sum = %10d  nbw  = %d\n\n", max_path_per_slot,
			board->slot[smax].code, 100.0 * max_path_per_slot / board->vertical.paths, sump, nbw);

	printf("debug.vertical_paths    = %10d             gsteps = %10d\n", board->vertical.paths, gsteps);
	printf("debug.nb_steps_path                                     = %10d\n", nb_steps_path);
	max_path_per_step = 0;
	sump = 0; nbw = 0;
	for (int s = 0 ; s < board->steps ; s++)
	{
		if (board->step[s].vpaths > max_path_per_step)
		{
			max_path_per_step = board->step[s].vpaths;
			smax = s;
		}
		sump += board->step[s].vpaths;
		if (board->step[s].vpaths < 0) nbw++;
	}
	printf("debug.max_path_per_step = %10d  %s %5.2f %%  sum = %10d  nbw  = %d\n", max_path_per_step,
			board->step[smax].code, 100.0 * max_path_per_step / board->vertical.paths, sump, nbw);

	printf("debug.total_allocated   = %10ld  MB\n", (size_slots+size_steps+size_hpaths+size_vpaths) / ONE_MILLION);

	fflush(stdout);

	long paths_size_released = (2* MAX_PATHS - board->horizontal.paths - board->vertical.paths) * sizeof(PATH);
	PATH *hpath = board->horizontal.path;
	board->horizontal.path = malloc(board->horizontal.paths * sizeof(PATH));
	for (int ih = 0 ; ih < board->horizontal.paths ; ih++)
		memcpy(&board->horizontal.path[ih], &hpath[ih], sizeof(PATH));
	free(hpath);

	PATH *vpath = board->vertical.path;
	board->vertical.path = malloc(board->vertical.paths * sizeof(PATH));
	for (int ih = 0 ; ih < board->vertical.paths ; ih++)
		memcpy(&board->vertical.path[ih], &vpath[ih], sizeof(PATH));
	free(vpath);

	printf("debug.path_released     = %10ld  MB\n", paths_size_released / ONE_MILLION);

	//checkSlotPaths(board, 63);

	// slots release
	long slots_size_released = 0;
	for (int s = 0 ; s < board->slots ; s++)
	{
		if (board->slot[s].hpaths == 0)
		{
			slots_size_released += MAX_PATH_PER_SLOT * sizeof(unsigned int);
			free(board->slot[s].hpath);
		}
		else
		{
			slots_size_released += (MAX_PATH_PER_SLOT - board->slot[s].hpaths) * sizeof(unsigned int);
			unsigned int *ph = board->slot[s].hpath;
			board->slot[s].hpath = malloc(board->slot[s].hpaths * sizeof(unsigned int));
			for (int ih = 0 ; ih < board->slot[s].hpaths ; ih++)
			{
				board->slot[s].hpath[ih] = ph[ih];
			}
			free(ph);
		}
		//---------
		if (board->slot[s].vpaths == 0)
		{
			slots_size_released += MAX_PATH_PER_SLOT * sizeof(unsigned int);
			free(board->slot[s].vpath);
		}
		else
		{
			slots_size_released += (MAX_PATH_PER_SLOT - board->slot[s].vpaths) * sizeof(unsigned int);
			unsigned int *ph = board->slot[s].vpath;
			board->slot[s].vpath = malloc(board->slot[s].vpaths * sizeof(unsigned int));
			for (int ih = 0 ; ih < board->slot[s].vpaths ; ih++)
			{
				board->slot[s].vpath[ih] = ph[ih];
			}
			free(ph);
		}
	}
	printf("debug.slots_released    = %10ld  MB\n", slots_size_released / ONE_MILLION);
		// steps release
		long steps_size_released = 0;
		for (int s = 0 ; s < board->steps ; s++)
		{
			if (board->step[s].hpaths == 0)
			{
				steps_size_released += MAX_PATH_PER_STEP * sizeof(unsigned int);
				free(board->step[s].hpath);
			}
			else
			{
				steps_size_released += (MAX_PATH_PER_STEP - board->step[s].hpaths) * sizeof(unsigned int);
				unsigned int *ph = board->step[s].hpath;
				board->step[s].hpath = malloc(board->step[s].hpaths * sizeof(unsigned int));
				for (int ih = 0 ; ih < board->step[s].hpaths ; ih++)
				{
					board->step[s].hpath[ih] = ph[ih];
				}
				free(ph);
			}
			//---------
			if (board->step[s].vpaths == 0)
			{
				steps_size_released += MAX_PATH_PER_STEP * sizeof(unsigned int);
				free(board->step[s].vpath);
			}
			else
			{
				steps_size_released += (MAX_PATH_PER_STEP - board->step[s].vpaths) * sizeof(unsigned int);
				unsigned int *ph = board->step[s].vpath;
				board->step[s].vpath = malloc(board->step[s].vpaths * sizeof(unsigned int));
				for (int ih = 0 ; ih < board->step[s].vpaths ; ih++)
				{
					board->step[s].vpath[ih] = ph[ih];
				}
				free(ph);
			}
		}
		printf("debug.steps_released    = %10ld  MB\n", steps_size_released / ONE_MILLION);
		printf("debug.total_released    = %10ld  MB\n", (paths_size_released+slots_size_released+steps_size_released) / ONE_MILLION);
		unsigned long ul_total_allocated = (size_slots+size_steps+size_hpaths+size_vpaths-paths_size_released-slots_size_released-steps_size_released) / ONE_MILLION;
		printf("debug.total_allocated   = %10ld  MB\n", ul_total_allocated);

	return ul_total_allocated;
}
