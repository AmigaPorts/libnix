#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
//
#include "socket.h"

#define MAXALIASES 35
#define TCP_PATH_SERVICES "AmiTCP:db/services"

void setservent(int stayopen)
{ struct SocketSettings *lss = _lx_get_socket_settings();

  if (lss->lx_network_type == LX_AMITCP) {
    if (!lss->lx_serv_fp)
      lss->lx_serv_fp = fopen(TCP_PATH_SERVICES, "r");
    else
      rewind(lss->lx_serv_fp);
    lss->lx_serv_stayopen = stayopen;
  }
}

void endservent(void)
{ struct SocketSettings *lss = _lx_get_socket_settings();

  lss->lx_serv_stayopen = 0;
  if (lss->lx_serv_fp) {
    fclose(lss->lx_serv_fp); lss->lx_serv_fp = NULL;
  }
#if 0
  if (lss->lx_serv_aliases) {
    free(lss->lx_serv_aliases); lss->lx_serv_aliases = NULL;
  }
  if (lss->lx_serv_line) {
    free(lss->lx_serv_line); lss->lx_serv_line = NULL;
  }
#endif
}

struct servent *getservent(void)
{ struct SocketSettings *lss = _lx_get_socket_settings();

  if (lss->lx_network_type == LX_AMITCP) {

    if (!lss->lx_serv_line)
      lss->lx_serv_line = malloc(BUFSIZ + 1);

    if (!lss->lx_serv_aliases)
      lss->lx_serv_aliases = malloc(MAXALIASES * sizeof(char *));

    if (!lss->lx_serv_line || !lss->lx_serv_aliases) {
      errno = ENOMEM; return NULL;
    }

    if (lss->lx_serv_fp || (lss->lx_serv_fp=fopen(TCP_PATH_SERVICES, "r"))) {
      for(;;) {
        char *s, *cp, **q;

        if ((s=fgets(lss->lx_serv_line, BUFSIZ, lss->lx_serv_fp)) == NULL)
          break;

        if ((*s == '#') || ((cp=strpbrk(s, "#\n")) == NULL))
          continue;
        *cp = '\0';
        lss->lx_serv.s_name = s;

        if ((s=strpbrk(s, " \t")) == NULL)
          continue;
        *s++ = '\0';
        while (*s == ' ' || *s == '\t')
          s++;
        if ((cp=strpbrk(s, ",/")) == NULL)
          continue;
        *cp++ = '\0';
        lss->lx_serv.s_port = htonl(atoi(s));
        lss->lx_serv.s_proto = cp;

        lss->lx_serv.s_aliases = (__STRPTR *)(q = lss->lx_serv_aliases);
        if ((cp=strpbrk(cp, " \t")) != NULL) {
          *cp++ = '\0';
          while (cp && *cp) {
            if (*cp == ' ' || *cp == '\t') {
              cp++; continue;
            }
            if (q < &lss->lx_serv_aliases[MAXALIASES - 1])
              *q++ = cp;
            if ((cp=strpbrk(cp, " \t")) != NULL)
              *cp++ = '\0';
          }
        }
        *q = NULL; return &lss->lx_serv;
      }
    }
  }
  return NULL;
}
