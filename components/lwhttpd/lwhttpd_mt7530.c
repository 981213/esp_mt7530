#include <string.h>
#include <mt7530.h>
#include <cJSON.h>
#include "lwhttpd.h"

int httpd_mt7530_portstat(lwhttpd_f_entry *fentry)
{
    struct cJSON *root, *tarray, *tmp;
    struct switch_port_link plink;
    root = cJSON_CreateObject();
    tarray = cJSON_CreateArray();
    for(int i=0;i<5;i++) {
        tmp = cJSON_CreateObject();
        mt7530_get_port_link(i, &plink);
        cJSON_AddItemToObject(tmp, "port", cJSON_CreateNumber(i));
        cJSON_AddItemToObject(tmp, "link", cJSON_CreateBool(plink.link));
        cJSON_AddItemToObject(tmp, "duplex", cJSON_CreateBool(plink.duplex));
        cJSON_AddItemToObject(tmp, "speed", cJSON_CreateNumber(plink.speed));
        cJSON_AddItemToArray(tarray, tmp);
    }
    cJSON_AddItemToObject(root, "switch0", tarray);
    fentry->data = cJSON_PrintUnformatted(root);
    fentry->len = strlen((const char*) fentry->data);
    cJSON_Delete(root);
    return 0;
}