#include "temu.h"
#include "reg.h"
/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

enum
{
	EQ,
	NOTEQ,
	Dec,
	lPar,
	rPar,
	Hex,
	Reg,
	NOT,
	AND,
	OR,
	NOTYPE = 256

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
	{"==", EQ},					// equal
	{"!=", NOTEQ},
	{"!", NOT},
	{"\\&&", AND},
	{"\\|\\|", OR},
	{"\\$zero|\\$at|\\$v0|v1|\\$a0|\\$a1|\\$a2|\\$a3|\\$t0|\\$t1|\\$t2|\\$t3|\\$t4|\\$t5|\\$t6|\\$t7|\\$s0|\\$s1|\\$s2|\\$s3|\\$s4|\\$s5|\\$s6|\\$s7|\\$t8|\\$t9|\\$k0|\\$k1|\\$gp|\\$sp|\\$fp|\\$ra|\\$pc", Reg},
	{"0[xX][0-9a-fA-F]+", Hex}, // 十六进制
	{"^-?[0-9]+[0-9]*", Dec}	// 十进制
	
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
				case Hex:
				case Reg:
					tokens[nr_token].type = rules[i].token_type;
					for (int j = 0; j < substr_len; j++)
						tokens[nr_token].str[j] = substr_start[j];
					//Log("%s", tokens[nr_token].str);
					nr_token++;
					break;
				case '+':
				case '-':
				case '/':
				case '*':
				case lPar:
				case rPar:
				case EQ:
				case NOTEQ:
				case NOT:
				case AND:
				case OR:
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
	int tag = tokens[p].type == lPar ? 1 : 0;

	for (int i = p; i <= q; i++)
	{
		if (tokens[i].type == rPar && parten == 0)
			assert(0);
		if (tokens[i].type == rPar)
			parten--;
		if (tokens[i].type == lPar)
			parten++;
		if (parten == 0 && i != q)
			tag = 0;
	}
	if (parten > 0)
		assert(0);

	return tag;
}

int c2i(char ch)
{
	// 如果是数字，则用数字的ASCII码减去48, 如果ch = '2' ,则 '2' - 48 = 2
	if (isdigit(ch))
		return ch - 48;

	// 如果是字母，但不是A~F,a~f则返回
	if (ch < 'A' || (ch > 'F' && ch < 'a') || ch > 'z')
		return -1;

	// 如果是大写字母，则用数字的ASCII码减去55, 如果ch = 'A' ,则 'A' - 55 = 10
	// 如果是小写字母，则用数字的ASCII码减去87, 如果ch = 'a' ,则 'a' - 87 = 10
	if (isalpha(ch))
		return isupper(ch) ? ch - 55 : ch - 87;

	return -1;
}

// 功能：将十六进制字符串转换为整型(int)数值
int hex2dec(char *hex)
{
	int len;
	int num = 0;
	int temp;
	int bits;
	int i;

	len = strlen(hex);

	for (i = 2, temp = 0; i < len; i++, temp = 0)
	{
		temp = c2i(*(hex + i));
		bits = (len - i - 1) * 4;
		temp = temp << bits;

		// 此处也可以用 num += temp;进行累加
		num = num | temp;
	}

	// 返回结果
	return num;
}

int eval(int p, int q)
{
	if (p > q)
		return -1;
	else if (p == q)
	{
		//Log("%s", tokens[p].str);
		if (tokens[p].type == Dec)
			return atoi(tokens[p].str);
		else if(tokens[p].type == Hex)
			return hex2dec(tokens[p].str);
		else 
		{
			for(int i=0; i<32; i++)
			{
				if(!strcmp(tokens[p].str, regfile[i])){
					//Log("%s %d", regfile[i],cpu.gpr[i]._32);
					return cpu.gpr[i]._32;
				}
			}
			return cpu.pc;
		}
	}
	else if (check_parentheses(p, q) == true)
	{
		return eval(p + 1, q - 1);
	}
	else
	{
		int op = -1; // 寻找分割符号
		char op_type = 0;
		int priority = 100; // 最低优先级
		int inPar = 0;	  // 如果当前符号在括号里就不能作为分割符号
		for (int i = 0; i < q - p; i++)
		{
			if (tokens[i + p].type == NOT && priority >= 5 && inPar == 0)
			{
				op = i + p;
				op_type = tokens[i + p].type;
				priority = 5;
			}
			if ((tokens[i + p].type == '*' || tokens[i + p].type == '/') && priority >= 4 && inPar == 0)
			{
				op = i + p;
				op_type = tokens[i + p].type;
				priority = 4;
			}
			if ((tokens[i + p].type == '+' || tokens[i + p].type == '-') && priority >= 3 && inPar == 0)
			{
				op = i + p;
				op_type = tokens[i + p].type;
				priority = 3;
			}
			if((tokens[i + p].type == EQ || tokens[i + p].type == NOTEQ) && priority >= 2 && inPar == 0)
			{
				op = i + p;
				op_type = tokens[i + p].type;
				priority = 2;
			}
			if(tokens[i + p].type == AND  && priority >= 1 && inPar == 0)
			{
				op = i + p;
				op_type = tokens[i + p].type;
				priority = 1;
			}
			if(tokens[i + p].type == OR  && priority >= 0 && inPar == 0)
			{
				op = i + p;
				op_type = tokens[i + p].type;
				priority = 0;
			}
			if (tokens[i + p].type == lPar)
				inPar += 1;
			if (tokens[i + p].type == rPar)
				inPar -= 1;
		}

		int val1 = eval(p, op - 1);
		int val2 = eval(op + 1, q);
		//Log("%d %d", val1, val2);
		switch (op_type)
		{
		case '+':
			return val1 + val2;
		case '-':
			return val1 - val2;
		case '/':
			return val1 / val2;
		case '*':
			return val1 * val2;
		case EQ:
			return val1 == val2;
		case NOTEQ:
			return val1 != val2;
		case NOT:
			return !val2;
		case AND:
			return val1 && val2;
		case OR:
			return val1 || val2;			 
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
	//printf("%d\n", eval(0, nr_token - 1));
	/* TODO: Insert codes to evaluate the expression. */
	//panic("please implement me");
	return eval(0, nr_token - 1);
}
