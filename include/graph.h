#ifndef _GRAPH_H
#define _GRAPH_H

struct output_node_value {
	struct gs_graph_node *node;
	double *value;
};

struct gs_graph {
	int n_nodes;
	int n_root_nodes;
	struct gs_graph_node **root_nodes;
};

struct gs_graph_node {
	const char *name;
	int n_inputs_total;
	int n_inputs_sent;
	int n_outputs;
	struct output_node_value *outputs;
	void *state;
	double (*func)(void *state);
};

//struct gs_graph_node gs_new_node(int n_outputs, struct output_node_value outputs[n_outputs]);
struct gs_graph_node gs_new_node(struct gs_graph *graph, const char *name, int n_outputs, struct output_node_value outputs[n_outputs], void *state, double (*func)(void *));
void gs_print_graph(struct gs_graph graph);
void gs_run_graph(struct gs_graph graph);
#endif
