/*
 *  Various definitions, types, ... for checksums in xattr
 *  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  @(#) [Zen] ai_cpri.h   Version 1.16 du 26/09/26 - 
 *
 *  vim: ts=4 sw=4 et foldmethod=marker :
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * ------------------------------------------------------------------------------
 */

#if ! defined(_AI_CPRI_H)
#define _AI_CPRI_H

#if defined(AI_RSYNC)
#include "ai_wrapper.h"
#endif /* AI_RSYNC */

/* Definitions {{{ */

#if ! defined(FALSE)
#define FALSE                   (0)
#endif

#if! defined(TRUE)
#define TRUE                    (1)
#endif

#if ! defined(BOOL)
#define BOOL                    int
#endif

#define BUFFER_SIZE             (1024 * 1024)

/* Widths for display */
#define AI_PATH_WIDTH           (25)
#define AI_TAG_WIDTH            (18)
#define AI_MD5_WIDTH            (32)
#define AI_SHA256_WIDTH         (64)
#define AI_SHA512_WIDTH         (128)

/* Types of the xattr */
#define AI_DATE                 (0x0001)
#define AI_MD5                  (0x0002)
#define AI_SHA256               (0x0004)
#define AI_SHA512               (0x0008)

/* Origin of the checksums */
#define AI_ORIGIN_XATTR         (0xAA)
#define AI_ORIGIN_COMPUTED      (0xCC)

/* Number of checksums : MD5, SHA256, SHA512 => 3 */
#define AI_CHECKSUMS_NR         (3)             // Number of manager checksums

/* Names of the xattr */
#define AI_XATTR_DATE           (unsigned char *) "date"          // XATTR name for timestamp
#define AI_XATTR_MD5            (unsigned char *) "md5_sum"       // XATTR name for MD5 checksum
#define AI_XATTR_SHA256         (unsigned char *) "sha256_sum"    // XATTR name for SHA256 checksum
#define AI_XATTR_SHA512         (unsigned char *) "sha512_sum"    // XATTR name for SHA512 checksum

#define AI_DISP_UNSET           (0)
#define AI_DISP_YES             (1)
#define AI_DISP_NO              (2)

/* Exit codes */
#define AI_EXIT_OK              (0)
#define AI_EXIT_USAGE           (1)
#define AI_EXIT_ERR_INTERNAL    (2)
#define AI_EXIT_ERR_STAT        (3)

/* For debugging purposes */
#define X                       if (G.debug) {\
                                    fprintf(stderr, "%s(%3d) : %s()\n", __FILE__, __LINE__, __func__); \
                                }
#define Z                       { fprintf(stderr, "%s(%d) [%s()]\n", __FILE__, __LINE__, __func__); }


/* Namespace of the xattr */
#define AI_NAMESPACE            "trusted"       // Extended attribute type

/* Definitions }}} */
/* Type definitions {{{ */
typedef struct ai_xattr_sums         ai_xattr_sums;
typedef struct ai_xattr_desc         ai_xattr_desc;

/* Type definitions }}} */
/* Enumerations {{{ */
typedef enum ai_retcode {
    AI_RET_OK = 0,
    AI_RET_ERR_OPEN,
    AI_RET_ERR_FDOPEN,
    AI_RET_ERR_FCLOSE,
    AI_RET_ERR_STAT,
    AI_RET_ERR_GETXATTR,
    AI_RET_ERR_UNSUPPORTED,
    AI_RET_ERR_IO_ERROR,
    AI_RET_ERR_XATTR_NO_SUMS,
    AI_RET_ERR_XATTR_MISSING_SUMS,
    AI_RET_ERR_XATTR_NEEDS_UPDATE,
} ai_retcode;
/* Enumerations }}} */
/* Structures {{{ */
/* struct global_params {{{ */
struct global_params {
    char                    *progname;
    int                      path_width,
                             tag_width,
                             checksum_width;
    int                      disp_md5,
                             disp_sha256,
                             disp_sha512;
    BOOL                     debug,
                             disp_name_only,
                             force_checksums,
                             list,
                             recurse,
                             silent,
                             verbose,
                             legacy_format,
                             unique_target,
                             dir_checksum;
    int                      list_OK,
                             list_none,
                             list_missing,
                             list_needs_update,
                             list_needs_sums,
                             list_status;
};

/* struct global_params }}} */
/* struct ai_xattr_sums {{{ */
/* This struct contains the values of the xattr of a file */
struct ai_xattr_sums {
    unsigned char           *chksum_ts_str,
                            *MD5,
                            *SHA256,
                            *SHA512;
};

/* struct ai_xattr_sums }}} */
/* struct ai_xattr_desc {{{ */
/* This struct describes an xattr */
struct ai_xattr_desc {
    int                      chksum_type;
    unsigned char           *xattr_name;
    unsigned char           *xattr_value;
    BOOL                     is_checksum;
};

/* struct ai_xattr_desc }}} */
/* Structures }}} */

#endif  /* _AI_CPRI_H */
