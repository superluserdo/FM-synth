synth: osc.c
	gcc -Wall -shared -o libsynth.so -fPIC osc.c graph.c -I./include -lm -g3
	#gcc -o synth synth_projects/toejam_and_earl.c libsynth/osc.c -I./libsynth/include -lm -g3

clean:
	 rm *.so *.o
