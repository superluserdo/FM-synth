#ifndef _PROJECT_H
#define _PROJECT_H

#include "graph.h"

struct gs_project {
	int sample_rate;
	double bpm;
	long counter_global;
	struct gs_graph graph;
	int *audio_out_i;
	double *master_volume;
};

struct gs_graph *gs_project_init(struct gs_project *project);

#endif
