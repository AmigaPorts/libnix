/* inet_aton, inet_network: dotted addresses parse, and bytes above 0x7f
   are rejected rather than indexing the ctype table out of range. libnix's
   table is Latin-1 and calls 0xa0 a space, so inet_aton needs its isascii()
   test to refuse a trailing no-break space. */
#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static int aton(const char *cp, int want_ok, unsigned long want)
{
	struct in_addr a = { 0 };
	int ok = inet_aton(cp, &a);

	if (ok == want_ok && (!ok || a.s_addr == want))
		return 0;
	printf("inet_aton(\"%s\") = %d, %08lx; want %d, %08lx\n", cp, ok,
	       (unsigned long)a.s_addr, want_ok, want);
	return 1;
}

static int network(const char *cp, unsigned long want)
{
	unsigned long got = inet_network(cp);

	if (got == want)
		return 0;
	printf("inet_network(\"%s\") = %08lx; want %08lx\n", cp, got, want);
	return 1;
}

int main(void)
{
	int fail = 0;

	fail |= aton("10.0.0.1", 1, 0x0a000001);
	fail |= aton("0x7f.1", 1, 0x7f000001);
	fail |= aton("10.0.0.1 trailing", 1, 0x0a000001);
	fail |= aton("10.0.0.1\xa0", 0, 0);
	fail |= aton("10.0.0.1\xe9", 0, 0);
	fail |= aton("10.0.\xb9.1", 0, 0);

	fail |= network("10.1", 0x0a01);
	fail |= network("0x7f.1", 0x7f01);
	fail |= network("10.1\xe9", (unsigned long)-1);
	fail |= network("10.\xb9", (unsigned long)-1);
	return fail;
}
