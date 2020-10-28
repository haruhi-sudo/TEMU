#include "temu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>
enum
{
	NOTYPE = 256,
	EQ,
	Dec,
	lPar,
	rPar
	/* TODO: Add more token types */

};

static struct rule
{
	char *regex;
	int token_type;
} rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */

	{" +", NOTYPE}, // spaces
	{"\\+", '+'},	// plus
	{"\\-", '-'},
	{"\\*", '*'},
	{"\\/", '/'},
	{"\\(", lPar},
	{"\\)", rPar},
	{"==", EQ},				 // equal
	{"^-?[1-9]+[0-9]*", Dec} // 十进制
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]))

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex()
{
	int i;
	char error_msg[128];
	int ret;

	for (i = 0; i < NR_REGEX; i++)
	{
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if (ret != 0)
		{
			regerror(ret, &re[i], error_msg, 128);
			Assert(ret == 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

typedef struct token
{
	int type;
	char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e)
{
	int position = 0;
	int i;
	regmatch_t pmatch;

	nr_token = 0;

	while (e[position] != '\0')
	{
		/* Try all rules one by one. */
		for (i = 0; i < NR_REGEX; i++)
		{
			if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0)
			{
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array `tokens'. For certain types
				 * of tokens, some extra actions should be performed.
				 */

				switch (rules[i].token_type)
				{
				case Dec:
					tokens[nr_token].type = rules[i].token_type;
					for (int j = 0; j < substr_len; j++)
						tokens[nr_token].str[j] = substr_start[j];
					//Log("%s", tokens[nr_token].str);
					nr_token++;
					break;
				case '+':
					tokens[nr_token++].type = rules[i].token_type;
					break;
				case '-':
					tokens[nr_token++].type = rules[i].token_type;
					break;
				case '/':
					tokens[nr_token++].type = rules[i].token_type;
					break;
				case '*':
					tokens[nr_token++].type = rules[i].token_type;
					break;
				case lPar:
					tokens[nr_token++].type = rules[i].token_type;
					break;
				case rPar:
					tokens[nr_token++].type = rules[i].token_type;
					break;
				case NOTYPE:
					break;
				default:
					panic("please implement me");
				}

				break;
			}
		}

		if (i == NR_REGEX)
		{
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
	}

	return true;
}
int check_parentheses(int p, int q)
{
	int parten = 0;
	int tag = tokens[p].type == lPar?1:0;

	for (int i = p; i <= q; i++)
	{
		if (tokens[i].type == rPar && parten == 0)
			assert(0);
		if (tokens[i].type == rPar)
			parten--;
		if (tokens[i].type == lPar)
			parten++;
		if (parten == 0 && i!=q) tag = 0;
	}
	Log("%d", parten);
	if (parten > 0)
		assert(0);

	return tag;
}

int eval(int p, int q)
{
	if (p > q)
		return -1;
	else if (p == q)
	{
		Log("%s", tokens[p].str);
		return atoi(tokens[p].str);
	}
	else if (check_parentheses(p, q) == true)
	{
		return eval(p + 1, q - 1);
	}
	else
	{
		int op = -1; // 寻找分割符号
		char op_type = 0;
		int lowerpri = 0; // 是否有更低优先级
		int inPar = 0; // 如果当前符号在括号里就不能作为分割符号
		for (int i = 0; i < q - p; i++)
		{
			if ((tokens[i + p].type == '+' || tokens[i + p].type == '-') && inPar == 0)
			{
				op = i + p;
				op_type = tokens[i + p].type;
				lowerpri = 1;
			}
			if ((tokens[i + p].type == '*' || tokens[i + p].type == '/') && lowerpri == 0 && inPar == 0)
			{
				op = i + p;
				op_type = tokens[i + p].type;
			}
			if (tokens[i + p].type == lPar)
				inPar = 1;
			if (tokens[i + p].type == rPar)
				inPar = 0;
		}
		Log("%d", op);
		int val1 = eval(p, op - 1);
		int val2 = eval(op + 1, q);
		switch (op_type)
		{
		case '+':
			return val1 + val2;
			break;
		case '-':
			return val1 - val2;
			break;
		case '/':
			return val1 / val2;
			break;
		case '*':
			return val1 * val2;
			break;
		default:
			assert(0);
		}
	}
	return 1;
}

uint32_t expr(char *e, bool *success)
{
	if (!make_token(e))
	{
		*success = false;
		return 0;
	}
	printf("%d\n", eval(0, nr_token - 1));
	/* TODO: Insert codes to evaluate the expression. */
	//panic("please implement me");
	return 0;
}
