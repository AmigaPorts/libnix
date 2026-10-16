/* swprintf: the alternate-form prefix and float digits come out as the wide
   characters they are, not as bytes of a misparenthesized shift or with the
   digit index advanced four times per character. */
#include <stdio.h>
#include <wchar.h>

static int check(const wchar_t *got, int n, const wchar_t *want)
{
	if (n == (int)wcslen(want) && wcscmp(got, want) == 0)
		return 0;
	printf("got %d \"%ls\", want \"%ls\"\n", n, got, want);
	return 1;
}

int main(void)
{
	wchar_t buf[64];
	int fail = 0;

	fail |= check(buf, swprintf(buf, 64, L"%#x %#X", 255, 255), L"0xff 0XFF");
	fail |= check(buf, swprintf(buf, 64, L"%.3f", 3.25), L"3.250");
	fail |= check(buf, swprintf(buf, 64, L"%.2e", 1234.5), L"1.23e+03");
	return fail;
}
