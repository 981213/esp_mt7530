#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_system.h"
#include "uart_shell.h"

static const char *log_tag = "uart_shell";
static QueueHandle_t uart_queue;
static uart_config_t uart_config = { .baud_rate = CONFIG_CONSOLE_UART_BAUDRATE,
				     .data_bits = UART_DATA_8_BITS,
				     .parity = UART_PARITY_DISABLE,
				     .stop_bits = UART_STOP_BITS_1,
				     .flow_ctrl = UART_HW_FLOWCTRL_DISABLE };

static char uart_getc()
{
	char ret = 0;
	uart_read_bytes(EX_UART_NUM, (uint8_t *)&ret, 1, portMAX_DELAY);
	return ret;
}

static void uart_cmd_split(char *argstr)
{
	static char *cmd_args[SH_MAX_ARGC];
	int chptr;
	int cmd_argc = 0;
	bool cur_ifgraph = 0;
	for (chptr = 0; argstr[chptr]; chptr++) {
		if (cur_ifgraph) {
			if (!isgraph((int)argstr[chptr])) {
				cur_ifgraph = 0;
				argstr[chptr] = 0;
			}
		} else {
			if (isgraph((int)argstr[chptr])) {
				if (cmd_argc >= SH_MAX_ARGC) {
					ESP_LOGE(log_tag,
						 "Too many arguments.");
					return;
				}
				cmd_args[cmd_argc++] = argstr + chptr;
				cur_ifgraph = 1;
			}
		}
	}
	if (cmd_argc > 0)
		uart_cmd_exec(cmd_argc, cmd_args);
}

static void uart_cmd_append(size_t cmd_wait_len)
{
	static char cmd_buffer[RD_BUF_SIZE];
	static size_t cmd_len;
	char ch;

	for (int i = 0; i < cmd_wait_len; i++) {
		ch = uart_getc();
		switch (ch) {
		case '\r':
			putchar('\n');
			cmd_buffer[cmd_len++] = 0;
			if (cmd_len > 1)
				uart_cmd_split(cmd_buffer);
			printf("esp_cmd>");
			cmd_len = 0;
			break;
		case '\b':
			if (cmd_len > 0) {
				putchar('\b');
				cmd_len--;
			}
			break;
		default:
			if (!isprint((int)ch))
				break;
			if (cmd_len >= RD_BUF_SIZE) {
				ESP_LOGE(log_tag, "Command too long.");
				cmd_len = 0;
			} else {
				putchar(ch);
				cmd_buffer[cmd_len++] = ch;
			}
			break;
		}
	}
}

static void uart_shell_task(void *pvParameters)
{
	uart_event_t event;
	printf("\nesp_cmd>");
	while (1) {
		// Waiting for UART event.
		if (xQueueReceive(uart_queue, (void *)&event,
				  (portTickType)portMAX_DELAY)) {
			switch (event.type) {
			case UART_DATA:
				uart_cmd_append(event.size);
				break;

			case UART_FIFO_OVF:
				ESP_LOGI(log_tag, "uart fifo overflow.");
				uart_flush_input(EX_UART_NUM);
				xQueueReset(uart_queue);
				break;

			case UART_BUFFER_FULL:
				ESP_LOGI(log_tag, "ring buffer full.");
				uart_flush_input(EX_UART_NUM);
				xQueueReset(uart_queue);
				break;

			case UART_PARITY_ERR:
				ESP_LOGI(log_tag, "uart parity error.");
				break;

			case UART_FRAME_ERR:
				ESP_LOGI(log_tag, "uart frame error.");
				break;

			default:
				ESP_LOGI(log_tag, "uart event: %d", event.type);
				break;
			}
		}
	}

	vTaskDelete(NULL);
}

void uart_shell_init(void)
{
	setbuf(stdout, NULL);
	uart_param_config(EX_UART_NUM, &uart_config);
	uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 100,
			    &uart_queue, 0);
	xTaskCreate(uart_shell_task, "uart_shell_task", 2048, NULL, 14, NULL);
}
