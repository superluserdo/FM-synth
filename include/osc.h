#ifndef _OSC_H
#define _OSC_H

#include <stdbool.h>

struct gs_oscillator_state {
	double f;
	double v;
	double theta;
	double offset;
};

union i_d {
	int i;
	double d;
};

struct gs_audio_out_state {
	int i;
	double d;
};

struct gs_arithmetic_state {
	int n_inputs_max;
	bool *inputs_used;
	union i_d *inputs;
};

struct oscillator_old {
	double f_mult;
	double f;
	double v;
	double theta;
	double offset;
	double (*func)(double f, double v, double theta, double offset);
	struct oscillator_old *input_osc;
};

struct seq_note {
	int key[2];
	int duration;
	float frac_held;
};

struct voice {
	int playing;
	struct seq_note *seq;
	int seq_index;
	int sample_index;
	int seq_len;
	double step_len;
	struct oscillator_old *osc;

	/*	Defined at sample 0 of every note	*/
	int samples_atk;
	int samples_dec;
	int samples_sus;
	int samples_empty;
	int samples_on;
	int samples_off;
	double x_atk_range;
	double sample_x_atk_mult;
	double x_dec_range;
	double sample_x_dec_mult;
	int samples_env_default;
};

enum keys {A, Bb, B, C, Cs, D, Eb, E, F, Fs, G, Ab};

struct oscillator_old *osc_deepcopy(struct oscillator_old *osc_orig);
int osc_play_seq(struct voice *voice);

struct gs_graph_node gs_new_oscillator(int n_outputs, struct output_node_value outputs[n_outputs], struct gs_oscillator_state *state, double (*func)(void *));
union i_d *gs_arithmetic_get_avail_slot(struct gs_arithmetic_state *state);

/* Oscillator primitives: */

int square(double f, double v);
double sinew(void *v);
double gs_osc_sin(void *v);
double gs_fmul(void *v);
double gs_fadd(void *v);
double gs_f_to_i_state(void *v);

/* Output */

void writeint(int num);
#endif
