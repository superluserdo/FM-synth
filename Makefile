synth: osc.c graph.c project.c
	gcc -Wall -shared -o libsynth.so -fPIC osc.c graph.c project.c -I./include -lm -g3

clean:
	 rm *.so *.o
