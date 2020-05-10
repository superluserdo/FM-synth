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

void gs_connect_nodes(struct gs_graph *graph, struct gs_graph_node *source, struct gs_graph_node *target, void *val) {
	source->n_outputs++;
	source->outputs = realloc(source->outputs, source->n_outputs*sizeof(source->outputs[0]));
	source->outputs[source->n_outputs - 1] = (struct output_node_value) {.node = target, .value = val};

	target->n_inputs++;
	target->inputs = realloc(target->inputs, target->n_inputs*sizeof(target->inputs[0]));
	target->inputs[target->n_inputs - 1] = source;

	// If the target is a root node, demote it:
	graph_demote_root_node(graph, graph_find_in_root_nodes(graph, target));

}

int gs_disconnect_nodes(struct gs_graph *graph, struct gs_graph_node *source, struct gs_graph_node *target, void *val) {
	int found_outputs = 0;
	for (int i = 0; i < source->n_outputs; i++) {
		struct output_node_value *source_output = &source->outputs[i];
		if (source_output->node == target) {
			if (source_output->value == val || val == NULL) {
				found_outputs++;
				continue;
			}
		}
		if (found_outputs) {
			source->outputs[i-found_outputs] = source->outputs[i];
		}
	}

	if (!found_outputs) {
		return 0;
	}
	
	source->n_outputs -= found_outputs;
	source->outputs = realloc(source->outputs, source->n_outputs*sizeof(source->outputs[0]));

	int found_inputs = 0;
	for (int i = 0; i < target->n_inputs; i++) {
		struct gs_graph_node *target_input = target->inputs[i];
		if (target_input == source) {
			found_inputs++;
			continue;
		}
		if (found_inputs) {
			target->inputs[i-found_inputs] = target->inputs[i];
		}
	}
	target->n_inputs -= found_inputs;
	target->inputs = realloc(target->inputs, target->n_inputs*sizeof(target->inputs[0]));

	// Promote target to root node:
	if (target->n_inputs == 0) {
		graph_promote_to_root_node(graph, target);
	}

	return 1;

}

void remove_from_list(int *n_items, void ***list, void *item, size_t size) {
	int found_items = 0;
	for (int i = 0; i < *n_items; i++) {
		if (list[i] == item) {
			found_items++;
			continue;
		}
		if (found_items) {
			list[i-found_items] = list[i];
		}
	}
	*n_items -= found_items;
	*list = realloc(*list, (size_t)n_items*size);
}

void remove_from_outputs_list(int *n_outputs, struct output_node_value **list, struct gs_graph_node *node, size_t size) {
	int found_outputs = 0;
	for (int i = 0; i < *n_outputs; i++) {
		if (list[i]->node == node) {
			found_outputs++;
			continue;
		}
		if (found_outputs) {
			list[i-found_outputs] = list[i];
		}
	}
	*n_outputs -= found_outputs;
	*list = realloc(*list, (*n_outputs)*size);
}

void remove_from_inputs_list(int *n_inputs, struct gs_graph_node ***list, struct gs_graph_node *node, size_t size) {
	int found_inputs = 0;
	for (int i = 0; i < *n_inputs; i++) {
		if ((*list)[i] == node) {
			found_inputs++;
			continue;
		}
		if (found_inputs) {
			(*list)[i-found_inputs] = (*list)[i];
		}
	}
	*n_inputs -= found_inputs;
	*list = realloc(*list, (*n_inputs)*size);
}


void graph_remove_node(struct gs_graph *graph, struct gs_graph_node *node) {
	graph_demote_root_node(graph, graph_find_in_root_nodes(graph, node));

	int n_inputs = node->n_inputs;
	for (int i = 0; i < n_inputs; i++) {
		//gs_disconnect_nodes(graph, node, node->inputs[0], NULL);
		struct gs_graph_node *input_node = node->inputs[i];
		remove_from_outputs_list(&input_node->n_outputs, &input_node->outputs, node, sizeof(input_node->outputs[0]));
	}
	int n_outputs = node->n_outputs;
	for (int i = 0; i < n_outputs; i++) {
		struct gs_graph_node *output_node = node->outputs[i].node;
		//gs_disconnect_nodes(graph, node, output_node, NULL);
		remove_from_inputs_list(&output_node->n_inputs, &output_node->inputs, node, sizeof(output_node->inputs[0]));
		if (output_node->n_inputs_sent > 0) {
			output_node->n_inputs_sent--;
		}
		if (output_node->n_inputs == 0) {
			if(!output_node->persist_on_hangup) {
				output_node->hangup = 1;
			//graph_remove_node(graph, output_node);
			}
			graph_promote_to_root_node(graph, output_node);
		}
	}
	graph->n_nodes--;
	free(node->state);
	free(node);
}

void graph_insert_arithmetic_input(struct gs_graph *graph, struct gs_graph_node *source, struct gs_graph_node *target, void *val) {
	//void *val = NULL;
	for (int i = 0; i < target->n_inputs; i++) {
		struct gs_graph_node *input = target->inputs[i];
		if(gs_disconnect_nodes(graph, input, target, val)) {
			gs_connect_to_arithmetic_node(graph, input, source);
		}
	}
	gs_connect_nodes(graph, source, target, val);
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


void run_node(struct gs_graph *graph, int n_nodes, struct gs_graph_node *(node_arr[n_nodes]), int curr_node_index, int *n_active_nodes, int *node_arr_next_avail_index) {
	struct gs_graph_node *node = node_arr[curr_node_index];
	//double output_val = node->func(node);

	for (int i = 0; i < node->n_outputs; i++) {
		struct output_node_value output = node->outputs[i];
		//*output.value = output_val;
		output.node->n_inputs_sent++;
		if (output.node->n_inputs_sent >= output.node->n_inputs) {
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

	if (node->hangup) {
		graph_remove_node(graph, node);
	} else if (node->func) {
		node->func(node);
		if (node->hangup) {
			graph_remove_node(graph, node);
		}
	}
}

struct graph_node_list {
	struct gs_graph_node *node;
	struct graph_node_list *next;
};

void gs_run_graph(struct gs_graph *graph) {
	graph->n_nodes_static = graph->n_nodes;

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
		if (curr_node_index >= graph->n_nodes_static) {
			fprintf(stderr, "Overrun the array of pointers to graph nodes (trying to read %dth node in graph of %d nodes). Something's broken.\n", curr_node_index, graph->n_nodes_static);
			abort();
		}
		if (!node_arr[curr_node_index]) {
			fprintf(stderr, "There's a NULL graph node in this list. Something's broken.\n");

			abort();
		}
		run_node(graph, graph->n_nodes_static, node_arr, curr_node_index, &n_active_nodes, &node_arr_next_avail_index);
		curr_node_index++;
	}
	if (curr_node_index != graph->n_nodes_static) {
		fprintf(stderr, "Missed some nodes somehow (finished after reading %d nodes in graph of %d nodes). Something's broken.\n", curr_node_index, graph->n_nodes);		
		abort();
	}

}

void print_graph_node_recursive(struct gs_graph_node *node) {
	const char *name = (node->name)? node->name : "(null)";
	fprintf(stderr,"Node: %p,	Name=%s,	Outputs={", node, name);
	for (int i = 0; i < node->n_outputs; i++) {
		fprintf(stderr,"%p ", node->outputs[i].node);
		fprintf(stderr,"(%s), ", node->outputs[i].node->name);
	}
	fprintf(stderr,"}.\n\n");

	//for (int i = 0; i < node->n_outputs; i++) {
	//	print_graph_node_recursive(node->outputs[i].node);
	//}
}

void print_graph_node(int n_nodes, struct gs_graph_node *(node_arr[n_nodes]), int curr_node_index, int *n_active_nodes, int *node_arr_next_avail_index) {
	struct gs_graph_node *node = node_arr[curr_node_index];

	for (int i = 0; i < node->n_outputs; i++) {
		struct output_node_value output = node->outputs[i];
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
	
	const char *name = (node->name)? node->name : "(null)";
	fprintf(stderr,"Node: %p,	Name=%s,	Outputs={", node, name);
	for (int i = 0; i < node->n_outputs; i++) {
		fprintf(stderr,"%p ", node->outputs[i].node);
		fprintf(stderr,"(%s), ", node->outputs[i].node->name);
	}
	fprintf(stderr,"}.\n\n");

}

void gs_print_graph(struct gs_graph *graph) {

	fprintf(stderr, "%d nodes, %d root nodes\n", graph->n_nodes, graph->n_root_nodes);
	//for (int i = 0; i < graph->n_root_nodes; i++) {
	//	print_graph_node_recursive(graph->root_nodes[i]);
	//}
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
		print_graph_node(graph->n_nodes, node_arr, curr_node_index, &n_active_nodes, &node_arr_next_avail_index);
		curr_node_index++;
	}
	if (curr_node_index != graph->n_nodes) {
		fprintf(stderr, "Missed some nodes somehow (finished after reading %d nodes in graph of %d nodes). Something's broken.\n", curr_node_index, graph->n_nodes);		
		abort();
	}

}

void gs_zero(struct gs_graph_node *node) {
	for (int i = 0; i < node->n_outputs; i++) {
		*(double *)node->outputs[i].value = 0;
	}
}

