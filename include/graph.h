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
	int n_inputs_sent;
	int n_inputs;
	struct gs_graph_node **inputs;
	int n_outputs;
	struct output_node_value *outputs;
	void *state;
	double (*func)(struct gs_graph_node *);
};

struct gs_arithmetic_state {
	//int n_inputs_max;
	//bool *inputs_used;
	//union i_d *inputs;
	double input_default;
	double *inputs;
};

//struct gs_graph_node gs_new_node(int n_outputs, struct output_node_value outputs[n_outputs]);
struct gs_graph_node *gs_new_node(struct gs_graph *graph, struct gs_graph_node node, size_t size);
void gs_add_node(struct gs_graph *graph, struct gs_graph_node *node);
void gs_connect_nodes(struct gs_graph *graph, struct gs_graph_node *source, struct gs_graph_node *target, double *val);
double *gs_connect_to_arithmetic_node(struct gs_graph *graph, struct gs_graph_node *source, struct gs_graph_node *target);
void gs_print_graph(struct gs_graph *graph);
void gs_run_graph(struct gs_graph *graph);
double gs_zero(struct gs_graph_node *);
#endif
