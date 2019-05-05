
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <sys/time.h>

#include "board.h"
#include "state.h"

static double epsilon = 0.000001;
static int gnid = 0;

typedef struct node_mcts
{
	char	move[4];
	double	value;
	int	id, depth, msid, visits, children;
	struct node_mcts*	parent;
	struct node_mcts*	child[32];
} MCTS_NODE;

double ucb(MCTS_NODE *node, double exploration)
{
	if (node->parent == NULL)
		return -1.0;
	else
		return node->value / (node->visits + epsilon) +
			exploration * sqrt(log((node->parent->visits+1)/(node->visits + epsilon))) +
			rand()*epsilon/RAND_MAX;
}

MCTS_NODE* retro_propagate(MCTS_NODE* node, double value)
{
	node->visits++;
	node->value += value;
	if (node->parent == NULL)
		return node;
	else
		return retro_propagate(node->parent, value);
}


MCTS_NODE* new_node(MCTS_NODE*parent, int msid, const char *move)
{
	MCTS_NODE* nn = malloc(sizeof(MCTS_NODE));
	nn->id = gnid;
	nn->parent = parent;
	if (parent == NULL)
		nn->depth = 0;
	else
		nn->depth = parent->depth + 1;
	strcpy(nn->move, move);
	nn->msid = msid;
	nn->value = 0.0;
	nn->visits = 0;
	nn->children = 0;
	gnid++;
	return nn;
}

bool add_child(MCTS_NODE* parent, MCTS_NODE* child)
{
	parent->child[parent->children] = child;
	parent->children++;
}

bool leaf(MCTS_NODE* node)
{
	return node->children == 0;
}

MCTS_NODE* best_child_node(MCTS_NODE* node, double exploration)
{
MCTS_NODE* best_node = NULL;
double	value, best_value = -1.0;

	for (int c = 0 ; c < node->children ; c++)
	{
		value = ucb(node->child[c], exploration);
		if (value > best_value)
		{
			best_value = value;
			best_node = node->child[c];
		}
	}
	return best_node;
}

double simulate(BOARD* board, STATE* state, int max_moves, double decay, double opponent)
{
	return (rand() % 100) / 100.0;
}

int available_moves(BOARD* board, STATE* state, char orientation, int max_moves, double lambda_decay, double opponent_decay, TRACK *moves)
{
	eval_orientation(board, state, orientation, lambda_decay, 0.0, 0.0, 0.0, true);
	int nb_moves = state_moves(board, state, orientation, opponent_decay, moves);
        /* while (sid < 0)
        {
            sid = zemoves[rand() % max_moves].idx;
            if (find_move(state, sid)) sid = -1;
        }*/
        
	return max_moves;
}

void iteration(MCTS_NODE* head, double exploration, BOARD* board, STATE* state, int max_moves, double decay, double opponent)
{
MCTS_NODE* node = head;
TRACK zemoves[256];

	while (node != NULL)
	{
		if (leaf(node))
		{
			if (node->visits == 0) // simulate current
				retro_propagate(node, simulate(board, state, max_moves, decay, opponent));
			else
			{
				// expand node
				MCTS_NODE* c1 = NULL;
				int nb_moves = available_moves(board, state, node->depth % 2 ? 'H' : 'V', max_moves, decay, opponent, &zemoves[0]);
				for (int m = 0 ; m < nb_moves ; m++)
				{
					MCTS_NODE* c = new_node(node, zemoves[m].idx, board->slot[zemoves[m].idx].code);
					add_child(node, c);
					if (c1 == NULL) c1 = c;
				}
				// simulate first child
				retro_propagate(c1, simulate(board, state, max_moves, decay, opponent));
			}
			node = NULL;
		}
		else // select best child
			node = best_child_node(node, exploration);
	}
}

MCTS_NODE* mcts(BOARD* board, STATE* state, int iterations, double exploration, int max_moves, double decay, double opponent)
{
MCTS_NODE* head = new_node(NULL, -1, "");

	for (int i = 0 ; i < iterations ; i++)
		iteration(head, exploration, board, state, max_moves, decay, opponent);

	return best_child_node(head, 0.0);
}


