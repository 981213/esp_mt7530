/*
 * dynamic content generating hooks
 * 
 * LwIP httpd doesn't have a hook for completely generated contents.
 * We hijack the fs functions to provide this ability.
 *
 */

#include <errno.h>
#include <lwip/apps/fs.h>
#include "lwhttpd_dync.h"

int lwhttpd_dync_load(struct fs_file *file, const char *name)
{
    return -EINVAL;
}