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
double sinew(void *v) {
	struct gs_oscillator_state *state = v;
	//int l = cycle_length(sample_rate, state->f);
	return sin(state->theta)*state->v;
}

double gs_osc_sin(void *v) {
	struct gs_oscillator_state *state = v;
	//int l = cycle_length(sample_rate, state->f);
	double val = sin(state->theta)*state->v;
	state->theta = fmod(state->theta + 2*PI/cycle_length(sample_rate, state->f),2*PI);
	return val;
}

double gs_fmul(void *v) {
	struct gs_arithmetic_state *state = v;
	double result = 1;
	for (int i = 0; i < state->n_inputs_max; i++) {
		if (state->inputs_used[i]) {
			result *= state->inputs[i].d;
		}
	}
	return result;
}

double gs_fadd(void *v) {
	struct gs_arithmetic_state *state = v;
	double result = 0;
	for (int i = 0; i < state->n_inputs_max; i++) {
		if (state->inputs_used[i]) {
			result += state->inputs[i].d;
		}
	}
	return result;
}

union i_d *gs_arithmetic_get_avail_slot(struct gs_arithmetic_state *state) {
	int found_slot = -1;
	for (int i = 0; i < state->n_inputs_max; i++) {
		if (!state->inputs_used[i]) {
			found_slot = i;
			state->inputs_used[i] = true;
			break;
		}
	}
	if (found_slot == -1) {
		fprintf(stderr, "Couldn't get input slot for arithmetic node :(\n");
		abort();
	}
	return &state->inputs[found_slot];
}


int f_to_i(double num) {
	return INT_MAX*num;
}

double gs_f_to_i_state(void *v) {
	struct gs_audio_out_state *state = v;
	state->i = INT_MAX * state->d;
	return 0;
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
