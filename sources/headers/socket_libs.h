#ifndef _HEADERS_SOCKET_LIBS_H
#define _HEADERS_SOCKET_LIBS_H

/*
** The calls into bsdsocket.library and usergroup.library, through the inline
** headers sfdc writes from their SFD files.  Each TCP_ and UG_ macro passes
** the base of the library that the caller's socket settings opened to the
** __<function>_base() form of the call.
**
** The proto headers are left out: clib/usergroup_protos.h declares the
** library's getpwnam(), initgroups() and the others, which clash with libnix's
** own.  So are all the plain macros: many have the names of libnix's
** functions, which those functions implement on top of the libraries.
** __NOLIBBASE__ makes the varargs wrappers take the base as an argument,
** rather than use global bases, which libnix doesn't have.
*/
#define _PROTO_BSDSOCKET_H
#define _PROTO_USERGROUP_H
#define BSDSOCKET_BASE_NAME lss->lx_BsdSocketBase
#define USERGROUP_BASE_NAME lss->lx_UserGroupBase
#ifndef __NOLIBBASE__
  #define __NOLIBBASE__
  #define _SOCKET_LIBS_NOLIBBASE
#endif
#include <inline/bsdsocket.h>
#include <inline/usergroup.h>
#ifdef _SOCKET_LIBS_NOLIBBASE
  #undef __NOLIBBASE__
  #undef _SOCKET_LIBS_NOLIBBASE
#endif

#undef AbortInterfaceConfig
#undef AddDomainNameServer
#undef AddInterfaceTagList
#undef AddInterfaceTags
#undef AddNetMonitorHookTagList
#undef AddNetMonitorHookTags
#undef AddRouteTagList
#undef AddRouteTags
#undef BeginInterfaceConfig
#undef ChangeRoadshowData
#undef CloseSocket
#undef ConfigureInterfaceTagList
#undef ConfigureInterfaceTags
#undef CreateAddrAllocMessage
#undef CreateAddrAllocMessageA
#undef DeleteAddrAllocMessage
#undef DeleteRouteTagList
#undef DeleteRouteTags
#undef Dup2Socket
#undef Errno
#undef FreeRouteInfo
#undef GetDefaultDomainName
#undef GetNetworkStatistics
#undef GetRouteInfo
#undef GetSocketEvents
#undef In_CanForward
#undef In_LocalAddr
#undef Inet_LnaOf
#undef Inet_MakeAddr
#undef Inet_NetOf
#undef Inet_NtoA
#undef IoctlSocket
#undef ObtainDomainNameServerList
#undef ObtainInterfaceList
#undef ObtainRoadshowData
#undef ObtainServerSocket
#undef ObtainSocket
#undef ProcessIsServer
#undef QueryInterfaceTagList
#undef QueryInterfaceTags
#undef ReleaseCopyOfSocket
#undef ReleaseDomainNameServerList
#undef ReleaseInterfaceList
#undef ReleaseRoadshowData
#undef ReleaseSocket
#undef RemoveDomainNameServer
#undef RemoveInterface
#undef RemoveNetMonitorHook
#undef SetDefaultDomainName
#undef SetErrnoPtr
#undef SetSocketSignals
#undef SocketBaseTagList
#undef SocketBaseTags
#undef WaitSelect
#undef accept
#undef bind
#undef bpf_close
#undef bpf_data_waiting
#undef bpf_ioctl
#undef bpf_open
#undef bpf_read
#undef bpf_set_interrupt_mask
#undef bpf_set_notify_mask
#undef bpf_write
#undef connect
#undef crypt
#undef endgrent
#undef endnetent
#undef endprotoent
#undef endpwent
#undef endservent
#undef endutent
#undef freeaddrinfo
#undef gai_strerror
#undef getaddrinfo
#undef getcredentials
#undef getdtablesize
#undef getegid
#undef geteuid
#undef getgid
#undef getgrent
#undef getgrgid
#undef getgrnam
#undef getgroups
#undef gethostbyaddr
#undef gethostbyaddr_r
#undef gethostbyname
#undef gethostbyname_r
#undef gethostid
#undef gethostname
#undef getlastlog
#undef getlogin
#undef getnameinfo
#undef getnetbyaddr
#undef getnetbyname
#undef getnetent
#undef getpass
#undef getpeername
#undef getpgrp
#undef getprotobyname
#undef getprotobynumber
#undef getprotoent
#undef getpwent
#undef getpwnam
#undef getpwuid
#undef getservbyname
#undef getservbyport
#undef getservent
#undef getsockname
#undef getsockopt
#undef getuid
#undef getumask
#undef getutent
#undef inet_addr
#undef inet_aton
#undef inet_network
#undef inet_ntop
#undef inet_pton
#undef initgroups
#undef listen
#undef mbuf_adj
#undef mbuf_cat
#undef mbuf_copyback
#undef mbuf_copydata
#undef mbuf_copym
#undef mbuf_free
#undef mbuf_freem
#undef mbuf_get
#undef mbuf_gethdr
#undef mbuf_prepend
#undef mbuf_pullup
#undef recv
#undef recvfrom
#undef recvmsg
#undef send
#undef sendmsg
#undef sendto
#undef setgid
#undef setgrent
#undef setgroups
#undef setlastlog
#undef setlogin
#undef setnetent
#undef setprotoent
#undef setpwent
#undef setregid
#undef setreuid
#undef setservent
#undef setsid
#undef setsockopt
#undef setuid
#undef setutent
#undef shutdown
#undef socket
#undef syslog
#undef ug_GetErr
#undef ug_GetSalt
#undef ug_SetupContextTagList
#undef ug_SetupContextTags
#undef ug_StrError
#undef umask
#undef vsyslog

/* bsdsocket.library */
#define TCP_Socket(domain, type, protocol) ((LONG)__socket_base(BSDSOCKET_BASE_NAME, (domain), (type), (protocol)))
#define TCP_Bind(s, name, namelen) ((LONG)__bind_base(BSDSOCKET_BASE_NAME, (s), (name), (namelen)))
#define TCP_Listen(s, backlog) ((LONG)__listen_base(BSDSOCKET_BASE_NAME, (s), (backlog)))
#define TCP_Accept(s, addr, addrlen) ((LONG)__accept_base(BSDSOCKET_BASE_NAME, (s), (addr), (addrlen)))
#define TCP_Connect(s, name, namelen) ((LONG)__connect_base(BSDSOCKET_BASE_NAME, (s), (name), (namelen)))
#define TCP_SendTo(s, msg, len, flags, to, tolen) ((LONG)__sendto_base(BSDSOCKET_BASE_NAME, (s), (msg), (len), (flags), (to), (tolen)))
#define TCP_Send(s, msg, len, flags) ((LONG)__send_base(BSDSOCKET_BASE_NAME, (s), (msg), (len), (flags)))
#define TCP_RecvFrom(s, buf, len, flags, from, fromlen) ((LONG)__recvfrom_base(BSDSOCKET_BASE_NAME, (s), (buf), (len), (flags), (from), (fromlen)))
#define TCP_Recv(s, buf, len, flags) ((LONG)__recv_base(BSDSOCKET_BASE_NAME, (s), (buf), (len), (flags)))
#define TCP_ShutDown(s, how) ((LONG)__shutdown_base(BSDSOCKET_BASE_NAME, (s), (how)))
#define TCP_SetSockOpt(s, level, optname, optval, optlen) ((LONG)__setsockopt_base(BSDSOCKET_BASE_NAME, (s), (level), (optname), (optval), (optlen)))
#define TCP_GetSockOpt(s, level, optname, optval, optlen) ((LONG)__getsockopt_base(BSDSOCKET_BASE_NAME, (s), (level), (optname), (optval), (optlen)))
#define TCP_GetSockName(s, hostname, namelen) ((LONG)__getsockname_base(BSDSOCKET_BASE_NAME, (s), (hostname), (namelen)))
#define TCP_GetPeerName(s, hostname, namelen) ((LONG)__getpeername_base(BSDSOCKET_BASE_NAME, (s), (hostname), (namelen)))
#define TCP_IoctlSocket(d, request, argp) ((LONG)__IoctlSocket_base(BSDSOCKET_BASE_NAME, (d), (request), (argp)))
#define TCP_CloseSocket(d) ((LONG)__CloseSocket_base(BSDSOCKET_BASE_NAME, (d)))
#define TCP_WaitSelect(nfds, readfds, writefds, execptfds, timeout, maskp) ((LONG)__WaitSelect_base(BSDSOCKET_BASE_NAME, (nfds), (readfds), (writefds), (execptfds), (timeout), (maskp)))
#define TCP_SetSocketSignals(SIGINTR, SIGIO, SIGURG) __SetSocketSignals_base(BSDSOCKET_BASE_NAME, (SIGINTR), (SIGIO), (SIGURG))
#define TCP_GetDTableSize() ((LONG)__getdtablesize_base(BSDSOCKET_BASE_NAME))
#define TCP_ObtainSocket(id, domain, type, protocol) ((LONG)__ObtainSocket_base(BSDSOCKET_BASE_NAME, (id), (domain), (type), (protocol)))
#define TCP_ReleaseSocket(fd, id) ((LONG)__ReleaseSocket_base(BSDSOCKET_BASE_NAME, (fd), (id)))
#define TCP_ReleaseCopyOfSocket(fd, id) ((LONG)__ReleaseCopyOfSocket_base(BSDSOCKET_BASE_NAME, (fd), (id)))
#define TCP_Errno() ((LONG)__Errno_base(BSDSOCKET_BASE_NAME))
#define TCP_SetErrnoPtr(errno_p, size) ((LONG)__SetErrnoPtr_base(BSDSOCKET_BASE_NAME, (errno_p), (size)))
#define TCP_Inet_NtoA(in) ((char *)__Inet_NtoA_base(BSDSOCKET_BASE_NAME, (in)))
#define TCP_Inet_Addr(cp) ((ULONG)__inet_addr_base(BSDSOCKET_BASE_NAME, (cp)))
#define TCP_Inet_LnaOf(in) ((ULONG)__Inet_LnaOf_base(BSDSOCKET_BASE_NAME, (in)))
#define TCP_Inet_NetOf(in) ((ULONG)__Inet_NetOf_base(BSDSOCKET_BASE_NAME, (in)))
#define TCP_Inet_MakeAddr(net, host) ((ULONG)__Inet_MakeAddr_base(BSDSOCKET_BASE_NAME, (net), (host)))
#define TCP_Inet_Network(cp) ((ULONG)__inet_network_base(BSDSOCKET_BASE_NAME, (cp)))
#define TCP_GetHostByName(name) ((struct hostent  *)__gethostbyname_base(BSDSOCKET_BASE_NAME, (name)))
#define TCP_GetHostByAddr(addr, len, type) ((struct hostent  *)__gethostbyaddr_base(BSDSOCKET_BASE_NAME, (addr), (len), (type)))
#define TCP_GetNetByName(name) ((struct netent   *)__getnetbyname_base(BSDSOCKET_BASE_NAME, (name)))
#define TCP_GetNetByAddr(net, type) ((struct netent   *)__getnetbyaddr_base(BSDSOCKET_BASE_NAME, (net), (type)))
#define TCP_GetServByName(name, proto) ((struct servent  *)__getservbyname_base(BSDSOCKET_BASE_NAME, (name), (proto)))
#define TCP_GetServByPort(port, proto) ((struct servent  *)__getservbyport_base(BSDSOCKET_BASE_NAME, (port), (proto)))
#define TCP_GetProtoByName(name) ((struct protoent *)__getprotobyname_base(BSDSOCKET_BASE_NAME, (name)))
#define TCP_GetProtoByNumber(proto) ((struct protoent *)__getprotobynumber_base(BSDSOCKET_BASE_NAME, (proto)))
#define TCP_SyslogA(level, format, ap) __vsyslog_base(BSDSOCKET_BASE_NAME, (level), (format), (ap))
#define TCP_Dup2Socket(fd1, fd2) ((LONG)__Dup2Socket_base(BSDSOCKET_BASE_NAME, (fd1), (fd2)))
#define TCP_SendMsg(s, msg, flags) ((LONG)__sendmsg_base(BSDSOCKET_BASE_NAME, (s), (msg), (flags)))
#define TCP_RecvMsg(s, msg, flags) ((LONG)__recvmsg_base(BSDSOCKET_BASE_NAME, (s), (msg), (flags)))
#define TCP_GetHostName(hostname, size) ((LONG)__gethostname_base(BSDSOCKET_BASE_NAME, (hostname), (size)))
#define TCP_GetHostId() ((ULONG)__gethostid_base(BSDSOCKET_BASE_NAME))
#define TCP_SocketBaseTagList(taglist) ((LONG)__SocketBaseTagList_base(BSDSOCKET_BASE_NAME, (taglist)))
#define TCP_GetSocketEvents(eventmaskp) ((LONG)__GetSocketEvents_base(BSDSOCKET_BASE_NAME, (eventmaskp)))

#ifndef NO_INLINE_STDARG
  #define TCP_SocketBaseTags(tags...) \
    ({ULONG _tags[] = { tags }; TCP_SocketBaseTagList((struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

/* usergroup.library */
#define ug_SetupContextTagList(pname, taglist) ((int)__ug_SetupContextTagList_base(USERGROUP_BASE_NAME, (pname), (taglist)))
#define ug_GetErr() ((int)__ug_GetErr_base(USERGROUP_BASE_NAME))
#define ug_StrError(code) ((const char *)__ug_StrError_base(USERGROUP_BASE_NAME, (code)))
#define UG_getuid() ((uid_t)__getuid_base(USERGROUP_BASE_NAME))
#define UG_geteuid() ((uid_t)__geteuid_base(USERGROUP_BASE_NAME))
#define UG_setreuid(real, eff) ((int)__setreuid_base(USERGROUP_BASE_NAME, (real), (eff)))
#define UG_setuid(id) ((int)__setuid_base(USERGROUP_BASE_NAME, (id)))
#define UG_getgid() ((gid_t)__getgid_base(USERGROUP_BASE_NAME))
#define UG_getegid() ((gid_t)__getegid_base(USERGROUP_BASE_NAME))
#define UG_setregid(real, eff) ((int)__setregid_base(USERGROUP_BASE_NAME, (real), (eff)))
#define UG_setgid(id) ((int)__setgid_base(USERGROUP_BASE_NAME, (id)))
#define UG_getgroups(ngroups, groups) ((int)__getgroups_base(USERGROUP_BASE_NAME, (ngroups), (groups)))
#define UG_setgroups(ngroups, groups) ((int)__setgroups_base(USERGROUP_BASE_NAME, (ngroups), (groups)))
#define UG_initgroups(name, basegroup) ((int)__initgroups_base(USERGROUP_BASE_NAME, (name), (basegroup)))
#define UG_getpwnam(name) ((struct TCP_passwd *)__getpwnam_base(USERGROUP_BASE_NAME, (name)))
#define UG_getpwuid(uid) ((struct TCP_passwd *)__getpwuid_base(USERGROUP_BASE_NAME, (uid)))
#define UG_setpwent() __setpwent_base(USERGROUP_BASE_NAME)
#define UG_getpwent() ((struct TCP_passwd *)__getpwent_base(USERGROUP_BASE_NAME))
#define UG_endpwent() __endpwent_base(USERGROUP_BASE_NAME)
#define UG_getgrnam(name) ((struct group *)__getgrnam_base(USERGROUP_BASE_NAME, (name)))
#define UG_getgrgid(gid) ((struct group *)__getgrgid_base(USERGROUP_BASE_NAME, (gid)))
#define UG_setgrent() __setgrent_base(USERGROUP_BASE_NAME)
#define UG_getgrent() ((struct group *)__getgrent_base(USERGROUP_BASE_NAME))
#define UG_endgrent() __endgrent_base(USERGROUP_BASE_NAME)
#define UG_crypt(key, salt) ((char *)__crypt_base(USERGROUP_BASE_NAME, (key), (salt)))
#define ug_GetSalt(user, buffer, size) ((char *)__ug_GetSalt_base(USERGROUP_BASE_NAME, (user), (buffer), (size)))
#define UG_getpass(prompt) ((char *)__getpass_base(USERGROUP_BASE_NAME, (prompt)))
#define UG_umask(mask) ((mode_t)__umask_base(USERGROUP_BASE_NAME, (mask)))
#define UG_getumask() ((mode_t)__getumask_base(USERGROUP_BASE_NAME))
#define UG_setsid() ((pid_t)__setsid_base(USERGROUP_BASE_NAME))
#define UG_getpgrp() ((pid_t)__getpgrp_base(USERGROUP_BASE_NAME))
#define UG_getlogin() ((char *)__getlogin_base(USERGROUP_BASE_NAME))
#define UG_setlogin(buffer) ((int)__setlogin_base(USERGROUP_BASE_NAME, (buffer)))
#define UG_setutent() __setutent_base(USERGROUP_BASE_NAME)
#define UG_getutent() ((struct utmp *)__getutent_base(USERGROUP_BASE_NAME))
#define UG_endutent() __endutent_base(USERGROUP_BASE_NAME)
#define UG_getlastlog(uid) ((struct lastlog *)__getlastlog_base(USERGROUP_BASE_NAME, (uid)))
#define UG_setlastlog(uid, name, host) ((int)__setlastlog_base(USERGROUP_BASE_NAME, (uid), (name), (host)))
#define UG_getcredentials(task) ((struct UserGroupCredentials *)__getcredentials_base(USERGROUP_BASE_NAME, (task)))

#endif /* _HEADERS_SOCKET_LIBS_H */
