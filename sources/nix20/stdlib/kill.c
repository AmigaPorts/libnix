#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <exec/execbase.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <signal.h>

/* The task with this pid, if it still exists; call under Forbid().  A pid
   is a task address, see getpid().  */
static struct Task *findpid(pid_t pid) {
	struct Task *want = (struct Task *)((unsigned long)pid << 2), *t;
	struct List *lists[2] = { &SysBase->TaskReady, &SysBase->TaskWait };
	int i;

	if (want == SysBase->ThisTask)
		return want;
	for (i = 0; i < 2; i++)
		for (t = (struct Task *)lists[i]->lh_Head; t->tc_Node.ln_Succ;
		     t = (struct Task *)t->tc_Node.ln_Succ)
			if (t == want)
				return t;
	return NULL;
}

/* SIGINT ends this program the way a shell's Break does; another task gets
   Ctrl-C for SIGINT and SIGTERM, which is all a task can be sent.  */
int kill(pid_t pid, int signo) {
	struct Task *task;

	if (pid <= 0) {
		errno = EINVAL;
		return -1;
	}
	if (pid == getpid()) {
		if (signo == SIGINT) {
			write(2, "***Break\n", 9);
			exit(20);
		}
		return 0;
	}
	if (signo != SIGTERM && signo != SIGINT) {
		errno = EINVAL;
		return -1;
	}

	Forbid();
	task = findpid(pid);
	if (task)
		Signal(task, SIGBREAKF_CTRL_C);
	Permit();
	if (!task) {
		errno = ESRCH;
		return -1;
	}
	return 0;
}
