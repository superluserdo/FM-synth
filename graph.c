#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "graph.h"

//struct gs_graph *ga_new_graph(

int graph_find_in_root_nodes(struct gs_graph *graph, void *p) {
	int index = -1;
	for (int i = 0; i < graph->n_root_nodes; i++) {
		if graph->root_nodes[i]

void graph_add_root_node(struct gs_graph *graph, struct gs_graph_node *node) {
	graph->n_root_nodes++;
	realloc(graph->root_nodes, graph->n_root_nodes*sizeof(graph->n_root_nodes[0]));
	graph->root_nodes[graph->n_root_nodes - 1] = node;
}

void graph_rm_root_node(struct gs_graph *graph, int i_root_node) {
	assert(i_root_node < graph->n_root_nodes);
	for (int i = i_root_node+1; i < graph->n_root_nodes; i++) {
		graph->root_nodes[i-1] = graph->root_nodes[i];
	}
	graph->n_root_nodes--
	realloc(graph->root_nodes, graph->n_root_nodes*sizeof(graph->n_root_nodes[0]));
}

struct gs_graph_node gs_new_node(struct gs_graph *graph, const char *name, int n_outputs, struct output_node_value outputs[n_outputs], void *state, double (*func)(void *)) {
	struct gs_graph_node node = {
		.name = name,
		.n_outputs = n_outputs,
		.outputs = outputs,//calloc(n_outputs, sizeof(struct output_node_value))
		.state = state,
		.func = func,
	};
	for (int i = 0; i < n_outputs; i++) {
		//node.outputs[i] = outputs[i];
		int i_root_node = graph_find_in_root_nodes(graph, node.outputs[i].node;
		if (i_root_node != -1) {
			graph_rm_root_node(graph, i_root_node);
		}

		node.outputs[i].node->n_inputs_total++;
	}
	graph_add_root_node(graph, node);

	return node;
}

//struct graph_node gs_sequencer(n_outputs, *outputs, sequence) {
//}
//
//struct graph_node gs_oscillator(n_outputs, *outputs, f, v, offset) {
//}

void run_node(int n_nodes, struct gs_graph_node *(node_arr[n_nodes]), int curr_node_index, int *n_active_nodes, int *node_arr_next_avail_index) {
	struct gs_graph_node node = *node_arr[curr_node_index];
	double output_val = node.func(node.state);

	for (int i = 0; i < node.n_outputs; i++) {
		struct output_node_value output = node.outputs[i];
		*output.value = output_val;
		output.node->n_inputs_sent++;
		if (output.node->n_inputs_sent == output.node->n_inputs_total) {
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
}

struct graph_node_list {
	struct gs_graph_node *node;
	struct graph_node_list *next;
};

void gs_run_graph(struct gs_graph graph) {

	struct gs_graph_node *(node_arr[graph.n_nodes]);
	for (int i = 0; i < graph.n_nodes; i++) {
		node_arr[i] = NULL;
	};

	for (int i = 0; i < graph.n_root_nodes; i++) {
		node_arr[i] = graph.root_nodes[i];
	};
	int n_active_nodes = graph.n_root_nodes;
	int node_arr_next_avail_index = graph.n_root_nodes;
	int curr_node_index = 0;

	while(n_active_nodes > 0) {
		if (curr_node_index >= graph.n_nodes) {
			fprintf(stderr, "Overrun the array of pointers to graph nodes (trying to read %dth node in graph of %d nodes). Something's broken.\n", curr_node_index, graph.n_nodes);
			abort();
		}
		if (!node_arr[curr_node_index]) {
			fprintf(stderr, "There's a NULL graph node in this list. Something's broken.\n");

			abort();
		}
		run_node(graph.n_nodes, node_arr, curr_node_index, &n_active_nodes, &node_arr_next_avail_index);
		curr_node_index++;
	}
	if (curr_node_index != graph.n_nodes) {
		fprintf(stderr, "Missed some nodes somehow (finished after reading %d nodes in graph of %d nodes). Something's broken.\n", curr_node_index, graph.n_nodes);		abort();
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

void gs_print_graph(struct gs_graph graph) {

	for (int i = 0; i < graph.n_root_nodes; i++) {
		print_graph_node_recursive(graph.root_nodes[i]);
	}
}
