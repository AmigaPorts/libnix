#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <exec/exec.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <errno.h>
#include <stdlib.h>
#include "stabs.h"
#include "stdio.h"

/* popen() runs the command asynchronously with one end of a PIPE: channel
   as its input or output, and returns the other end.  An NP_ExitCode hook
   records the command's return code and signals the caller, so pclose()
   can wait for it.  */

struct popen_child {
	struct popen_child *next;
	FILE *fp;
	struct Task *parent;
	volatile LONG rc;
	volatile BYTE done;
	BYTE sig;
};

static struct popen_child *children;
static unsigned long counter;

/* Runs in the command's process as it exits, with the process's return
   code in d0 and NP_ExitData in d1.  That process is the shell, whose own
   return code is not the command's (OS 3.2 always passes 3976), so take
   the command's from the shell's CLI structure.  It must not touch our
   globals: in -fbaserel builds a4 does not point at our data there.
   Forbid() keeps pclose() from freeing the signal between setting done
   and Signal().  */
static void child_exit(LONG rc asm("d0"), struct popen_child *c asm("d1")) {
	struct ExecBase *SysBase = *(struct ExecBase **)4;
	struct Process *me = (struct Process *)SysBase->ThisTask;
	struct CommandLineInterface *cli = BADDR(me->pr_CLI);

	Forbid();
	c->rc = cli ? cli->cli_ReturnCode : rc;
	c->done = 1;
	Signal(c->parent, 1UL << c->sig);
	Permit();
}

FILE *popen(const char *command, const char *type) {
	char pname[40];
	struct popen_child *c;
	BPTR end, other;
	FILE *fp;
	int reading;

	if ((type[0] != 'r' && type[0] != 'w') || type[1] != '\0') {
		errno = EINVAL;
		return NULL;
	}
	reading = type[0] == 'r';

	if ((c = malloc(sizeof(*c))) == NULL) {
		errno = ENOMEM;
		return NULL;
	}
	if ((c->sig = AllocSignal(-1)) < 0) {
		free(c);
		errno = EAGAIN;
		return NULL;
	}
	c->parent = FindTask(NULL);
	c->rc = -1;
	c->done = 0;

	sprintf(pname, "PIPE:libnix-popen-%08lx-%lu", (unsigned long)c->parent, counter++);

	/* Open both ends before starting the command: if it finished before
	   we opened ours, the channel would be gone and we would wait on a
	   new, empty one.  */
	end = Open((CONST_STRPTR)pname, reading ? MODE_NEWFILE : MODE_OLDFILE);
	fp = end ? fopen(pname, type) : NULL;
	if (fp) {
		/* The command's other stream: no input while we read its output,
		   and our console, if there is one, for its output while we feed
		   its input.  */
		if (reading)
			other = Open((CONST_STRPTR)"NIL:", MODE_OLDFILE);
		else if ((other = Open((CONST_STRPTR)"*", MODE_NEWFILE)) == 0)
			other = Open((CONST_STRPTR)"NIL:", MODE_NEWFILE);
		if (other) {
			/* On success the command's process closes end and other.  */
			if (SystemTags((CONST_STRPTR)command,
					SYS_Input, reading ? other : end,
					SYS_Output, reading ? end : other,
					SYS_Asynch, TRUE,
					SYS_UserShell, TRUE,
					NP_ExitCode, (ULONG)child_exit,
					NP_ExitData, (ULONG)c,
					TAG_END) != -1) {
				c->fp = fp;
				c->next = children;
				children = c;
				return fp;
			}
			Close(other);
		}
		fclose(fp);
	}
	if (end)
		Close(end);
	FreeSignal(c->sig);
	free(c);
	errno = EIO;
	return NULL;
}

/* Returns the command's AmigaDOS return code, as system() does.  */
int pclose(FILE *fp) {
	struct popen_child **p, *c;
	LONG rc;
	int closed, err;

	for (p = &children; (c = *p) != NULL; p = &c->next)
		if (c->fp == fp)
			break;
	if (c == NULL) {
		errno = ECHILD;
		return -1;
	}
	*p = c->next;

	/* Closing our end gives the command end of file on its input.  A
	   failed flush of what we wrote to it is an error, whatever the
	   command returns.  */
	closed = fclose(fp);
	err = errno;
	while (!c->done)
		Wait(1UL << c->sig);
	SetSignal(0, 1UL << c->sig);
	FreeSignal(c->sig);
	rc = c->rc;
	free(c);
	if (closed != 0) {
		errno = err;
		return -1;
	}
	return rc;
}

/* A command still running would call child_exit() after our code is
   unloaded, so wait for it.  This runs before stdio closes the files.  */
void __exitpopen(void) {
	while (children)
		pclose(children->fp);
}

ADD2EXIT(__exitpopen, -25);
