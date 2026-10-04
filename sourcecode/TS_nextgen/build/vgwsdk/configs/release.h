/**************************************************************************/
/*                                                                        */
/*   MODULE:  release.h                                                   */
/*   PURPOSE: release specific definitions file.                          */
/*                                                                        */
/**************************************************************************/

#ifndef _RELEASE_H_
#define _RELEASE_H_


#include "vendor_release.h"
/* 1. the release file was modify to support a vendor release version.
   2. in case VENDOR_SW_VERSION_ACTIVE is defined in vendor_release.h the version is taken from vendor definitions*/
#ifndef VENDOR_SW_VERSION_ACTIVE

#define SW_VERSION_MAJOR    4
#define SW_VERSION_MINOR    5
#define SW_VERSION_PATCH    5
#define SW_VERSION_BUILD    44


#else

/*added a double space after the define cause of CQ auto release file changed  */
#define  SW_VERSION_MAJOR   VENDOR_SW_VERSION_MAJOR
#define  SW_VERSION_MINOR   VENDOR_SW_VERSION_MINOR
#define  SW_VERSION_PATCH   VENDOR_SW_VERSION_PATCH
#define  SW_VERSION_BUILD   VENDOR_SW_VERSION_BUILD

#endif
                      /* YYYY/DD/MM */  
#define SW_BUILD_STR    "2016/10/5 15:55:00 "


#endif


