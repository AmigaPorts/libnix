/* What uname(), getpid(), kill() and sysconf() report, checked against exec.
 * newlib's testsuite has the same test: both runtimes must answer alike. */
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <exec/execbase.h>
#include <proto/exec.h>

static int failed;

#define CHECK(c) do { \
	if (!(c)) { \
		printf("FAIL: %s (line %d)\n", #c, __LINE__); \
		failed++; \
	} \
} while (0)

#define TERMINATED(field) (memchr((field), 0, sizeof (field)) != NULL)

int main(void)
{
	struct utsname u;
	int major, minor;

	memset(&u, 'x', sizeof u);
	CHECK(uname(&u) == 0);
	CHECK(TERMINATED(u.sysname));
	CHECK(TERMINATED(u.nodename));
	CHECK(TERMINATED(u.release));
	CHECK(TERMINATED(u.version));
	CHECK(TERMINATED(u.machine));
	if (!failed)
		printf("uname: sysname=%s nodename=%s release=%s version=%s machine=%s\n",
		       u.sysname, u.nodename, u.release, u.version, u.machine);
	CHECK(strcmp(u.sysname, "AmigaOS") == 0);
	CHECK(strcmp(u.machine, "m68k") == 0);
	/* release is the Kickstart: exec's version and SoftVer */
	CHECK(sscanf(u.release, "%d.%d", &major, &minor) == 2);
	CHECK(major == SysBase->LibNode.lib_Version && minor == SysBase->SoftVer);
	/* version is the Workbench's, from version.library, or exec's */
	CHECK(sscanf(u.version, "%d.%d", &major, &minor) == 2 && major > 0);
	errno = 0;
	CHECK(uname(NULL) == -1 && errno == EFAULT);

	/* a pid is the task address, shifted to stay positive */
	pid_t pid = getpid();
	printf("getpid: %ld, task %p\n", (long)pid, (void *)FindTask(NULL));
	CHECK(pid > 0);
	CHECK((struct Task *)((unsigned long)pid << 2) == FindTask(NULL));

	/* SIGTERM to itself is ignored; no task lives at a stack address */
	CHECK(kill(pid, SIGTERM) == 0);
	errno = 0;
	CHECK(kill(0, SIGTERM) == -1 && errno == EINVAL);
	errno = 0;
	CHECK(kill((pid_t)((unsigned long)&u >> 2), SIGTERM) == -1 && errno == ESRCH);

	printf("sysconf: pagesize=%ld cpus=%ld clk_tck=%ld\n", sysconf(_SC_PAGESIZE),
	       sysconf(_SC_NPROCESSORS_ONLN), sysconf(_SC_CLK_TCK));
	CHECK(sysconf(_SC_PAGESIZE) > 0);
	CHECK(sysconf(_SC_NPROCESSORS_ONLN) >= 1);
	/* the unit of times() */
	CHECK(sysconf(_SC_CLK_TCK) == CLK_TCK);
	errno = 0;
	CHECK(sysconf(-12345) == -1 && errno == EINVAL);

	printf("%d checks failed\n", failed);
	return failed != 0;
}
