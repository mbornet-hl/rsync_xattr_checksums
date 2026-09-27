/*
 *	Wrapper for "config.h"
 *	~~~~~~~~~~~~~~~~~~~~~~
 *
 *	@(#)	[Zen] ai_wrapper.h	Version 1.2 du 26/09/25 - 
 *
 */
#ifndef GNULIB_WRAPPER_H
#define GNULIB_WRAPPER_H

/* Inclusion of "config.h" from rsync */
#include "../config.h"

/* Backup of rsync macros */
#pragma push_macro("PACKAGE_BUGREPORT")
#pragma push_macro("PACKAGE_NAME")
#pragma push_macro("PACKAGE_STRING")
#pragma push_macro("PACKAGE_TARNAME")
#pragma push_macro("PACKAGE_URL")
#pragma push_macro("PACKAGE_VERSION")

/* Undefine of rsync macros */
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_URL
#undef PACKAGE_VERSION

#ifndef _GL_CONFIG_H_INCLUDED
/* Include of "config.h" from gnulib */
#include "../../gnulib.build/config.h"
#endif /* _GL_CONFIG_H_INCLUDED */

/* Restoration of rsync macros */
#pragma pop_macro("PACKAGE_VERSION")
#pragma pop_macro("PACKAGE_URL")
#pragma pop_macro("PACKAGE_TARNAME")
#pragma pop_macro("PACKAGE_STRING")
#pragma pop_macro("PACKAGE_NAME")
#pragma pop_macro("PACKAGE_BUGREPORT")

#endif
