
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

typedef struct node_mcts
{
	char	move[4];
	double	value;
	int	msid, visits, children;
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


MCTS_NODE* new_node(MCTS_NODE*parent, const char *move, int msid)
{
	MCTS_NODE* nn = malloc(sizeof(MCTS_NODE));
	nn->parent = parent;
	strcpy(nn->move, move);
	nn->msid = msid;
	nn->value = 0.0;
	nn->visits = 0;
	nn->children = 0;
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
	return 0.23;
}

int available_moves(BOARD* board, STATE* state, int max_moves, double decay, double opponent)
{
TRACK zemoves[1024];

	//eval_orientation(board, state, orientation, lambda_decay, wpegs, wlinks, wzeta, true);
	/*int nb_moves = state_moves(board, state, orientation, opponent_decay, &zemoves[0]);
        while (sid < 0)
        {
            sid = zemoves[rand() % max_moves].idx;
            if (find_move(state, sid)) sid = -1;
        }*/
        
	return 0;
}

void iteration(MCTS_NODE* head, double exploration, BOARD* board, STATE* state, int max_moves, double decay, double opponent)
{
MCTS_NODE* node = head;
MOVE	moves[32];

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
				int nb_moves = available_moves(board, state, max_moves, decay, opponent);
				for (int m = 0 ; m< nb_moves ; m++)
				{
					MCTS_NODE* c = new_node(node, "ZZ", 999);
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

void mcts(BOARD* board, STATE* state, int iterations, double exploration, int max_moves, double decay, double opponent)
{
MCTS_NODE* head = new_node(NULL, "", -1);

	for (int i = 0 ; i < iterations ; i++)
		iteration(head, exploration, board, state, max_moves, decay, opponent);

MCTS_NODE* bcn = best_child_node(head, 0.0);
}


