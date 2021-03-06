/**
 * @file
 *
 * The simple graph renderer for SimpleGit.
 +
 * (c) 2015 by Sebastian Bauer
 */

#include "graph.h"

#include <stdio.h>
#include <string.h>

#define MAX_COLUMNS 16

void graph_render(int num_nodes, graph_callbacks *callbacks)
{
	int i;
	int used_columns = 0;
	node_t assigned_to[MAX_COLUMNS] = {0};
	int aboves[MAX_COLUMNS] = {0};
	int num_aboves_previous = 0;
	struct graph_print_data pd = {0};

	for (i = 0; i < num_nodes; i++)
	{
		int j,k,l;
		node_t n;
		int to_be_removed[MAX_COLUMNS];
		int first_to_be_inserted = MAX_COLUMNS;
		int first_to_be_removed = MAX_COLUMNS;
		int collapse_goal = MAX_COLUMNS;
		int num_to_be_removed = 0;
		int num_parents;
		int col_of_n;
		cell_state_t row[MAX_COLUMNS];

		memset(row,0,sizeof(row));

		n = callbacks->get_node(i, callbacks->userdata);
		num_parents = callbacks->get_num_parents(n, callbacks->userdata);

		for (j = 0; j < MAX_COLUMNS && j < used_columns; j++)
		{
			if (assigned_to[j] == n)
				break;
		}

		/* Too many columns, abort */
		if (j == MAX_COLUMNS)
			goto bailout;

		/* Possibly assign a new column */
		if (j == used_columns)
		{
			aboves[j] = -1;
			row[j] |= NODE;
			assigned_to[used_columns++] = n;
		}

		col_of_n = j;

		/* Print current state */
		for (k = 0; k < used_columns; k++)
		{
			if (assigned_to[k] == n) row[k] |= NODE;
			else row[k] |= TOP_TO_CENTER;
			aboves[k] = k;
		}

		/* Assign parents. For now, the first parent gets the same column as the
		 * current commit. Each of the other parent gets a new column.
		 */
		for (l = 0; l < num_parents; l++)
		{
			node_t p = callbacks->get_parent(n, l, callbacks->userdata);
			if (l==0)
			{
				aboves[col_of_n] = col_of_n;
				assigned_to[j] = p;

				/* There must be something coming from if this is a merge */
				if (num_parents != 1)
					row[j] |= RIGHT_TO_CENTER;
			} else
			{
				if (used_columns >= MAX_COLUMNS)
					goto bailout;
				if (first_to_be_inserted == MAX_COLUMNS)
					first_to_be_inserted = used_columns;

				/* The cell must have an connection to the left.
				 * All but the last one must have a connection to the right.
				 */
				row[used_columns] |= LEFT_TO_CENTER;
				if (l < num_parents - 1)
					row[used_columns] |= RIGHT_TO_CENTER;

				aboves[used_columns] = col_of_n;
				assigned_to[used_columns++] = p;
			}
		}

		/* Determine duplicates, for now only an O(n^2) algo */
		for (j = 0; j < used_columns; j++)
		{
			for (k = j + 1; k < used_columns; k++)
			{
				if (assigned_to[j] == assigned_to[k])
				{
					if (collapse_goal == MAX_COLUMNS)
						collapse_goal = j;

					first_to_be_removed = k;
					to_be_removed[num_to_be_removed++] = k;

					row[j] |= RIGHT_TO_CENTER;
					row[k] |= LEFT_TO_CENTER;
				}
			}
		}

		/* Print still existing branches and merge or fork points (with respect to the history.
		 * i.e., as seen from bottom). This works only for the most simple cases.
		 */
		if (first_to_be_inserted != MAX_COLUMNS)
		{
			/* Straight lines from the beginning to the first inserted branch */
			for (k = 0; k < used_columns && k < first_to_be_inserted; k++)
				row[k] |= TOP_TO_BOTTOM;
			for (; k < used_columns; k++)
				row[k] |= LEFT_TO_CENTER|BOTTOM_TO_CENTER; /* Merge */
		} else
		{
			for (k = 0; k < TOP_TO_BOTTOM && k < first_to_be_removed; k++)
				row[k] |= TOP_TO_BOTTOM;
			for (; k < used_columns; k++) /* Fork */
				row[k] |= LEFT_TO_CENTER|TOP_TO_CENTER;
		}


		if (callbacks->new_print_row)
		{
			pd.num_aboves_previous = num_aboves_previous;
			pd.num_aboves = used_columns;
			pd.aboves = aboves;
			pd.collapse_goal = collapse_goal;
			pd.num_collapsing = num_to_be_removed;
			pd.collapsing = to_be_removed;
			pd.node = col_of_n;
			callbacks->new_print_row(n, &pd, callbacks->userdata);

			num_aboves_previous = pd.num_aboves;
		}

		callbacks->print_row(n, row, used_columns, callbacks->userdata);

		/* Then remove them, also one by one */
		for (j = 0; j < num_to_be_removed; j++)
		{
			int dest = to_be_removed[j];
			int src = dest + 1;
			memmove(&assigned_to[dest], &assigned_to[src], (used_columns - src) * sizeof(node_t));
		}
		used_columns -= num_to_be_removed;
	}
bailout:
	return;
}
