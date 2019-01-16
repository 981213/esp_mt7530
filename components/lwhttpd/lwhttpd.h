#ifndef _LWHTTPD_H_
#define _LWHTTPD_H_
#include <lwip/apps/fs.h>
#include <spiffs.h>

typedef struct _LWHTTPD_F_ENTR_ {
	void *data;
	size_t len;
	spiffs_file fdp;
} lwhttpd_f_entry;

typedef struct _LWHTTPD_DYNC_ENTR_ {
	const char *path;
	int (*hook)(lwhttpd_f_entry *fentry);
} lwhttpd_dync_entry;

int lwhttpd_dync_load(lwhttpd_f_entry *fentry, const char *name);

#endif // _LWHTTPD_H_
