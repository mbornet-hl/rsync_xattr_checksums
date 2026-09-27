/*
 *  Simultaneous computation of MD5, SHA256 and SHA512 checksums of a file
 *  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  @(#) [Zen] ai_xattr.c   Version 1.14 du 26/09/26 - 
 *
 *  vim: ts=4 sw=4 et foldmethod=marker :
 *
 *  Needs GNUlib.
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
 *
 *  3 threads are created and are each attached to a CPU core.
 *  On 64 bits machines, MD5 run faster than SHA512, which run faster than SHA256.
 *  Computed checksums are stored in trusted extended attributes, if the users is
 *  allowed to access them.
 *
 * Remark: the prefix 'ai' you can see in the file names and in the source code has
 * no meaning, it's just after "ah' and before "aj". There's no artificial intelligence
 * there.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1  /* Needed for pthread_setaffinity_np and sched_getcpu */
#endif /* ! _GNU_SOURCE */

/* Includes {{{ */
#if defined(AI_RSYNC)
#include "../../gnulib.build/config.h"
#else
#include "../gnulib-build/config.h"
#endif /* AI_RSYNC */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <sys/capability.h>

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

/* ai_set_progname() {{{ */

/******************************************************************************

                AI_SET_PROGNAME

******************************************************************************/
void ai_set_progname(char *progname)
{
    G.progname      = progname;
}

/* ai_set_progname() }}} */
/* ai_ts_to_date() {{{ */
/******************************************************************************

                AI_TS_TO_DATE

******************************************************************************/
char *ai_ts_to_date(time_t ts)
{
    struct tm                _tm;
    char                     _buf[20];

    localtime_r(&ts, &_tm);
    strftime(_buf, sizeof(_buf), "%Y-%m-%d %H:%M:%S", &_tm);

    return strdup(_buf);
}

/* ai_ts_to_date() }}} */
/* ai has_cap_sys_admin() {{{ */

/******************************************************************************

                AI_HAS_CAP_SYS_ADMIN

******************************************************************************/
BOOL ai_has_cap_sys_admin(void)
{
    cap_t                    _caps;
    cap_flag_value_t         _value;

    _caps           = cap_get_proc();
    if (!_caps) {
        return FALSE;
    }

    if (cap_get_flag(_caps, CAP_SYS_ADMIN, CAP_EFFECTIVE, &_value) == -1) {
        cap_free(_caps);
        return FALSE;
    }

    cap_free(_caps);
    return (_value == CAP_SET);
}
/* ai has_cap_sys_admin() }}} */
/* ai_set_cpu_affinity() {{{ */
/* Function that attaches a thread to a specific CPU core */
/******************************************************************************

                AI_SET_CPU_AFFINITY

******************************************************************************/
static void ai_set_cpu_affinity(int core_id)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

/* ai_set_cpu_affinity() }}} */
/* ai_md5_worker() {{{ */
/******************************************************************************

                AI_MD5_WORKER

******************************************************************************/
static void *ai_md5_worker(void *arg)
{
    int core_id = *(int *)arg;
    ai_set_cpu_affinity(core_id);
//    printf("[MD5 Worker] Started on CPU core %d (real: %d)\n", core_id, sched_getcpu());

    struct md5_ctx ctx;
    md5_init_ctx(&ctx);

    while (1) {
        pthread_barrier_wait(&ai_start_barrier);
        if (ai_bytes_read == 0) break;
        md5_process_bytes(ai_buffer, ai_bytes_read, &ctx);
        pthread_barrier_wait(&ai_end_barrier);
    }

    md5_finish_ctx(&ctx, ai_digest_md5);
    return NULL;
}

/* ai_md5_worker() }}} */
/* ai_sha256_worker() {{{ */
/******************************************************************************

                AI_SHA256_WORKER

******************************************************************************/
static void *ai_sha256_worker(void *arg)
{
    int core_id = *(int *)arg;
    ai_set_cpu_affinity(core_id);
//    printf("[SHA256 Worker] Started on CPU core %d (real: %d)\n", core_id, sched_getcpu());

    struct sha256_ctx ctx;
    sha256_init_ctx(&ctx);

    while (1) {
        pthread_barrier_wait(&ai_start_barrier);
        if (ai_bytes_read == 0) break;
        sha256_process_bytes(ai_buffer, ai_bytes_read, &ctx);
        pthread_barrier_wait(&ai_end_barrier);
    }

    sha256_finish_ctx(&ctx, ai_digest_sha256);
    return NULL;
}

/* ai_sha256_worker() }}} */
/* ai_sha512_worker() {{{ */
/******************************************************************************

                AI_SHA512_WORKER

******************************************************************************/
static void *ai_sha512_worker(void *arg)
{
    int core_id = *(int *)arg;
    ai_set_cpu_affinity(core_id);
//    printf("[SHA512 Worker] Started on CPU core %d (real: %d)\n", core_id, sched_getcpu());

    struct sha512_ctx ctx;
    sha512_init_ctx(&ctx);

    while (1) {
        pthread_barrier_wait(&ai_start_barrier);
        if (ai_bytes_read == 0) break;
        sha512_process_bytes(ai_buffer, ai_bytes_read, &ctx);
        pthread_barrier_wait(&ai_end_barrier);
    }

    sha512_finish_ctx(&ctx, ai_digest_sha512);
    return NULL;
}

/* ai_sha512_worker() }}} */
/* ai_sprint_hex() {{{ */
/******************************************************************************

                AI_SPRINT_HEX

******************************************************************************/
static void ai_sprint_hex(unsigned char *dest, const unsigned char *digest, size_t len)
{
    unsigned int         _offset = 0;

    for (size_t i = 0; i < len; i++) {
        _offset             += sprintf((char *)( dest + _offset), "%02x", digest[i]);
    }
    dest[_offset]       = '\0';
}

/* ai_sprint_hex() }}} */
/* ai_init_checksum_threads() {{{ */

/******************************************************************************

                AI_INIT_CHECKSUM_THREADS

******************************************************************************/
void ai_init_checksum_threads(void)
{
    pthread_barrier_init(&ai_start_barrier, NULL, 4);
    pthread_barrier_init(&ai_end_barrier,   NULL, 4);

    pthread_create(&ai_t_md5,    NULL, ai_md5_worker,    &ai_core_md5);
    pthread_create(&ai_t_sha256, NULL, ai_sha256_worker, &ai_core_sha256);
    pthread_create(&ai_t_sha512, NULL, ai_sha512_worker, &ai_core_sha512);
}

/* ai_init_checksum_threads() }}} */
/* ai_process_checksum_bytes() {{{ */

/******************************************************************************

                AI_PROCESS_CHECKSUM_BYTES

******************************************************************************/
void ai_process_checksum_bytes(const char *buf, size_t len)
{
    size_t                   offset = 0;

    while (offset < len) {
        size_t chunk = (len - offset > BUFFER_SIZE) ? BUFFER_SIZE : (len - offset);
        
        if (buf != NULL) {
            memcpy(ai_buffer, buf + offset, chunk);
        }
        else {
            /* If buf is NULL, inject nul bytes (0x00) */
            memset(ai_buffer, 0, chunk);
        }
        ai_bytes_read       = chunk;

        pthread_barrier_wait(&ai_start_barrier);
        pthread_barrier_wait(&ai_end_barrier);

        offset              += chunk;
    }
}
/* ai_process_checksum_bytes() }}} */
/* ai_finalize_checksum_threads() {{{ */

/******************************************************************************

                AI_FINALIZE_CHECKSUM_THREADS

******************************************************************************/
void ai_finalize_checksum_threads()
{
    /* End-of-file indicator (ai_bytes_read = 0) */
    ai_bytes_read           = 0;
    pthread_barrier_wait(&ai_start_barrier);

    pthread_join(ai_t_md5,    NULL);
    pthread_join(ai_t_sha256, NULL);
    pthread_join(ai_t_sha512, NULL);

    pthread_barrier_destroy(&ai_start_barrier);
    pthread_barrier_destroy(&ai_end_barrier);

    /* Formats raw fingerprints into hexadecimal strings */
    ai_sprint_hex(ai_md5,    ai_digest_md5,    MD5_DIGEST_SIZE);
    ai_sprint_hex(ai_sha256, ai_digest_sha256, SHA256_DIGEST_SIZE);
    ai_sprint_hex(ai_sha512, ai_digest_sha512, SHA512_DIGEST_SIZE);
}
/* ai_finalize_checksum_threads() }}} */
/* ai_setxattr() {{{ */

/******************************************************************************

					AI_SETXATTR

******************************************************************************/
void ai_setxattr(const char *pathname, int fileno, unsigned char *tag, unsigned char *val)
{
	int              _flags;
	char			 _tag[128], *_namespace;
	size_t           _size;

	_namespace      = AI_NAMESPACE;
	sprintf(_tag, "%s.%s", _namespace, tag);

	_flags          = XATTR_CREATE;
	_size           = strlen((char *)val) + 1;

	if (fsetxattr(fileno, _tag, val, _size, _flags) == -1) {
		if (errno == EEXIST) {
			_flags		= XATTR_REPLACE;
			if (fsetxattr(fileno, _tag, val, _size, _flags) == -1) {
                perror("fsetxattr");
                fprintf(stderr, "%s: cannot execute fsetxattr(REPLACE, %s, %s) for \"%s\" !\n",
                        G.progname, _tag, val, pathname);
			}
			else {
                if (G.debug) {
                    fprintf(stderr, "%s: fsetxattr(REPLACE, %s, %s) for \"%s\" : OK\n",
                            G.progname, _tag, val, pathname);
                }
			}
		}
		else {
            perror("fsetxattr");
            fprintf(stderr, "%s: fsetxattr(CREATE,  %s, %s) for \"%s\" : OK\n",
                    G.progname, _tag, val, pathname);
		}
	}
	else {
        if (G.debug) {
            fprintf(stderr, "%s: fsetxattr(CREATE,  %s, %s) for \"%s\" : OK\n",
                    G.progname, _tag, val, pathname);
        }
	}
}

/* ai_setxattr() }}} */
/* ai_compute_checksums() {{{ */
/******************************************************************************

                AI_COMPUTE_CHECKSUMS

******************************************************************************/
int ai_compute_checksums(const char *pathname)
{
    FILE                *_fp;
    size_t               bytes_read;

    int                  _fd, _flags, _errno;

#if defined(_GNU_SOURCE)
    _flags              = O_RDONLY | O_NOATIME;
#else
    _flags              = O_RDONLY;
#endif

    /* Open pathname without updating access time */
    if ((_fd = open(pathname, _flags)) < 0) {
        _errno              = errno;
        fprintf(stderr, "%s: cannot open \"%s\" (%s)!\n",
                G.progname, pathname, strerror(_errno));
        return AI_RET_ERR_OPEN;
    }

    _fp                 = fdopen(_fd, "r");
    if (!_fp) {
       fprintf(stderr, "%s: cannot open \"%s\" !\n", G.progname, pathname);
        perror("fopen");
        return AI_RET_ERR_FDOPEN;
    }

    pthread_barrier_init(&ai_start_barrier, NULL, 4);
    pthread_barrier_init(&ai_end_barrier,   NULL, 4);

    pthread_create(&ai_t_md5,    NULL, ai_md5_worker,    &ai_core_md5);
    pthread_create(&ai_t_sha256, NULL, ai_sha256_worker, &ai_core_sha256);
    pthread_create(&ai_t_sha512, NULL, ai_sha512_worker, &ai_core_sha512);

    while ((bytes_read = fread(ai_buffer, 1, sizeof(ai_buffer), _fp)) > 0) {
        ai_bytes_read = bytes_read;
        pthread_barrier_wait(&ai_start_barrier);
        pthread_barrier_wait(&ai_end_barrier);
    }

    /* Indicate an End Of File (ai_bytes_read = 0) */
    ai_bytes_read = 0;
    pthread_barrier_wait(&ai_start_barrier); 

    /* Wait for threads termination */
    pthread_join(ai_t_md5, NULL);
    pthread_join(ai_t_sha256, NULL);
    pthread_join(ai_t_sha512, NULL);

    pthread_barrier_destroy(&ai_start_barrier);
    pthread_barrier_destroy(&ai_end_barrier);

    if (fclose(_fp) != 0) {
        fprintf(stderr, "%s: fclose() error !\n", G.progname);
        return AI_RET_ERR_FCLOSE;
    }

    return AI_RET_OK;
}

/* ai_compute_checksums() }}} */
/* ai_setxattr_sums() {{{ */

/******************************************************************************

                AI_SETXATTR_SUMS

******************************************************************************/
void ai_setxattr_sums(const char *pathname, ai_xattr_sums *ref_xattr_sums)
{
    unsigned char            *_tag, *_val, _ts_val[16];
    int                      _fd, _flags, _errno, _max_retries;
    time_t                   _time;
    BOOL                     _good = FALSE;
    struct stat              _stat;

#if defined(_GNU_SOURCE)
    _flags              = O_RDONLY | O_NOATIME;
#else
    _flags              = O_RDONLY;
#endif

    /* Open pathname without updating access time */
    if ((_fd = open(pathname, _flags)) < 0) {
        _errno              = errno;
        fprintf(stderr, "%s: cannot open \"%s\" (%s)!\n",
                G.progname, pathname, strerror(_errno));
        return;
    }

	// Set MD5 sum xattr {{{
	_tag            = AI_XATTR_MD5;
	_val    		= ref_xattr_sums->MD5;
	ai_setxattr(pathname, _fd, _tag, _val);

	// }}}
	// Set SHA256 sum xattr {{{
	_tag            = AI_XATTR_SHA256;
	_val            = ref_xattr_sums->SHA256;
	ai_setxattr(pathname, _fd, _tag, _val);

	// }}}
	// Set SHA512 sum xattr {{{
	_tag            = AI_XATTR_SHA512;
	_val            = ref_xattr_sums->SHA512;
	ai_setxattr(pathname, _fd, _tag, _val);

	// }}}
    // Set sum generation date xattr {{{
    for (_max_retries = 10; !_good && _max_retries > 0; _max_retries--) {
        _time		= time(0);
        sprintf((char *)_ts_val, "%lu", _time);
        _tag			= AI_XATTR_DATE;
        _val			= _ts_val;
        ai_setxattr(pathname, _fd, _tag, _val);

        if (fstat(_fd, &_stat) < 0) {
            _errno			= errno;
            fprintf(stderr, "%s: cannot fstat \"%s\" (%s) !\n",
                     G.progname, pathname, strerror(_errno));
            return;
        }

        if (_time == _stat.st_ctime) {
            /* Checksum timestamp is consistent with i-node change */
            _good		= TRUE;

            if (G.debug) {
                fprintf(stderr, "%s: [DEBUG] checksum timestamp consistent with i-node for \"%s\"\n",
                        G.progname, pathname);
            }
        }
        else {
            if (G.debug) {
                fprintf(stderr, "%s: [DEBUG] checksum timestamp is NOT consistent with i-node for \"%s\"\n",
                         G.progname, pathname);
                fprintf(stderr, "%lu != %lu\n", _time, _stat.st_ctime);
            }
        }
    }
    // }}}

    close(_fd);
}

/* ai_setxattr_sums() }}} */
/* ai_getxattr_sums() {{{ */

/******************************************************************************

                AI_GETXATTR_SUMS

******************************************************************************/
ai_retcode ai_getxattr_sums(const char *pathname)
{
    enum ai_retcode          _retcode;
    struct stat              _stat;
    char                     _tag[128], *_namespace, _val[256], _ctime[16];
    struct ai_xattr_desc    *_desc;
    int                      _checksums_count = 0, _errno, _lg;
    BOOL                     _up_to_date = FALSE;

    _namespace  = AI_NAMESPACE;

    if ((stat(pathname, &_stat)) < 0) {
        fprintf(stderr, "%s: cannot stat \"%s\" !\n", G.progname, pathname);
        return AI_RET_ERR_STAT;
    }

    sprintf(_ctime, "%lu", _stat.st_ctime);

    _retcode                = AI_RET_OK;
    for (_desc = ai_xattr_descs; (_retcode == AI_RET_OK) && (_desc->xattr_name != 0); _desc++) {
        /* Generate the tag name */
        sprintf(_tag, "%s.%s", _namespace, _desc->xattr_name);

        /* Try to get the tag from xattr */
        if ((_lg = getxattr(pathname, _tag, _val, sizeof(_val) - 1)) == -1) {
            _errno              = errno;

            if (G.debug) {
                fprintf(stderr, "%-*s : [DEBUG] tag [%-*s] NOT FOUND in xattr\n",
                        G.path_width, pathname, G.tag_width, _tag);
//                fprintf(stderr, "errno = %d (%s)\n", _errno, strerror(_errno));
            }

            switch (_errno) {

            case ENODATA:
                /* Not available in xattr */
                _retcode            = AI_RET_ERR_XATTR_NO_SUMS;
                break;

            case ENOTSUP:
                /* Not supported */
                _retcode            = AI_RET_ERR_UNSUPPORTED;
                break;

            case ERANGE:
                /* Buffer too small : INTERNAL ERROR */
                fprintf(stderr, "%s() : INTERNAL ERROR\n", __func__);
                exit(AI_EXIT_ERR_INTERNAL);

            case EIO:
                /* I/O error */
                fprintf(stderr, "%s() : Input / output error on \"%s\" !\n",
                        G.progname, pathname);
                _retcode            = AI_RET_ERR_IO_ERROR;
                break;

            default:
                /* Unknown retcode */
                fprintf(stderr, "%s() : unknown retcode (%d)\n", __func__, _errno);
                exit(AI_EXIT_ERR_INTERNAL);
            }
        }
        else {
            _val[_lg]           = '\0';

            if (G.debug) {
                fprintf(stderr, "%-*s : [DEBUG] tag [%-*s] found in xattr\n",
                        G.path_width, pathname, G.tag_width, _tag);
            }

            /* Count checksums */
            if (_desc->is_checksum) {
                _checksums_count++;
            }

            /* Tag has been retrieved from xattr */
            if (_desc->chksum_type == AI_DATE) {
                /* Check timestamp consistency */
                if (strcmp(_val, _ctime)) {
                    /* Timestamps are inconsistent */
                    if (G.debug) {
                        fprintf(stderr, "%-*s : [DEBUG] timestamps are NOT CONSISTENT (%s != %s)\n",
                                G.path_width, pathname, _val, _ctime);
                        fprintf(stderr, "%-*s : [DEBUG] %s (xattr) != %s (ctime)\n",
                                G.path_width, pathname,
                                ai_ts_to_date(atol(_val)), ai_ts_to_date(_stat.st_ctime));
                    }
                    _up_to_date     = FALSE;
                }
                else {
                    if (G.debug) {
                        fprintf(stderr, "%-*s : [DEBUG] timestamps are consistent\n",
                                G.path_width, pathname);
                    }
                    _up_to_date     = TRUE;
                }
            }

            /* Copy the value of the xattr into the descriptor */
            _desc->xattr_value      = (unsigned char *) strdup(_val);
        }
    }

    /* Prepare the return code */
    if (_checksums_count == 0) {
        /* No checksum found in xattr */
        if (G.debug) {
            fprintf(stderr, "%-*s : [DEBUG] no checksums in xattr\n", G.path_width, pathname);
        }
        _retcode    = AI_RET_ERR_XATTR_NO_SUMS;
    }
    else if (_checksums_count < AI_CHECKSUMS_NR) {
        /* Missing checksums */
        if (G.debug) {
            fprintf(stderr, "%-*s : [DEBUG] missing checksums in xattr\n", G.path_width, pathname);
        }
        _retcode    = AI_RET_ERR_XATTR_MISSING_SUMS;
    }
    else if (_up_to_date == TRUE) {
        if (G.debug) {
            fprintf(stderr, "%-*s : [DEBUG] all checksums are in xattr\n", G.path_width, pathname);
        }
        _retcode    = AI_RET_OK;
    }
    else {
        if (G.debug) {
            fprintf(stderr, "%-*s : [DEBUG] xattr need update\n", G.path_width, pathname);
        }
        _retcode    = AI_RET_ERR_XATTR_NEEDS_UPDATE;
    }

    return _retcode;
}

/* ai_getxattr_sums() }}} */
/* ai_print_hex() {{{ */
/******************************************************************************

                AI_PRINT_HEX

******************************************************************************/
void ai_print_hex(const unsigned char *digest, size_t len)
{
    unsigned int         _offset = 0;

    for (size_t i = 0; i < len; i++) {
        _offset             += printf("%02x", digest[i]);
    }
}

/* ai_print_hex() }}} */
/* ai_get_checksums() {{{ */
/******************************************************************************

                AI_GET_CHECKSUMS

******************************************************************************/
void ai_get_checksums(const char *pathname, ai_xattr_sums *ref_xattr_sums, int *origin)
{
    struct stat              _stat;
    int                      _retcode = TRUE;
    BOOL                     _do_compute = TRUE;    // Compute checksums by default
    struct ai_xattr_desc    *_desc;

    /* Capability CAP_SYS_ADMIN is needed to access trusted attributes */
    if (ai_has_cap_sys_admin()) {
// fprintf(stderr, "[DEBUG] process has CAP_SYS_ADMIN\n");
        /* Get i-node modification timestamp */
        if ((stat(pathname, &_stat)) < 0) {
            fprintf(stderr, "%s: cannot stat \"%s\" !\n", G.progname, pathname);
            return;
        }

        /* Try to get checksums from file's xattrs */
        _retcode        = ai_getxattr_sums(pathname);

        switch (_retcode) {

        case AI_RET_OK:
            if (G.debug) {
                fprintf(stderr, "%-*s : [DEBUG] checksums are retrieved from xattr\n",
                       G.path_width, pathname);
            }
            *origin         = AI_ORIGIN_XATTR;

            if (G.list_OK || G.list_status) {
                printf("%-*s", G.path_width, pathname);
                if (G.verbose || G.list_status) {
                    printf(" : all xattr are OK");
                }
                putchar('\n');
            }

            /* No need to compute checksums, all of them are known */
            _do_compute     = FALSE;

            /* Copy values from xattr
               ~~~~~~~~~~~~~~~~~~~~~~ */
            for (_desc = ai_xattr_descs; _desc->xattr_name != 0; _desc++) {
                switch (_desc->chksum_type) {

                case AI_DATE:
                    ref_xattr_sums->chksum_ts_str   = _desc->xattr_value;
                    break;

                case AI_MD5:
                    ref_xattr_sums->MD5             = _desc->xattr_value;
                    break;

                case AI_SHA256:
                    ref_xattr_sums->SHA256          = _desc->xattr_value;
                    break;

                case AI_SHA512:
                    ref_xattr_sums->SHA512          = _desc->xattr_value;
                    break;

                default:
                    fprintf(stderr, "INTERNAL ERROR\n");
                    exit(AI_EXIT_ERR_INTERNAL);
                    break;
                }
            }
            break;

        case AI_RET_ERR_XATTR_NO_SUMS:
            if (G.disp_name_only) {
                printf("%s\n", pathname);
            }
            else if (G.list_none || G.list_needs_sums || G.list_status) {
                printf("%-*s", G.path_width, pathname);
                if (G.verbose || G.list_status) {
                    printf(" : no checksums in xattr");
                }
                putchar('\n');
                _do_compute     = FALSE;
            }
            else if (G.list) {
                /* Another type of list has been selected */
                _do_compute     = FALSE;
            }
            else {
                /* Checksums need to be computed */
                _do_compute     = TRUE;
            }
            break;

        case AI_RET_ERR_XATTR_MISSING_SUMS:
            if (G.disp_name_only) {
                printf("%s\n", pathname);
            }
            else if (G.list_missing || G.list_needs_sums || G.list_status) {
                printf("%-*s", G.path_width, pathname);
                if (G.verbose || G.list_status) {
                    printf(" : missing checksums in xattr");
                }
                putchar('\n');
                _do_compute     = FALSE;
            }
            else if (G.list) {
                /* Another type of list has been selected */
                _do_compute     = FALSE;
            }
            else {
                /* Checksums need to be computed */
                _do_compute     = TRUE;
            }
            break;

        case AI_RET_ERR_XATTR_NEEDS_UPDATE:
            if (G.disp_name_only) {
                printf("%s\n", pathname);
            }
            else if (G.list_needs_update || G.list_needs_sums || G.list_status) {
                printf("%-*s", G.path_width, pathname);
                if (G.verbose || G.list_status) {
                    printf(" : needs timestamp update");
                }
                putchar('\n');
                _do_compute     = FALSE;
            }
            else if (G.list) {
                /* Another type of list has been selected */
                _do_compute     = FALSE;
            }
            else {
                /* Checksums need to be computed */
                _do_compute     = TRUE;
            }
            break;

        default:
            fprintf(stderr, "%s: INTERNAL ERROR\n", G.progname);
            exit(AI_EXIT_ERR_INTERNAL);
            break;
        }
    }
    else {
// fprintf(stderr, "[DEBUG] process DOES NOT have CAP_SYS_ADMIN\n");
        /* Unprivileged user : unable to access trusted xattr */
        _do_compute     = TRUE;
    }

    if (G.force_checksums) {
        _do_compute         = TRUE;
    }

    if (_do_compute) {
        if (G.debug) {
            fprintf(stderr, "%-*s : [DEBUG] checksums are computed\n",
                   G.path_width, pathname);
        }
        *origin         = AI_ORIGIN_COMPUTED;

        if (ai_compute_checksums(pathname) != 0) {
            fprintf(stderr, "%s: error for \"%s\" !\n", G.progname, pathname);
        }

        ai_sprint_hex(ai_md5,    ai_digest_md5,    MD5_DIGEST_SIZE);
        ai_sprint_hex(ai_sha256, ai_digest_sha256, SHA256_DIGEST_SIZE);
        ai_sprint_hex(ai_sha512, ai_digest_sha512, SHA512_DIGEST_SIZE);

        ref_xattr_sums->MD5     = ai_md5;
        ref_xattr_sums->SHA256  = ai_sha256;
        ref_xattr_sums->SHA512  = ai_sha512;

        if (ai_has_cap_sys_admin()) {
            /* Store checksums in xattr */
            ai_setxattr_sums(pathname, ref_xattr_sums);
        }
    }
}

/* ai_get_checksums() }}} */
