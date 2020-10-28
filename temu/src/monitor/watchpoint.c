#include "watchpoint.h"
#include "expr.h"
#include <stdlib.h>

#define NR_WP 32

static WP *head, *free_;
static WP wp_pool[NR_WP];



void init_wp_pool()
{
	int i;
	for (i = 0; i < NR_WP; i++)
	{
		wp_pool[i].NO = i;
		wp_pool[i].next = &wp_pool[i + 1];
		wp_pool[i].isfree = 1;
	}
	wp_pool[NR_WP - 1].next = NULL;
	head = malloc(sizeof(WP));
	head->next = NULL;
	head->NO = -1;
	free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
WP *new_wp(char *expression)
{	
	int i;
	for (i = 0; i < NR_WP; i++)
	{
		if(wp_pool[i].isfree == 1)
		{
			free_ = wp_pool + i;
			break;
		} 
	}
	if (i == NR_WP)
		assert(0);

	WP* wp_new = malloc(sizeof(WP));
	strcpy(wp_new->expr, expression);
	wp_new->NO = free_->NO;
	wp_pool[free_->NO].isfree = 0;
	wp_new->next = head->next;
	head->next = wp_new;
	
	wp_new->expr_val = expr(expression, (bool*)1);
	
	return head;
}

void free_wp(int N)
{
	wp_pool[N].isfree = 1; 
	WP* pre = head;
	WP* cur = head->next;
	while (1)
	{
		if(cur->NO == N) break;
		pre = pre ->next;
		cur = cur ->next;
	}
	
	pre->next = cur->next;
	free(cur);
}

WP* return_first(){
	return head->next;
}
