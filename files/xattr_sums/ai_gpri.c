/*
 *  Global variables for checksums in xattr
 *  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  @(#) [Zen] ai_gpri.c   Version 1.9 du 26/09/26 - 
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

/* Includes {{{ */
#if defined(AI_RSYNC)
#include "../../gnulib.build/config.h"
#else
#include "../gnulib-build/config.h"
#endif /* AI_RSYNC */
// #include <stdio.h>
// #include <stdlib.h>
// #include <fcntl.h>
// #include <errno.h>
#include <pthread.h>
// #include <sched.h>
// #include <sys/stat.h>
// #include <sys/capability.h>
// 
#if defined(AI_RSYNC)
#include "../../gnulib/lib/md5.h"
#include "../../gnulib/lib/sha256.h"
#include "../../gnulib/lib/sha512.h"
#else
#include "../gnulib/lib/md5.h"
#include "../gnulib/lib/sha256.h"
#include "../gnulib/lib/sha512.h"
#endif /* AI_RSYNC */
#include "ai_cpri.h"
#include "ai_epri.h"

/* Includes }}} */

/* Global variables {{{ */

struct global_params         G                                          = { 0 };

unsigned char                ai_buffer[BUFFER_SIZE]                     = { 0 };
size_t                       ai_bytes_read                              = 0;

pthread_barrier_t            ai_start_barrier;
pthread_barrier_t            ai_end_barrier;

unsigned char                ai_digest_md5[MD5_DIGEST_SIZE]             = { 0 };
unsigned char                ai_digest_sha256[SHA256_DIGEST_SIZE]       = { 0 };
unsigned char                ai_digest_sha512[SHA512_DIGEST_SIZE]       = { 0 };

unsigned char                ai_md5[(2 * MD5_DIGEST_SIZE) + 1]          = { 0 };
unsigned char                ai_sha256[(2 * SHA256_DIGEST_SIZE) + 1]    = { 0 };
unsigned char                ai_sha512[(2 * SHA512_DIGEST_SIZE) + 1]    = { 0 };

pthread_t                    ai_t_md5,
                             ai_t_sha256,
                             ai_t_sha512;


int                          ai_core_md5                                 = 0,
                             ai_core_sha256                              = 1,
                             ai_core_sha512                              = 2;

int                          ai_checksum_target_fd                       = -1;

/* Array of xattr checksums descriptors */
struct ai_xattr_desc         ai_xattr_descs[] = {
    { AI_DATE,          AI_XATTR_DATE,          0,      FALSE },
    { AI_MD5,           AI_XATTR_MD5,           0,      TRUE  },
    { AI_SHA256,        AI_XATTR_SHA256,        0,      TRUE  },
    { AI_SHA512,        AI_XATTR_SHA512,        0,      TRUE  },
    { 0,                NULL,                   0,      FALSE }
};

/* Global variables }}} */
