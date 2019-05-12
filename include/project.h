#ifndef _PROJECT_H
#define _PROJECT_H

#include "graph.h"

struct gs_project {
	int sample_rate;
	long counter_global;
	struct gs_graph graph;
	int *audio_out_i;
	double *master_volume;
};
#endif
