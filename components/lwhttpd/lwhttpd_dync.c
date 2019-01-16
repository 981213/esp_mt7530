/*
 * dynamic content generating hooks
 * 
 * LwIP httpd doesn't have a hook for completely generated contents.
 * We hijack the fs functions to provide this ability.
 *
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "lwhttpd.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

int lw_hello_world(lwhttpd_f_entry *fentry)
{
    char *content = malloc(6);
    sprintf(content, "hello");
    fentry->data = content;
    fentry->len = strlen((char*) fentry->data);
    return 0;
}

static lwhttpd_dync_entry dync_entries[] = {
    {"/hello.txt", lw_hello_world},
};

int lwhttpd_dync_load(lwhttpd_f_entry *fentry, const char *name)
{
    int i;
	for (i = 0; i < ARRAY_SIZE(dync_entries); i++) {
		if (!strcmp(name, dync_entries[i].path)) {
			return (dync_entries[i].hook(fentry) != 0);
		}
	}
    return -EINVAL;
}