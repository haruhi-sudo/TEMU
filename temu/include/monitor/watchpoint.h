#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
	int NO;
	struct watchpoint *next;

	/* TODO: Add more members if necessary */
	int isfree ; // 是否可用
	char expr[100]; // 表达式
	int expr_val; // 表达式的值
} WP;

void init_wp_pool();
WP *new_wp(char* expr);
void free_wp(int N);
WP* return_first();

#endif
