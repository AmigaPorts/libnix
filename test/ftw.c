/* ftw() and nftw(): walk a small tree, check the paths and levels they
   report, then delete it depth-first with FTW_DEPTH as gcov-tool does.  */

#include <ftw.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <proto/dos.h>

static int failures;
static int files, dirs, maxlevel;
static int found_root_entry;

static void check(int cond, const char *what)
{
	if (!cond) {
		printf("FAIL: %s\n", what);
		failures++;
	}
}

static void touch(const char *name)
{
	FILE *f = fopen(name, "w");
	check(f != NULL, name);
	if (f)
		fclose(f);
}

static __stdargs int count(const char *path, const struct stat *st, int type)
{
	(void)path;
	(void)st;
	if (type == FTW_F)
		files++;
	else if (type == FTW_D)
		dirs++;
	return 0;
}

static __stdargs int levels(const char *path, const struct stat *st, int type, struct FTW *ftw)
{
	(void)st;
	(void)type;
	if (ftw->level > maxlevel)
		maxlevel = ftw->level;
	if (!strcmp(path, "ftwtmp/a/b/f3"))
		check(ftw->level == 3 && !strcmp(path + ftw->base, "f3"), "level and base of ftwtmp/a/b/f3");
	return 0;
}

/* Entries right under a volume name get no '/': "SYS:C", not "SYS:/C",
   which is C in the parent.  */
static __stdargs int under_volume(const char *path, const struct stat *st, int type, struct FTW *ftw)
{
	(void)st;
	(void)type;
	if (ftw->level == 1) {
		check(ftw->base == 4 && !strncmp(path, "SYS:", 4) && path[4] != '/', "entry under SYS:");
		found_root_entry = 1;
	}
	return 0;
}

static __stdargs int stop_at_f2(const char *path, const struct stat *st, int type, struct FTW *ftw)
{
	(void)st;
	(void)type;
	(void)ftw;
	return strcmp(path + ftw->base, "f2") ? 0 : 42;
}

static __stdargs int remove_entry(const char *path, const struct stat *st, int type, struct FTW *ftw)
{
	(void)st;
	(void)ftw;
	check(type == FTW_F || type == FTW_DP || type == FTW_SL, "FTW_DEPTH reports directories as FTW_DP");
	return remove(path);
}

int main(int argc, char **argv)
{
	struct stat st;

	check(mkdir("ftwtmp", 0777) == 0, "mkdir ftwtmp");
	check(mkdir("ftwtmp/a", 0777) == 0, "mkdir ftwtmp/a");
	check(mkdir("ftwtmp/a/b", 0777) == 0, "mkdir ftwtmp/a/b");
	touch("ftwtmp/f1");
	touch("ftwtmp/a/f2");
	touch("ftwtmp/a/b/f3");

	check(ftw("ftwtmp", count, 8) == 0, "ftw returns 0");
	check(files == 3 && dirs == 3, "ftw sees 3 files and 3 directories");

	check(nftw("ftwtmp", levels, 8, FTW_PHYS) == 0, "nftw returns 0");
	check(maxlevel == 3, "deepest level is 3");

	files = dirs = 0;
	check(ftw("ftwtmp", count, 1) == 0, "ftw with fd_limit 1 returns 0");
	check(files == 3 && dirs == 3, "fd_limit 1 still visits the whole tree");

	maxlevel = 0;
	check(nftw("ftwtmp/", levels, 8, FTW_PHYS | FTW_MOUNT) == 0, "nftw ftwtmp/ with FTW_MOUNT returns 0");
	check(maxlevel == 3, "FTW_MOUNT stays on the volume and visits it all");

	check(nftw("SYS:", under_volume, 1, FTW_PHYS) == 0, "nftw SYS: returns 0");
	check(found_root_entry, "nftw SYS: reports entries at level 1");

	check(nftw("ftwtmp", stop_at_f2, 8, FTW_PHYS) == 42, "nftw returns the callback's nonzero value");

	check(nftw("ftwtmp", remove_entry, 8, FTW_DEPTH | FTW_PHYS) == 0, "depth-first removal returns 0");
	check(stat("ftwtmp", &st) != 0, "ftwtmp is gone");

	/* FTW_PHYS: a depth-first removal deletes a soft link to a directory,
	   not what it points to.  Only with --links: the simulators have no
	   MakeLink(), so run it on AmigaOS.  */
	if (argc > 1 && !strcmp(argv[1], "--links")) {
		char target[256];
		size_t n;
		BPTR here = Lock((CONST_STRPTR)"", SHARED_LOCK);

		check(here && NameFromLock(here, (STRPTR)target, sizeof(target) - 16), "current directory name");
		UnLock(here);
		n = strlen(target);
		strcpy(target + n, n && target[n-1] == ':' ? "ftwtarget" : "/ftwtarget");
		check(mkdir("ftwtarget", 0777) == 0, "mkdir ftwtarget");
		touch("ftwtarget/keep");
		check(mkdir("ftwtmp", 0777) == 0, "mkdir ftwtmp again");
		check(symlink(target, "ftwtmp/link") == 0, "symlink ftwtmp/link");
		check(nftw("ftwtmp", remove_entry, 8, FTW_DEPTH | FTW_PHYS) == 0, "removal with a soft link returns 0");
		check(stat("ftwtarget/keep", &st) == 0, "the soft link's target is intact");
		check(stat("ftwtmp", &st) != 0, "ftwtmp with the link is gone");
		remove("ftwtarget/keep");
		remove("ftwtarget");
	}

	if (failures)
		return 1;
	printf("ftw: ok\n");
	return 0;
}
