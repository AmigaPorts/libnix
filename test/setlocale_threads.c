/* SPDX-License-Identifier: MIT */
/* Copyright (c) 2026 Dalsin Limited */
/* setlocale_threads: what libstdc++'s generic locale model does for every
   floating-point number a stream reads or writes, from 8 threads at once.
   Reading (__convert_to_v): save setlocale(LC_ALL, NULL), set "C", strtod,
   set the saved name back. Writing (__convert_from_v): the same with
   LC_NUMERIC around vsnprintf, unless the name is already "C".
   With libnix's old setlocale() it hangs; with the fixed implementation
   it prints "setlocale_threads OK" and returns 0: all 8 threads
   finished, every number right, and setlocale() never returned NULL.
   3000 rounds make the threads overlap: at 300 a thread can finish within
   one time slice. */
#include <locale.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef NT
#define NT 8
#endif
#ifndef ROUNDS
#define ROUNDS 3000
#endif
static int bad, done, nulls;
static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

static char *save_and_set_c(int category)
{
	const char *old = setlocale(category, NULL);
	char *sav;
	if (old == NULL)
		return NULL;
	if ((sav = malloc(strlen(old) + 1)) == NULL)
		return NULL;
	strcpy(sav, old);
	setlocale(category, "C");
	return sav;
}

static void *run(void *p)
{
	int t = (int) (long) p, i, b = 0, n = 0;
	for (i = 0; i < ROUNDS; i++) {
		char buf[64], want[64], *e, *sav;
		long a;
		double d;
		/* writing a number, as num_put does */
		sav = save_and_set_c(LC_NUMERIC);
		if (sav == NULL)
			n++;
		snprintf(buf, sizeof buf, "%d %.2f", i, i / 4.0 + t);
		if (sav) {
			setlocale(LC_NUMERIC, sav);
			free(sav);
		}
		/* reading it back, as num_get does */
		sav = save_and_set_c(LC_ALL);
		if (sav == NULL)
			n++;
		a = strtol(buf, &e, 10);
		d = strtod(e, &e);
		if (sav) {
			setlocale(LC_ALL, sav);
			free(sav);
		}
		snprintf(want, sizeof want, "%d %d.%02d", i, i / 4 + t, (i % 4) * 25);
		if (a != i || d != i / 4.0 + t || strcmp(buf, want) != 0)
			b++;
		if (i % 100 == 0) {
			pthread_mutex_lock(&m);
			printf("thread %d at %d: %s\n", t, i, buf);
			fflush(stdout);
			pthread_mutex_unlock(&m);
		}
	}
	pthread_mutex_lock(&m);
	bad += b;
	nulls += n;
	done++;
	pthread_mutex_unlock(&m);
	return NULL;
}

int main(void)
{
	pthread_t th[NT];
	long t;
	const char *s = setlocale(LC_ALL, NULL);
	char *first = s ? strdup(s) : NULL, *c;
	for (c = first; c && *c; c++)
		if (*c == '\n')
			*c = '|';
	printf("locale at start: %s\n", first ? first : "(NULL)");
	fflush(stdout);
	if (setlocale(LC_ALL, "C") == NULL)
		return 20;
	for (t = 0; t < NT; t++)
		pthread_create(&th[t], NULL, run, (void *) t);
	for (t = 0; t < NT; t++)
		pthread_join(th[t], NULL);
	printf("%d threads done, %d wrong, %d NULL names\n", done, bad, nulls);
	if (done == NT && bad == 0 && nulls == 0) {
		puts("setlocale_threads OK");
		return 0;
	}
	puts("setlocale_threads FAILED");
	return 20;
}
