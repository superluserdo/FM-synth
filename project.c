#include <stdlib.h>
#include <stddef.h>
#include "project.h"
#include "graph.h"
#include "osc.h"

struct gs_graph *gs_project_init(struct gs_project *project) {
	//struct gs_graph *graph = malloc(sizeof(*graph));
	//*graph = (struct gs_graph) {0};

	struct gs_graph *graph = &project->graph;

	struct gs_graph_node *audio_out = gs_new_node(graph, 
			(struct gs_graph_node) {
				.name = "audio_out",
				.state = &(struct gs_audio_out_state) {.i = 0, .d = 0},
				.func = gs_f_to_i_state
			}, sizeof(struct gs_audio_out_state));
	project->audio_out_i = &((struct gs_audio_out_state *) audio_out->state)->i;

	struct gs_graph_node *master_channel = gs_new_node(graph, 
			(struct gs_graph_node) {
				.name = "master_channel", 
				.n_outputs = 1,
				.outputs = (struct output_node_value []){{.node=audio_out, 
						.value=&((struct gs_audio_out_state *)audio_out->state)->d}},
				.state = &(struct gs_arithmetic_state) {.input_default = 1},
				.func = gs_fmul
			}, sizeof(struct gs_arithmetic_state));
	double *master_volume = &((struct gs_arithmetic_state *)master_channel->state)->input_default;
	project->master_volume = master_volume;

	//double *master_volume = &((struct gs_arithmetic_state *) master_channel->state)->inputs[0];

	struct gs_graph_node *mixer = gs_new_node(graph, 
			(struct gs_graph_node) {
				.name = "mixer", 
				.state = &(struct gs_arithmetic_state) {.input_default = 0},
				.func = gs_fadd
			}, sizeof(struct gs_arithmetic_state));
	gs_connect_to_arithmetic_node(graph, mixer, master_channel);

	graph->mixer = mixer;

	return graph;
}
