#include <langinfo.h>

/* libnix has only the C locale: the same answer for any locale.
   libstdc++'s <format> and std::text_encoding ask for the encoding this
   way.  */
char *nl_langinfo_l(nl_item item, locale_t locale) {
	(void)locale;
	return nl_langinfo(item);
}
