#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#define PI 3.1415926
#include "graph.h"
#include "osc.h"

//TODO: Remove
int sample_rate = 44100;
double v = 1;
long counter_global = 0;
//------

//struct gs_graph_node gs_new_oscillator(int n_outputs, struct output_node_value outputs[n_outputs], struct gs_oscillator_state *state, double (*func)(void *)) {
//
//	struct gs_graph_node node = gs_new_node(n_outputs, outputs);
//	//node.state = calloc(1, sizeof(struct gs_oscillator_state));
//	//*(struct gs_oscillator_state *)node.state = state;
//	node.state = state;
//	node.func = func;
//	return node;
//};

int time_to_samples(int sample_rate, double t) {
	return sample_rate * t;
}

int cycle_length(int sample_rate, double f) {
	return sample_rate/f;
}

double osc_run(struct oscillator_old *osc) {
	double f_new = osc->f;
	if (osc->input_osc) {
		f_new += osc_run(osc->input_osc);
	}
	double val = osc->func(f_new, osc->v, osc->theta, osc->offset);
	osc->theta = fmod(osc->theta + 2*PI/cycle_length(sample_rate, f_new),2*PI);
	return val;
}

int square(double f, double v) {
	int l = cycle_length(sample_rate, f);
	if (counter_global%l < l/2)
		return INT_MIN*v;
	else
		return INT_MAX*v;
}

//double sinew(double f, double v, double theta, double offset) {
//void sinew(struct gs_graph_node *node) {
//	struct gs_oscillator_state *state = node->state;
//	//int l = cycle_length(sample_rate, state->f);
//	double val = sin(state->theta)*state->v;
//}

void gs_osc_sin(struct gs_graph_node *node) {
	struct gs_oscillator_state *state = node->state;
	//int l = cycle_length(sample_rate, state->f);
	double val = sin(state->theta)*state->v;
	state->theta = fmod(state->theta + 2*PI/cycle_length(sample_rate, state->f),2*PI);
	for (int i = 0; i < node->n_outputs; i++) {
		*(double *)node->outputs[i].value = val;
	}
}

void gs_fmul(struct gs_graph_node *node) {
	struct gs_arithmetic_state *state = node->state;
	double val = state->input_default;
	for (int i = 0; i < node->n_inputs; i++) {
		if (node->inputs[i]) {
			val *= state->inputs[i];
		}
	}
	//return result;
	for (int i = 0; i < node->n_outputs; i++) {
		*(double *)node->outputs[i].value = val;
	}
}

void gs_fadd(struct gs_graph_node *node) {
	struct gs_arithmetic_state *state = node->state;
	double val = state->input_default;
	for (int i = 0; i < node->n_inputs; i++) {
		if (node->inputs[i]) {
			val += state->inputs[i];
		}
	}
	//return result;
	for (int i = 0; i < node->n_outputs; i++) {
		*(double *)node->outputs[i].value = val;
	}
}

//double *gs_arithmetic_get_avail_slot(struct gs_arithmetic_state *state) {
//	int found_slot = -1;
//	for (int i = 0; i < node->n_inputs; i++) {
//		if (!state->inputs[i]) {
//			found_slot = i;
//			state->inputs[i] = true;
//			break;
//		}
//	}
//	if (found_slot == -1) {
//		fprintf(stderr, "Couldn't get input slot for arithmetic node :(\n");
//		abort();
//	}
//	return &state->inputs[found_slot];
//}

//double *gs_arithmetic_get_new_slot(struct gs_arithmetic_state *state) {
//	int found_slot = -1;
//	for (int i = 0; i < node->n_inputs; i++) {
//		if (!state->inputs[i]) {
//			found_slot = i;
//			state->inputs[i] = true;
//			break;
//		}
//	}
//	if (found_slot == -1) {
//		fprintf(stderr, "Couldn't get input slot for arithmetic node :(\n");
//		abort();
//	}
//	return &state->inputs[found_slot];
//}


int f_to_i(double num) {
	return INT_MAX*num;
}

void gs_f_to_i_state(struct gs_graph_node *node) {
	struct gs_audio_out_state *state = node->state;
	state->i = INT_MAX * state->d;
}

void writeint(int num) {
	fwrite(&(int){num}, sizeof(int), 1, stdout);
}

double key_to_f(int key[2]) {
	//return pitches[key];
	int A440 = 49; //49th key on keyboard
	int exp = key[0] + key[1]*12;
	float diff = exp - A440;
	return 440 * pow(2, diff/12);
}

void f_init(struct oscillator_old *osc, double f) {
	do {
		osc->f = f * osc->f_mult;
		osc = osc->input_osc;
	} while (osc);
}

double sigmoid(double x) {
	return 1 / (1 + exp (-x));
}

int osc_play_seq(struct voice *voice) {
	if (!voice->playing) {
		return 0;
	}
	if (voice->sample_index == 0) {
		double pitch = key_to_f(voice->seq[voice->seq_index].key);
		f_init(voice->osc, pitch);
		voice->samples_env_default = 800;
		voice->samples_on = time_to_samples(sample_rate, voice->seq[voice->seq_index].duration*voice->step_len*voice->seq[voice->seq_index].frac_held);
		voice->samples_off = time_to_samples(sample_rate, voice->seq[voice->seq_index].duration*voice->step_len*(1-voice->seq[voice->seq_index].frac_held));
		//voice->samples_atk = (samples_on > samples_env_default)? samples_env_default : samples_on/4;
		//voice->samples_dec = (samples_off > samples_env_default)? samples_env_default : samples_off/4;
		voice->samples_atk = voice->samples_env_default;
		voice->samples_dec = voice->samples_env_default;
		voice->samples_sus = voice->samples_on - voice->samples_atk - voice->samples_dec/2;
		voice->samples_empty = voice->samples_off - voice->samples_dec/2;

		//Sigmoid envelope -- get -6 to +6 in x over sample range
		voice->x_atk_range= 12.0;
		voice->sample_x_atk_mult = voice->x_atk_range/voice->samples_atk;
		voice->x_dec_range= 12.0;
		voice->sample_x_dec_mult = voice->x_dec_range/voice->samples_atk;
	}

	int out_sample;
	int sample_index_sus = voice->sample_index - voice->samples_atk;
	int sample_index_dec = voice->sample_index - voice->samples_atk - voice->samples_sus;
	int sample_index_empty = voice->sample_index - voice->samples_atk - voice->samples_sus - voice->samples_dec;

	//for (int sample = 0; sample < samples_atk; sample++) {
	if (voice->sample_index < voice->samples_atk) {
		float atk_mult = sigmoid(-6 + voice->sample_index*voice->sample_x_atk_mult);
		//writeint(atk_mult*f_to_i(osc_run(voice->osc)));
		out_sample = atk_mult*f_to_i(osc_run(voice->osc));
	//}
	} else if (sample_index_sus < voice->samples_sus) {
	//for (int sample = 0; sample < samples_sus; sample++) {
		//writeint(f_to_i(osc_run(voice->osc)));
		out_sample = f_to_i(osc_run(voice->osc));
	//}
	} else if (sample_index_dec < voice->samples_dec) {
	//for (int sample = 0; sample < samples_dec; sample++) {
		float dec_mult = 1 - sigmoid(-6 + sample_index_dec*voice->sample_x_dec_mult);
		//writeint(dec_mult*f_to_i(osc_run(voice->osc)));
		out_sample = dec_mult*f_to_i(osc_run(voice->osc));
	//}
	} else if (sample_index_empty < voice->samples_empty) {
	//for (int sample = 0; sample < samples_empty; sample++) {
		//writeint(0);
		out_sample = 0;
	}
	voice->sample_index++;
	if (voice->sample_index >= voice->samples_on + voice->samples_off) {
		voice->sample_index = 0;
		voice->seq_index++;
	}
	if (voice->seq_index >= voice->seq_len) {
		voice->playing = 0;
	}
	return out_sample;
}
void osc_play_seq_non_reentrant(struct oscillator_old *osc, int seq_len, struct seq_note seq[], double step_len) {
	for (int i = 0; i < seq_len; i++) {
		double pitch = key_to_f(seq[i].key);
		f_init(osc, pitch);
		int samples_env_default = 800;
		int samples_on = time_to_samples(sample_rate, seq[i].duration*step_len*seq[i].frac_held);
		int samples_off = time_to_samples(sample_rate, seq[i].duration*step_len*(1-seq[i].frac_held));
 		//int samples_atk = (samples_on > samples_env_default)? samples_env_default : samples_on/4;
		//int samples_dec = (samples_off > samples_env_default)? samples_env_default : samples_off/4;
 		int samples_atk = samples_env_default;
		int samples_dec = samples_env_default;
		int samples_sus = samples_on - samples_atk - samples_dec/2;
		int samples_empty = samples_off - samples_dec/2;

		//Sigmoid envelope -- get -6 to +6 in x over sample range
		double x_atk_range= 12.0;
		double sample_x_atk_mult = x_atk_range/samples_atk;
		for (int sample = 0; sample < samples_atk; sample++) {
			float atk_mult = sigmoid(-6 + sample*sample_x_atk_mult);
			writeint(atk_mult*f_to_i(osc_run(osc)));
		}
		for (int sample = 0; sample < samples_sus; sample++) {
			writeint(f_to_i(osc_run(osc)));
		}
		double x_dec_range= 12.0;
		double sample_x_dec_mult = x_dec_range/samples_atk;
		for (int sample = 0; sample < samples_dec; sample++) {
			float dec_mult = 1 - sigmoid(-6 + sample*sample_x_dec_mult);
			writeint(dec_mult*f_to_i(osc_run(osc)));
		}
		for (int sample = 0; sample < samples_empty; sample++) {
			writeint(0);
		}
	}
}

struct oscillator_old *osc_deepcopy(struct oscillator_old *osc_orig) {
	struct oscillator_old *osc_new_prev = NULL;
	struct oscillator_old *osc_new_carrier = NULL;
	while (osc_orig) {
		struct oscillator_old *osc_new = malloc(sizeof *osc_orig);
		*osc_new = *osc_orig;
		/* If carrier osc, it is not the input to any other osc */
		if (osc_new_prev) {
			osc_new_prev->input_osc = osc_new;
		/* Make note that this is the carrier */
		} else {
			osc_new_carrier = osc_new;
		}
		osc_orig = osc_orig->input_osc;
		osc_new_prev = osc_new;
	}
	return osc_new_carrier;
}

struct gs_graph_node *gs_new_voice(struct gs_graph *graph, struct gs_graph_node *sequencer, struct gs_graph_node *adsr_controller, struct gs_graph_node node, size_t size) {
	struct gs_graph_node *node_voice = gs_new_node(graph, node, size);
	//struct gs_graph_node *carrier_f = gs_
	struct gs_graph_node *voice_envelope = gs_new_node(graph, 
			(struct gs_graph_node) {
				.name = "envelope", 
				.state = &(struct gs_arithmetic_state) {.input_default = 1},
				.func = gs_fmul
			}, sizeof(struct gs_arithmetic_state));
	gs_connect_to_arithmetic_node(graph, voice_envelope, graph->mixer);
	gs_connect_to_arithmetic_node(graph, node_voice, voice_envelope);

	//gs_connect_nodes(graph, adsr_controller, node_voice, &((struct gs_oscillator_state *)node_voice->state)->f);
	gs_connect_to_arithmetic_node(graph, adsr_controller, voice_envelope);
	gs_connect_nodes(graph, sequencer, adsr_controller, &((struct gs_adsr_state *)adsr_controller->state)->f_samples);
	gs_connect_nodes(graph, sequencer, node_voice, &((struct gs_oscillator_state *)node_voice->state)->f);
	return node_voice;
}

//void gs_connect_seq_osc(struct gs_graph_node *sequencer, struct gs_graph_node *adsr_controller, struct gs_graph_node *osc) {
//	gs_connect_nodes(graph, adsr_controller, osc, ((struct gs_oscillator_state *)osc->state)->f);
//	gs_connect_nodes(graph, sequencer, adsr_controller, ((struct gs_adsr_state *)adsr_controller->state)->f_samples);
//}
//
//void gs_connect_adsr_voice(struct gs_graph_node *adsr_controller, struct gs_graph_node *voice) {
//	struct gs_graph_node *voice_envelope = gs_new_node(graph, 
//			(struct gs_graph_node) {
//				.name = "envelope", 
//				.n_outputs = 1, 
//				.outputs = (struct output_node_value []){voice}, 
//				.state = &(struct gs_arithmetic_state) {.input_default = ((struct gs_oscillator_state *)target->state)->f},
//				.func = gs_fmul
//			}, sizeof(struct gs_arithmetic_state));
//	gs_connect_nodes(graph, adsr_controller, osc, ((struct gs_oscillator_state *)osc->state)->f);
//	gs_connect_nodes(graph, sequencer, adsr_controller, ((struct gs_adsr_state *)adsr_controller->state)->f_samples);
//}

struct gs_graph_node *gs_osc_seq_freqmod(struct gs_graph *graph, struct gs_graph_node *sequencer, struct gs_graph_node *target, double modf_multiple, double modv, int mul) {

	void (*func)(struct gs_graph_node *) = mul ? gs_fmul : gs_fadd;

	struct gs_graph_node *node_f_add = gs_new_node(graph, 
			(struct gs_graph_node) {
				.name = "f_add", 
				//.n_outputs = 1, 
				//.outputs = (struct output_node_value []){{.node=target, 
				//		.value=&((struct gs_oscillator_state *)target->state)->f}}, 
				.state = &(struct gs_arithmetic_state) {0},//.input_default = ((struct gs_oscillator_state *)target->state)->f},
				.func = func
			}, sizeof(struct gs_arithmetic_state));

	graph_insert_arithmetic_input(graph, node_f_add, target, &((struct gs_oscillator_state *)target->state)->f);

	struct gs_graph_node *node_fmod = gs_new_node(graph, 
			(struct gs_graph_node) {
				.name = "f_mod", 
				.state = &(struct gs_oscillator_state) {.v = modv}, 
				.func = gs_osc_sin
			}, sizeof(struct gs_arithmetic_state));
	gs_connect_to_arithmetic_node(graph, node_fmod, node_f_add);

	struct gs_graph_node *node_fmod_mul = gs_new_node(graph, 
			(struct gs_graph_node) {
				.name = "f_mod_mul", 
				.n_outputs = 1, 
				.outputs = (struct output_node_value []){
						{.node=node_fmod, .value=&((struct gs_oscillator_state *)node_fmod->state)->f},
				}, 
				.state = &(struct gs_arithmetic_state) {.input_default = modf_multiple},
				.func = gs_fmul
			}, sizeof(struct gs_arithmetic_state));

	gs_connect_to_arithmetic_node(graph, sequencer, node_fmod_mul);

	return node_fmod;
}

void gs_seq_func(struct gs_graph_node *node) {
	struct gs_sequencer_state *state = node->state;
	struct f_samples *f_samples = &state->f_samples;
	struct gs_sequence sequence = state->sequence;

	struct seq_note curr_note = sequence.notes[state->curr_note];
	double note_beat_diff = state->curr_note_beat_progress - curr_note.duration;

	f_samples->f = key_to_f(curr_note.key);
	for (int i = 0; i < node->n_outputs; i++) {
		// Hack :(
		if (node->outputs[i].node->func == gs_control_adsr) {
			*(struct f_samples *)node->outputs[i].value = *f_samples;
		} else {
			*(double *)node->outputs[i].value = f_samples->f;
		}
	}
	state->curr_sample++;
	f_samples->samples++;

	double beat_diff = 1/(double)(state->samples_per_beat);

	state->curr_beat += beat_diff;
	state->curr_note_beat_progress += beat_diff;

	if (note_beat_diff >= 0) {
		state->curr_note++;
		state->curr_note_beat_progress = note_beat_diff;
		f_samples->samples = 0;
	}

	if (state->curr_note >= sequence.seq_len) {
		node->hangup = 1;
	}

}


double gs_atk_func_linear(int samples, int atk_samples) {
	return (double)samples/atk_samples;
}

double gs_dec_func_linear(int samples, int dec_samples, double sus_val) {
	return 1 - (1 - sus_val) * (double)samples/dec_samples;
}

void gs_control_adsr(struct gs_graph_node *node) {
	static int test_counter = 0;
	double result = 1;
	struct gs_adsr_state *state = node->state;
	struct gs_adsr adsr = state->adsr;

	int n_samples = state->f_samples.samples;
	if (state->f_samples.f > 0) {
		/* Note is held */
		if (n_samples > (adsr.atk_samples + adsr.dec_samples)) {
			result = adsr.sus_val;
		} else if (n_samples > adsr.atk_samples) {
			if (adsr.dec_func) {
				result = adsr.dec_func(n_samples - adsr.atk_samples, adsr.dec_samples, adsr.sus_val);
			} else {
				result = 1;
			}
		} else {
			if (adsr.atk_func) {
				result = adsr.atk_func(n_samples, adsr.atk_samples);
			} else {
				result = 1;
			}
		}
	} else {
		/* Note is off */
		if (n_samples > adsr.rel_samples) {
			result = 0;
		} else {
			if (adsr.rel_func) {
				result = adsr.rel_func(n_samples, adsr.rel_samples, adsr.sus_val);
			} else {
				result = 0;
			}
		}
	}

	
	for (int i = 0; i < node->n_outputs; i++) {
		*(double *)node->outputs[i].value = result;
	}
	//fprintf(stderr, "%f\n", result);
	//if (test_counter > 10) {
	//	exit(0);
	//}
	test_counter++;
}

