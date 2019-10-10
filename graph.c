#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "graph.h"

//struct gs_graph *ga_new_graph(

int graph_find_in_root_nodes(struct gs_graph *graph, void *p) {
	for (int i = 0; i < graph->n_root_nodes; i++) {
		if (graph->root_nodes[i] == p) {
			return i;
		}
	}
	return -1;
}

void graph_promote_to_root_node(struct gs_graph *graph, struct gs_graph_node *node) {
	graph->n_root_nodes++;
	graph->root_nodes = realloc(graph->root_nodes, graph->n_root_nodes*sizeof(graph->root_nodes[0]));
	graph->root_nodes[graph->n_root_nodes - 1] = node;
}

void graph_demote_root_node(struct gs_graph *graph, int i_root_node) {
	if (i_root_node != -1) {
		assert(i_root_node < graph->n_root_nodes);
		for (int i = i_root_node+1; i < graph->n_root_nodes; i++) {
			graph->root_nodes[i-1] = graph->root_nodes[i];
		}
		graph->n_root_nodes--;
		graph->root_nodes = realloc(graph->root_nodes, graph->n_root_nodes*sizeof(graph->root_nodes[0]));
	}
}

void gs_add_node(struct gs_graph *graph, struct gs_graph_node *node) {

	graph_promote_to_root_node(graph, node);
	graph->n_nodes++;

	for (int i = 0; i < node->n_outputs; i++) {
		struct gs_graph_node *output_node = node->outputs[i].node;
		// If any targets are root nodes, demote them:
		graph_demote_root_node(graph, graph_find_in_root_nodes(graph, output_node));

		output_node->n_inputs++;
		output_node->inputs = realloc(output_node->inputs, output_node->n_inputs*sizeof(output_node->inputs[0]));
		int input_index = output_node->n_inputs - 1;
		output_node->inputs[input_index] = node;
	}

}

void gs_connect_nodes(struct gs_graph *graph, struct gs_graph_node *source, struct gs_graph_node *target, double *val) {
	source->n_outputs++;
	source->outputs = realloc(source->outputs, source->n_outputs*sizeof(source->outputs[0]));
	source->outputs[source->n_outputs - 1] = (struct output_node_value) {.node = target, .value = val};

	target->n_inputs++;
	target->inputs = realloc(target->inputs, target->n_inputs*sizeof(target->inputs[0]));
	target->inputs[target->n_inputs - 1] = source;

	// If the target is a root node, demote it:
	graph_demote_root_node(graph, graph_find_in_root_nodes(graph, target));

}

double *gs_connect_to_arithmetic_node(struct gs_graph *graph, struct gs_graph_node *source, struct gs_graph_node *target) {
	struct gs_arithmetic_state *state = target->state;

	target->n_inputs++;
	target->inputs = realloc(target->inputs, target->n_inputs*sizeof(target->inputs[0]));
	state->inputs = realloc(state->inputs, target->n_inputs*sizeof(state->inputs[0]));
	int input_index = target->n_inputs - 1;
	target->inputs[input_index] = source;

	// If the target is a root node, demote it:
	graph_demote_root_node(graph, graph_find_in_root_nodes(graph, target));

	double *target_val = &state->inputs[input_index];

	source->n_outputs++;
	source->outputs = realloc(source->outputs, source->n_outputs*sizeof(source->outputs[0]));
	source->outputs[source->n_outputs - 1] = (struct output_node_value) {.node = target, .value = target_val};

	return target_val;
}

struct gs_graph_node *gs_new_node(struct gs_graph *graph, struct gs_graph_node node, size_t size) {
	struct gs_graph_node *node_p = malloc(sizeof(*node_p));
	*node_p = node;
	node_p->inputs = malloc(node.n_inputs * sizeof(node_p->inputs[0]));
	for (int i = 0; i < node.n_inputs; i++) {
		node_p->inputs[i] = node.inputs[i];
	}
	node_p->outputs = malloc(node.n_outputs * sizeof(node_p->outputs[0]));
	for (int i = 0; i < node.n_outputs; i++) {
		node_p->outputs[i] = node.outputs[i];
	}
	node_p->state = malloc(size);
	memcpy(node_p->state, node.state, size);

	gs_add_node(graph, node_p);

	return node_p;
}

//struct graph_node gs_sequencer(n_outputs, *outputs, sequence) {
//}
//
//struct graph_node gs_oscillator(n_outputs, *outputs, f, v, offset) {
//}

double run_node(int n_nodes, struct gs_graph_node *(node_arr[n_nodes]), int curr_node_index, int *n_active_nodes, int *node_arr_next_avail_index) {
	struct gs_graph_node *node = node_arr[curr_node_index];
	double output_val = node->func(node);

	for (int i = 0; i < node->n_outputs; i++) {
		struct output_node_value output = node->outputs[i];
		*output.value = output_val;
		output.node->n_inputs_sent++;
		if (output.node->n_inputs_sent == output.node->n_inputs) {
			if (*node_arr_next_avail_index >= n_nodes) {
				fprintf(stderr, "Overrun the array of pointers to graph nodes (trying to write to node %d in graph of %d nodes). Something's broken.\n", *node_arr_next_avail_index, n_nodes);
				abort();
			}
			node_arr[*node_arr_next_avail_index] = output.node;
			(*node_arr_next_avail_index)++;
			(*n_active_nodes)++;
			output.node->n_inputs_sent = 0;
		}
	}
	(*n_active_nodes)--;
	return output_val;
}

struct graph_node_list {
	struct gs_graph_node *node;
	struct graph_node_list *next;
};

void gs_run_graph(struct gs_graph *graph) {

	struct gs_graph_node *(node_arr[graph->n_nodes]);
	for (int i = 0; i < graph->n_nodes; i++) {
		node_arr[i] = NULL;
	};

	for (int i = 0; i < graph->n_root_nodes; i++) {
		node_arr[i] = graph->root_nodes[i];
	};
	int n_active_nodes = graph->n_root_nodes;
	int node_arr_next_avail_index = graph->n_root_nodes;
	int curr_node_index = 0;

	while(n_active_nodes > 0) {
		if (curr_node_index >= graph->n_nodes) {
			fprintf(stderr, "Overrun the array of pointers to graph nodes (trying to read %dth node in graph of %d nodes). Something's broken.\n", curr_node_index, graph->n_nodes);
			abort();
		}
		if (!node_arr[curr_node_index]) {
			fprintf(stderr, "There's a NULL graph node in this list. Something's broken.\n");

			abort();
		}
		double return_val = run_node(graph->n_nodes, node_arr, curr_node_index, &n_active_nodes, &node_arr_next_avail_index);
		curr_node_index++;
	}
	if (curr_node_index != graph->n_nodes) {
		fprintf(stderr, "Missed some nodes somehow (finished after reading %d nodes in graph of %d nodes). Something's broken.\n", curr_node_index, graph->n_nodes);		abort();
	}

}

void print_graph_node_recursive(struct gs_graph_node *node) {
	const char *name = (node->name)? node->name : "(null)";
	fprintf(stderr,"Node: %p,	Name=%s,	Outputs={", node, name);
	for (int i = 0; i < node->n_outputs; i++) {
		fprintf(stderr,"%p, ", node->outputs[i].node);
	}
	fprintf(stderr,"}.\n\n");

	for (int i = 0; i < node->n_outputs; i++) {
		print_graph_node_recursive(node->outputs[i].node);
	}
}

void gs_print_graph(struct gs_graph *graph) {

	for (int i = 0; i < graph->n_root_nodes; i++) {
		print_graph_node_recursive(graph->root_nodes[i]);
	}
}

double gs_zero(struct gs_graph_node *n) {
	return 1000;
}
