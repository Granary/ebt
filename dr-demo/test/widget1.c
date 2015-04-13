#include <stdio.h>
#include <stdlib.h>

typedef struct { int x; int y; } widget_t;

widget_t *create_widget(int x, int y) {
	widget_t *w =  (widget_t *) malloc(sizeof(widget_t));
	w->x = x; w->y = y;
	return w;
}

#define WNUM 32
widget_t *line[WNUM];

void poke_1(int n, int k) {
	printf ("%d poke %d --> %d %d\n", k, n, line[n]->x, line[n]->y);
}

void main()
{
	srand(time(NULL));
	int i, k = 0;
	for (i = 0; i < WNUM; i++)
		line[i] = create_widget(rand() % 100, rand() % 100);
	for (i = 0; i < WNUM; i++)
		if (line[i]->x > 50) {
			poke_1(i, k); k++;
		}
}
