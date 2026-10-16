/* popen() and pclose().  Not part of check-target: the simulators have no
   PIPE: or shell, so run it on AmigaOS from its own directory:
     popen
   It starts itself as the command, with these arguments:
     --emit N     print N numbered lines, return 7
     --count      count the lines on stdin, return the count
     --wait       sleep a second, return 0 (popen() then exit without
                  pclose(): the exit code must wait for it)  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int failures;

static void check(int cond, const char *what)
{
	printf("%s: %s\n", cond ? "ok" : "FAIL", what);
	if (!cond)
		failures++;
}

int main(int argc, char **argv)
{
	char cmd[256], line[64];
	FILE *f;
	int i, n, rc;

	if (argc == 3 && !strcmp(argv[1], "--emit")) {
		n = atoi(argv[2]);
		for (i = 0; i < n; i++)
			printf("line %d\n", i);
		return 7;
	}
	if (argc == 2 && !strcmp(argv[1], "--count")) {
		n = 0;
		while (fgets(line, sizeof(line), stdin))
			n++;
		return n;
	}
	if (argc == 2 && !strcmp(argv[1], "--wait")) {
		sleep(1);
		return 0;
	}

	check(popen("echo", "rw") == NULL, "popen rejects mode \"rw\"");
	check(pclose(stdin) == -1, "pclose rejects a FILE popen did not open");

	/* Read the command's output, and get its return code.  */
	sprintf(cmd, "%s --emit 3", argv[0]);
	f = popen(cmd, "r");
	check(f != NULL, "popen r");
	if (f) {
		n = 0;
		while (fgets(line, sizeof(line), f)) {
			char want[24];
			sprintf(want, "line %d\n", n);
			if (strcmp(line, want))
				break;
			n++;
		}
		check(n == 3, "read 3 lines");
		rc = pclose(f);
		printf("pclose returned %d\n", rc);
		check(rc == 7, "pclose returns the command's return code");
	}

	/* Feed the command's input.  */
	sprintf(cmd, "%s --count", argv[0]);
	f = popen(cmd, "w");
	check(f != NULL, "popen w");
	if (f) {
		for (i = 0; i < 5; i++)
			fprintf(f, "x%d\n", i);
		rc = pclose(f);
		printf("pclose returned %d\n", rc);
		check(rc == 5, "the command read 5 lines");
	}

	/* Stop reading early: pclose() must not hang on a writer.  */
	sprintf(cmd, "%s --emit 20000", argv[0]);
	f = popen(cmd, "r");
	check(f != NULL, "popen r, large output");
	if (f) {
		check(fgets(line, sizeof(line), f) != NULL, "read the first line");
		rc = pclose(f);
		printf("pclose returned %d\n", rc);
		check(1, "pclose after a partial read returned");
	}

	/* Two at once.  */
	{
		FILE *a, *b;
		sprintf(cmd, "%s --emit 1", argv[0]);
		a = popen(cmd, "r");
		sprintf(cmd, "%s --emit 2", argv[0]);
		b = popen(cmd, "r");
		check(a && b, "two popens");
		if (a && b) {
			n = 0;
			while (fgets(line, sizeof(line), b))
				n++;
			check(pclose(b) == 7 && n == 2, "second one first");
			n = 0;
			while (fgets(line, sizeof(line), a))
				n++;
			check(pclose(a) == 7 && n == 1, "then the first");
		}
	}

	/* Leave one running: the exit code has to wait for it.  */
	sprintf(cmd, "%s --wait", argv[0]);
	check(popen(cmd, "r") != NULL, "popen left open at exit");

	printf(failures ? "popen: FAILED\n" : "popen: ok\n");
	return failures ? 20 : 0;
}
