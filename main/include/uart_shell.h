#ifndef _UART_SHELL_H
#define _UART_SHELL_H
#define EX_UART_NUM UART_NUM_0
#define BUF_SIZE (512)
#define RD_BUF_SIZE (BUF_SIZE)
#define SH_MAX_ARGC 10
#define CMD_CNT 6

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

typedef struct _UART_CMD_ENTR {
	char *cmd;
	char *desc;
	int (*func_main)(int argc, char** argv);
} uart_cmd_entry;
extern const uart_cmd_entry uart_cmds[CMD_CNT];

void uart_shell_init(void);
void uart_cmd_exec(int argc, char **argv);

#endif