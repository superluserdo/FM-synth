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
	double duration;
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

struct gs_adsr {
	int atk_samples;
	double (*atk_func)(int samples, int atk_samples);
	int dec_samples;
	double (*dec_func)(int samples, int dec_samples, double sus_val);
	double sus_val;
	int rel_samples;
	double (*rel_func)(int samples, int rel_samples, double sus_val);
};

struct f_samples {
	double f;
	int samples;
};

struct gs_sequence {
	int seq_len;
	struct seq_note *notes;
};

struct gs_sequencer_state {
	struct f_samples f_samples;
	struct gs_sequence sequence;
	int curr_note;
	double curr_beat;
	double curr_note_beat_progress;
	int curr_sample;
	int samples_per_beat;
};

struct gs_adsr_state {
	struct f_samples f_samples;
	struct gs_adsr adsr;
};

enum keys {A, Bb, B, C, Cs, D, Eb, E, F, Fs, G, Ab};

struct oscillator_old *osc_deepcopy(struct oscillator_old *osc_orig);
int osc_play_seq(struct voice *voice);

struct gs_graph_node gs_new_oscillator(int n_outputs, struct output_node_value outputs[n_outputs], struct gs_oscillator_state *state, double (*func)(struct gs_graph_node *));
//union i_d *gs_arithmetic_get_avail_slot(struct gs_arithmetic_state *state);
//struct gs_graph_node *gs_new_voice(struct gs_graph *graph, struct gs_sequencer *sequencer, struct gs_graph_node node, size_t size);
struct gs_graph_node *gs_new_voice(struct gs_graph *graph, struct gs_graph_node *sequencer, struct gs_graph_node *adsr_controller, struct gs_graph_node node, size_t size);
void *gs_arithmetic_get_avail_slot(struct gs_arithmetic_state *state);
struct gs_graph_node *gs_osc_seq_freqmod(struct gs_graph *graph, struct gs_graph_node *sequencer, struct gs_graph_node *target, double modf_multiple, double modv, int mul);
void gs_seq_func(struct gs_graph_node *node);
double gs_atk_func_linear(int samples, int atk_samples);
double gs_dec_func_linear(int samples, int dec_samples, double sus_val);

/* Oscillator primitives: */

int square(double f, double v);
void sinew(struct gs_graph_node *);
void gs_osc_sin(struct gs_graph_node *);
void gs_fmul(struct gs_graph_node *);
void gs_fadd(struct gs_graph_node *);
void gs_f_to_i_state(struct gs_graph_node *);

/* Output */

void writeint(int num);
#endif
