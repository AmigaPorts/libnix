#include <string.h>
#include <errno.h>
#include <sys/utsname.h>

#include <proto/exec.h>
#include <exec/execbase.h>
#include "stdio.h"

int uname (struct utsname * uname) {
	struct Library * versionLibrary;

	if (!uname) {
		errno = EFAULT;
		return -1;
	}

	strcpy(uname->sysname, "AmigaOS");
	strcpy(uname->nodename, "alive");

	/* The Kickstart: exec's version and SoftVer, e.g. 40.68 for 3.1 */
	sprintf(uname->release, "%d.%d", SysBase->LibNode.lib_Version, SysBase->SoftVer);

	/* The Workbench release that version.library on disk tells, or exec's */
	versionLibrary = OpenLibrary((CONST_STRPTR)"version.library", 0);
	if (versionLibrary) {
		sprintf(uname->version, "%d.%d", versionLibrary->lib_Version, versionLibrary->lib_Revision);
		CloseLibrary(versionLibrary);
	} else {
		sprintf(uname->version, "%d.%d", SysBase->LibNode.lib_Version, SysBase->LibNode.lib_Revision);
	}

	strcpy(uname->machine, "m68k");

	return 0;
}
