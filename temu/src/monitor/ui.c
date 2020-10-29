#include "monitor.h"
#include "temu.h"
#include "reg.h"
#include "helper.h"
#include "memory.h"
#include "expr.h"
#include "watchpoint.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>

void cpu_exec(uint32_t);

void display_reg();

/* We use the `readline' library to provide more flexibility to read from stdin. */
char *rl_gets()
{
	static char *line_read = NULL;

	if (line_read)
	{
		free(line_read);
		line_read = NULL;
	}

	line_read = readline("(temu) ");

	if (line_read && *line_read)
	{
		add_history(line_read);
	}

	return line_read;
}

static int cmd_c(char *args)
{
	cpu_exec(-1);
	return 0;
}

static int cmd_q(char *args)
{
	return -1;
}

// 监视点
static int cmd_w(char *args)
{
	new_wp(args);
	return 1;
}

// 监视点
static int cmd_dw(char *args)
{
	free_wp(atoi(args));
	return 1;
}

static int cmd_help(char *args);
static int cmd_si(char *args);
static int cmd_info(char *args);
static int cmd_p(char *args);
static int cmd_mr(char *args);

static struct
{
	char *name;
	char *description;
	int (*handler)(char *);
} cmd_table[] = {
	{"help", "Display informations about all supported commands", cmd_help},
	{"c", "Continue the execution of the program", cmd_c},
	{"q", "Exit TEMU", cmd_q},
	{"si", "单步执行", cmd_si},
	{"info", "打印寄存器和监视点状态", cmd_info},
	{"p", "表达式求值", cmd_p},
	{"x", "读取内存", cmd_mr},
	{"w", "监视点", cmd_w},
	{"d", "删除监视点", cmd_dw}
	/* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args)
{
	/* extract the first argument */
	char *arg = strtok(NULL, " ");
	int i;

	if (arg == NULL)
	{
		/* no argument given */
		for (i = 0; i < NR_CMD; i++)
		{
			printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
		}
	}
	else
	{
		for (i = 0; i < NR_CMD; i++)
		{
			if (strcmp(arg, cmd_table[i].name) == 0)
			{
				printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
				return 0;
			}
		}
		printf("Unknown command '%s'\n", arg);
	}
	return 0;
}

// 单步执行
static int cmd_si(char *args)
{
	if (args == NULL)
	{
		cpu_exec(1);
		return 0;
	}

	int n = atoi(args);
	cpu_exec(n);
	return 1;
}
// 打印寄存器状态
static int cmd_info(char *args)
{
	if (args == NULL)
		return 0;
	else if (args[0] == 'r')
	{
		display_reg();
		return 1;
	}
	else if (args[0] == 'w')
	{
		WP *p = return_first();
		while (p != NULL)
		{
			Log("断点号%d, 表达式%s, 表达式的值%d\n", p->NO, p->expr, p->expr_val);
			p = p->next;
		}
		return 1;
	}
	else
		return -1;
}
// 打印表达式的值
static int cmd_p(char *args)
{
	printf("%d\n", expr(args, (bool *)1));
	return 1;
}
// 读取内存
static int cmd_mr(char *args)
{
	int i = 0;
	for (;; i++)
	{
		if (args[i] == ' ')
			break;
	}

	int nbyte = atoi(args);
	uint32_t addr = expr(args + i + 1, (bool *)1);

	for (i = 0; i < nbyte; i++)
	{
		printf("%#x:                                           %#x\n", addr + i * 4, mem_read(addr + i * 4, 4));
	}
	return 1;
}

void ui_mainloop()
{
	init_wp_pool();
	while (1)
	{
		char *str = rl_gets();
		char *str_end = str + strlen(str);

		/* extract the first token as the command */
		char *cmd = strtok(str, " ");
		if (cmd == NULL)
		{
			continue;
		}

		/* treat the remaining string as the arguments,
		 * which may need further parsing
		 */
		char *args = cmd + strlen(cmd) + 1;
		if (args >= str_end)
		{
			args = NULL;
		}

		int i;
		for (i = 0; i < NR_CMD; i++)
		{
			if (strcmp(cmd, cmd_table[i].name) == 0)
			{
				if (cmd_table[i].handler(args) < 0)
				{
					return;
				}
				break;
			}
		}

		if (i == NR_CMD)
		{
			printf("Unknown command '%s'\n", cmd);
		}
	}
}
