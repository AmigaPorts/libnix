#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <inetd.h>
#include "debuglib.h"
#include "select.h"
#include "socket.h"
#include "stabs.h"
#include "stdio.h"

/*
**
*/
extern int    __argc;
extern char **__argv;

extern int h_errno;

struct Library * SocketBase;

/*
**
*/
static ssize_t __stdargs _sock_read(struct _StdFileDes *fp,void *buf,size_t len)
{
  return recv(fp->lx_pos,buf,len,0);
}

static ssize_t __stdargs _sock_write(struct _StdFileDes *fp,const void *buf,size_t len)
{
  return send(fp->lx_pos,buf,len,0);
}

static int __stdargs _sock_close(struct _StdFileDes *fp)
{ struct SocketSettings *lss;
  int rc;

  switch (lss=_lx_get_socket_settings(),lss->lx_network_type) {

    case LX_AMITCP:
      rc = TCP_CloseSocket(fp->lx_sock);
    break;

    default:
      rc = 0;
    break;
  }

  return rc;
}

static int __stdargs _sock_dup(struct _StdFileDes *fp)
{ struct SocketSettings *lss;
  StdFileDes *fp2;
  int rc;

  if ((fp2=_create_socket(fp->lx_family, fp->lx_domain, fp->lx_protocol)) == NULL)
    return -1;

  switch (lss=_lx_get_socket_settings(),lss->lx_network_type) {

    case LX_AMITCP:
      rc = TCP_Dup2Socket(fp->lx_sock, -1);
    break;

    default:
      rc = -1;
    break;
  }

  if (rc >= 0) {
    fp2->lx_sock = rc; rc = fp2->lx_pos;
    /* the copy shares the socket, and so whether it blocks */
    fp2->lx_flags |= fp->lx_flags & LX_NONBLOCK;
  }
  else {
    fp2->lx_inuse = 0;
  }

  return rc;
}

static int __stdargs _sock_fstat(struct _StdFileDes *fp,struct stat *sb)
{ long value;
  socklen_t size = sizeof(value);

  memset(sb, 0, sizeof(*sb));

//sb->st_uid = 0;
//sb->st_gid = 0;

  if (getsockopt(fp->lx_sock, SOL_SOCKET, SO_SNDBUF,(void *)&value, &size) == 0)
    sb->st_blksize = value;

  return 0;
}

static int __stdargs _sock_poll(struct _StdFileDes *fp,int io_mode,struct SocketSettings *lss)
{ struct timeval tv = { .tv_sec = 0, .tv_usec = 0 };
  fd_set in, out, exc;
  int rc;

  FD_ZERO(&in);
  FD_ZERO(&out);
  FD_ZERO(&exc);

  switch (io_mode) {
    case SELMODE_IN:
      FD_SET(fp->lx_sock,&in);
    break;

    case SELMODE_OUT:
      FD_SET(fp->lx_sock,&out);
    break;

    case SELMODE_EXC:
      FD_SET(fp->lx_sock,&exc);
    break;
  }

  switch (lss->lx_network_type) {

    case LX_AMITCP:
      rc = TCP_WaitSelect(fp->lx_sock, &in, &out, &exc, &tv, NULL);
    break;

    default:
      rc = -1;
    break;
  }

  return ((rc == 1) ? 1 : 0);
}

static int __stdargs _sock_select(struct _StdFileDes *fp,int select_cmd,int io_mode,void *fdset,u_long *nfds)
{ struct SocketSettings *lss = _lx_get_socket_settings();
  fd_set *set = fdset;
  
  if (select_cmd == SELCMD_PREPARE) {
    FD_SET(fp->lx_sock, set);
    if ((u_long) fp->lx_sock > *nfds)
      *nfds = fp->lx_sock;
    return (1L << lss->lx_sigurg | 1L << lss->lx_sigio);
  }

  if (select_cmd == SELCMD_CHECK)
    return FD_ISSET(fp->lx_sock, set);

  if (select_cmd == SELCMD_POLL)
    return _sock_poll(fp, io_mode, lss);

  return 0;
}

StdFileDes *_create_socket(int family,int type,int protocol);

static struct _StdFileFx socket_fx = {
		_sock_read, _sock_write, _sock_close, _sock_dup, _sock_fstat, _sock_select
};

StdFileDes *_create_socket(int family,int type,int protocol)
{ extern StdFileDes *__allocfd(void);
  StdFileDes *sfd = __allocfd();

  if (sfd) {
    sfd->lx_flags     = LX_SOCKET;
    sfd->lx_inuse    = 1;
    sfd->lx_family   = family;
    sfd->lx_domain   = type;
    sfd->lx_protocol = protocol;
    sfd->lx_fx       = &socket_fx;
  }

  return sfd;
}

/*
** Redirect stdio (stdin, stdout and stderr) to/from socket 'sock'.
** This is done by dup2()'ing 'sock' to the level 1 files underneath
** the stdin, stdout and stderr.
*/
static int set_socket_stdio(int sock)
{
  if (sock>=0 && dup2(sock,0)>=0 && dup2(sock,1)>=0 && dup2(sock,2)>=0) {
    /*
     * Close the original reference to the sock if necessary
     */
    if (sock > 2)
      close(sock);

    return 0;
  }

  return -1;
}

/*
 *  init_inet_daemon.c - obtain socket accepted by the inetd
 *
 *  Copyright � 1994 AmiTCP/IP Group,
 *       Network Solutions Development Inc.
 *       All rights reserved.
 *  Portions Copyright � 1995 by Jeff Shepherd
 */

static int init_inet_daemon(int *argc,char ***argv,struct SocketSettings *lss)
{ StdFileDes *fp = NULL;
  int sock, err;

  if (lss->lx_network_type == LX_AMITCP) {
    struct Process *me = (struct Process *)FindTask(NULL);
    struct DaemonMessage *dm = (struct DaemonMessage *)me->pr_ExitData;

    DB( BUG("daemon message is %lx\n", dm); )
    if (dm == NULL) {
      /*
      * No DaemonMessage, return error code - probably not an inet daemon
      */
      return -1;
    }

    /*
     * Obtain the server socket
     */
    sock = TCP_ObtainSocket(dm->dm_Id, dm->dm_Family, dm->dm_Type, 0);
    DB( BUG("Obtained socket is %ld\n", sock); )

    if (sock < 0) {
      /*
      * If ObtainSocket fails we need to exit with this specific exit code
      * so that the inetd knows to clean things up
      */
      exit(DERR_OBTAIN);
    }

    err = -1;

    do {

      if ((fp=_create_socket(dm->dm_Family, dm->dm_Type, 0)) == NULL)
        break;
      fp->lx_sock = sock;

      DB( BUG("FD for daemon socket is %ld\n", fp->lx_pos); )

      err = 0;
    } while (0);

    return ((errno = err) != 0 ? -1 : fp->lx_pos);
  }

  return -1;
}

/*
**
*/
struct SocketSettings *_lx_get_socket_settings(void) 
{ static struct SocketSettings lx_socket_settings;
  return &lx_socket_settings;
}

/*
**
*/
extern int network_installed;

/*
**
*/
void __initsocket(void)
{ struct SocketSettings *lss = _lx_get_socket_settings();
  int sock;

  lss->lx_sigurg = AllocSignal(-1l);
  lss->lx_sigio  = AllocSignal(-1l);
  DB( BUG("sigurg is %ld\nsigio is %ld\n", lss->lx_sigurg, lss->lx_sigio); )

  lss->lx_network_type = LX_AMITCP;
  if ((lss->lx_BsdSocketBase=OpenLibrary("bsdsocket.library",3))) {
    struct Process *pr = (struct Process *)FindTask(NULL);
    struct CommandLineInterface *cli = BADDR(pr->pr_CLI);
    char *progname = (cli ? BADDR(cli->cli_CommandName) : pr->pr_Task.tc_Node.ln_Name);
    struct TagItem list[] = {
      { SBTM_SETVAL(SBTC_ERRNOPTR(sizeof(int))), (ULONG)&errno    },
      { SBTM_SETVAL(SBTC_HERRNOLONGPTR),         (ULONG)&h_errno  },
      { SBTM_SETVAL(SBTC_LOGTAGPTR),             (ULONG)progname  },
      { SBTM_SETVAL(SBTC_SIGURGMASK),            0                },
      { SBTM_SETVAL(SBTC_SIGIOMASK),             0                },
      { SBTM_SETVAL(SBTC_BREAKMASK),             SIGBREAKF_CTRL_C },
      { TAG_END,TAG_END }
    };

    DB( BUG("lss->lx_BsdSocketBase is %lx\n", lss->lx_BsdSocketBase); )
    DB( BUG("progname is %s\n", progname); )

	SocketBase = lss->lx_BsdSocketBase;

    list[3].ti_Data = (1L << lss->lx_sigurg);
    list[4].ti_Data = (1L << lss->lx_sigio);

    /* I will assume this always is successful */
    TCP_SocketBaseTagList(list);

    /* only use usergroup stuff when AmiTCP is started only */
    /* I call OpenLibrary() with the full path since
     * usergroup.library might open yet - some people bypass the
     * "login" command which loads usergroup.library
     */
#if 1
    lss->lx_UserGroupBase = OpenLibrary("usergroup.library",1);
    if (lss->lx_UserGroupBase) {
        struct TagItem ug_list[] = {
          { UGT_ERRNOPTR(sizeof(int)), (ULONG)&errno    },
          { UGT_INTRMASK,              SIGBREAKB_CTRL_C },
          { TAG_END,TAG_END }
        };
        ug_SetupContextTagList(progname, ug_list);
    }
#else
   if (1) {
	   lss->lx_UserGroupBase = 0;
#endif
      sock = init_inet_daemon(&__argc, &__argv, lss);
      if (sock >= 0) {
        set_socket_stdio(sock);
      }

      network_installed = 1;
      return;
  }

  FreeSignal(lss->lx_sigio);
  FreeSignal(lss->lx_sigurg);

  lss->lx_network_type = LX_NONE;
}

void __exitsocket(void)
{ struct SocketSettings *lss = _lx_get_socket_settings();

  DB( BUG("__exitsocket start\n"); )

  switch (lss->lx_network_type) {
    case LX_AMITCP:
      DB( BUG("lss->lx_UserGroupBase is %lx\n", lss->lx_UserGroupBase); )
      if (lss->lx_UserGroupBase)
        CloseLibrary(lss->lx_UserGroupBase);
      DB( BUG("lss->lx_BsdSocketBase is %lx\n", lss->lx_BsdSocketBase); )
      if (lss->lx_BsdSocketBase) {
        CloseLibrary(lss->lx_BsdSocketBase);
        FreeSignal(lss->lx_sigio);
        FreeSignal(lss->lx_sigurg);
      }
    break;

    default:
      /* silence compiler */
    break;
  }

  DB( BUG("__exitsocket done\n"); )
}

ADD2INIT(__initsocket, -35); // used to be -10
ADD2EXIT(__exitsocket, -35); // need to clean up FD's before closing socket library
