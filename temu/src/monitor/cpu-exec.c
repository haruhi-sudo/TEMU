#include "monitor.h"
#include "helper.h"
#include "expr.h"
#include "watchpoint.h"
/* The assembly code of instructions executed is only output to the screen
 * when the number of instructions executed is less than this value.
 * This is useful when you use the `si' command.
 * You can modify this value as you want.
 */
#define MAX_INSTR_TO_PRINT 10

int temu_state = STOP;

void exec(uint32_t);

char assembly[80];
char asm_buf[128];

void print_bin_instr(uint32_t pc) {
	int i;
	int l = sprintf(asm_buf, "%8x:   ", pc);
	for(i = 3; i >= 0; i --) {
		l += sprintf(asm_buf + l, "%02x ", instr_fetch(pc + i, 1));
	}
	sprintf(asm_buf + l, "%*.s", 8, "");
}

/* Simulate how the MiniMIPS32 CPU works. */
void cpu_exec(volatile uint32_t n) {
	if(temu_state == END) {
		printf("Program execution has ended. To restart the program, exit TEMU and run again.\n");
		return;
	}
	temu_state = RUNNING;

#ifdef DEBUG
	volatile uint32_t n_temp = n;
#endif

	for(; n > 0; n --) {
#ifdef DEBUG
		uint32_t pc_temp = cpu.pc;
		if((n & 0xffff) == 0) {
			
			fputc('.', stderr);
		}
#endif

		/* Execute one instruction, including instruction fetch,
		 * instruction decode, and the actual execution. */
		exec(cpu.pc);

		cpu.pc += 4;

#ifdef DEBUG
		print_bin_instr(pc_temp);
		strcat(asm_buf, assembly);
		Log_write("%s\n", asm_buf);
		if(n_temp < MAX_INSTR_TO_PRINT) {
			printf("%s\n", asm_buf);
		}
#endif

		/* TODO: check watchpoints here. */
		WP *p = return_first();
		while (p != NULL)
		{
			//Log("断点号%d, 表达式%s, 表达式的值%d\n", p->NO, p->expr, p->expr_val);
			//int new_expr_val = expr(p->expr, (bool*)1);
			if(p->expr_val != expr(p->expr, (bool*)1)){
				temu_state = STOP;
				printf("触发监视点%d: %s, 此时表达式的值为: %d\n", p->NO, p->expr, expr(p->expr, (bool*)1));
				p->expr_val = expr(p->expr, (bool*)1);
			}
			p = p->next;
		}

		if(temu_state != RUNNING) { return; }
	}

	if(temu_state == RUNNING) { temu_state = STOP; }
}
