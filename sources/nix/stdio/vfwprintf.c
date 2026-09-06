#include <stdarg.h>
#include "stdio.h"
#include "wchar.h"

extern int __vfwprintf_total_size(FILE *stream, const char *fmt, va_list args);

int vfwprintf(FILE *stream, const wchar_t *fmt, va_list args)
{
	int total_write_size = __vfwprintf_total_size(stream, fmt, args);
	if(stream->_flags & __SERR) {
		return -1;
	}
	return total_write_size;
}

#ifdef TESTME
int main(int argc, char ** argv) {
  double d;
	wprintf(L"%-20.0f|\n", 0.0d);
	wprintf(L"%-20.0e\n", 0.0d);
	wprintf(L"%-20.1g\n", 0.0d);
	wprintf(L"%#20.0f\n", 0.0d);
	wprintf(L"%#20.0e\n", 0.0d);
	wprintf(L"%#20.1g\n", 0.0d);

	d = 1.2345678902468e-13;
	for (int i = 0; i < 24; ++i) {
		d *= 10;
		wprintf(L"%20.7f\n", d);
	}

	d = 1.2345678902468e-13;
	for (int i = 0; i < 24; ++i) {
		d *= 10;
		wprintf(L"%20.7g\n", d);
	}

	d = 1.2345678902468e-13;
	for (int i = 0; i < 24; ++i) {
		d *= 10;
		wprintf(L"%20.7e\n", d);
	}

  return 0;
}
#endif
