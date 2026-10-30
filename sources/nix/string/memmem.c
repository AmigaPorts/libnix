#include <string.h>

void* memmem(const void *haystack, size_t haystacklen,
		const void *needle, size_t needlelen)
{
	if (needlelen == 0) {
		return (void*) haystack;
	}
	if (haystacklen < needlelen || haystack == NULL || needle == NULL) {
		return NULL;
	}

	const unsigned char *h = (const unsigned char*) haystack;
	const unsigned char *n = (const unsigned char*) needle;

	unsigned char first_byte = n[0];

	size_t max_search_len = haystacklen - needlelen + 1;

	while (max_search_len > 0) {
		h = memchr(h, first_byte, max_search_len);
		if (h == NULL) {
			return NULL;
		}

		if (memcmp(h, n, needlelen) == 0) {
			return (void*) h; // found!
		}

		size_t bytes_passed = (const unsigned char*) haystack + haystacklen - h;
		if (bytes_passed <= needlelen) {
			break;
		}

		max_search_len = bytes_passed - needlelen;
		h++;
	}

	return NULL;
}
