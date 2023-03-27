#include "mongoose.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static void httpd_cb(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
    if (ev == MG_EV_HTTP_MSG) {
        mg_http_reply(c, 200, "Content-Type: text/plain\r\n", "Hello, %s\n", "world");
    }
}

static void mongoose_task(void *pvParameters) {
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    mg_http_listen(&mgr, "http://0.0.0.0", httpd_cb, &mgr);
    for (;;) mg_mgr_poll(&mgr, 1000);
    mg_mgr_free(&mgr);
}

void mongoose_start(void) {
    xTaskCreate(mongoose_task, "mongoose_task", 4096, NULL, 14, NULL);
}