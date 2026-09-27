/*
 *  Various declarations for checksums in xattr
 *  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  @(#) [Zen] ai_epri.h   Version 1.12 du 26/09/22 - 
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

#if ! defined(_AI_EPRI_H)
#define _AI_EPRI_H

#include <pthread.h>
#include "../gnulib/lib/md5.h"
#include "../gnulib/lib/sha256.h"
#include "../gnulib/lib/sha512.h"
#include "ai_cpri.h"

/* Variable declarations {{{ */
extern struct global_params          G;

extern unsigned char                 ai_buffer[BUFFER_SIZE];
extern size_t                        ai_bytes_read;

extern pthread_barrier_t             ai_start_barrier;
extern pthread_barrier_t             ai_end_barrier;

extern unsigned char                 ai_digest_md5[MD5_DIGEST_SIZE];
extern unsigned char                 ai_digest_sha256[SHA256_DIGEST_SIZE];
extern unsigned char                 ai_digest_sha512[SHA512_DIGEST_SIZE];

extern unsigned char                 ai_md5[(2 * MD5_DIGEST_SIZE) + 1];
extern unsigned char                 ai_sha256[(2 * SHA256_DIGEST_SIZE) + 1];
extern unsigned char                 ai_sha512[(2 * SHA512_DIGEST_SIZE) + 1];

extern pthread_t                     ai_t_md5,
                                     ai_t_sha256,
                                     ai_t_sha512;

extern int                           ai_core_md5,
                                     ai_core_sha256,
                                     ai_core_sha512;

extern int                           ai_checksum_target_fd;

extern struct ai_xattr_desc          ai_xattr_descs[];

/* }}} */
/* Functions declarations {{{ */
void                                 ai_set_progname(char *progname);
BOOL                                 ai_has_cap_sys_admin(void);
void                                 ai_get_checksums(const char *pathname, ai_xattr_sums *ref_xattr_sums,
                                                      int *origin);
void                                 ai_setxattr_sums(const char *pathname, ai_xattr_sums *ref_xattr_sums);

void                                 ai_init_checksum_threads(void);
void                                 ai_process_checksum_bytes(const char *buf, size_t len);
void                                 ai_finalize_checksum_threads(void);

/* }}} */

#endif /* _AI_EPRI_H */
