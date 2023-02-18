/***************************************************************************************
 * Copyright (c) 2014-2022 Zihao Yu, Nanjing University
 *
 * NEMU is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum
{ 
  TK_SPACE = 38,
  TK_NOPO = 39,
  TK_LEFT_BRACE = 40,  // (
  TK_RIGHT_BRACE = 41, // )
  TK_MULTIPLY = 42,    // *
  TK_DIVISION = 43,    // /
  TK_MINUS = 44,       // -
  TK_ADD = 45,         // +
  TK_EQ = 257,         // ==
  TK_NTEQ = 258,       // !=
  TK_AND = 259,        // &&
  TK_OR = 260,         // ||
  TK_NOT = 261,        // !
  TK_HEX = 262,        // 16
  TK_DECI = 263,       // 10
  TK_REG = 264,        // register
  TK_OCT = 265,        // 8
  TK_VAR = 266,        // variable
};

static struct rule
{
  const char *regex;
  int token_type;
} rules[] = {
    {" +", TK_SPACE},    // spaces
    {"\\+", TK_ADD},    // plus
    {"/", TK_DIVISION},      // divide
    {"\\*", TK_MULTIPLY},    // multi
    {"\\-", TK_MINUS},    // minus
    {"==", TK_EQ},   // equal
    {"!=", TK_NTEQ}, // not equal
    {"&&", TK_AND},  // and &&
    {"!", TK_NOT},
    {"\\(", TK_LEFT_BRACE},
    {"\\)", TK_RIGHT_BRACE},
    {"\\|\\|", TK_OR},              // OR
    {"0x[0-9a-f]+", TK_HEX},        // hex nums
    {"\\$[a-ehilpx]{2,3}", TK_REG}, // the registers
    {"[0-9]+", TK_DECI},            // decimal nums
    {"[0-9]{1,10}", TK_OCT},
    {"[a-zA-Z_][a-zA-Z0-9_]*", TK_VAR}, // variable
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

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
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token
{
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used)) = 0;

static bool make_token(char *e)
{
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0')
  {
    for (i = 0; i < NR_REGEX; i++)
    {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0)
      {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;

        switch (rules[i].token_type)
        {
        case TK_LEFT_BRACE:
          tokens[nr_token].type = TK_LEFT_BRACE;
          strcpy(tokens[nr_token].str, "(");
          break;
        case TK_RIGHT_BRACE:
          tokens[nr_token].type = TK_RIGHT_BRACE;
          strcpy(tokens[nr_token].str, ")");
          break;
        case TK_DIVISION:
          tokens[nr_token].type = TK_DIVISION;
          strcpy(tokens[nr_token].str, "/");
          break;
        case TK_ADD:
          tokens[nr_token].type = TK_ADD;
          strcpy(tokens[nr_token].str, "+");
          break;
        case TK_MULTIPLY:
          tokens[nr_token].type = TK_MULTIPLY;
          strcpy(tokens[nr_token].str, "*");
          break;
        case TK_MINUS:
          tokens[nr_token].type = TK_MINUS;
          strcpy(tokens[nr_token].str, "-");
          break;
        case TK_EQ:
          tokens[nr_token].type = TK_EQ;
          strcpy(tokens[nr_token].str, "==");
          break;
        case TK_NTEQ:
          tokens[nr_token].type = TK_NTEQ;
          strcpy(tokens[nr_token].str, "!=");
          break;
        case TK_AND:
          tokens[nr_token].type = TK_AND;
          strcpy(tokens[nr_token].str, "&&");
          break;
        case TK_OR:
          tokens[nr_token].type = TK_OR;
          strcpy(tokens[nr_token].str, "||");
          break;
        case TK_HEX:
          tokens[nr_token].type = TK_HEX;
          break;
        case TK_DECI:
          tokens[nr_token].type = TK_DECI;
          strncpy(tokens[nr_token].str, &e[position - substr_len], substr_len);
          break;
        case TK_REG:
          tokens[nr_token].type = TK_REG;
          strncpy(tokens[nr_token].str, &e[position - substr_len], substr_len);
          break;
        case TK_OCT:
          tokens[nr_token].type = TK_OCT;
          strncpy(tokens[nr_token].str, &e[position - substr_len], substr_len);
          break;
        case TK_VAR:
          tokens[nr_token].type = TK_VAR;
          strncpy(tokens[nr_token].str, &e[position - substr_len], substr_len);
          break;
        case TK_NOT:
          tokens[nr_token].type = TK_NOT;
          strcpy(tokens[nr_token].str, "!");
          break;
        default:
          nr_token--;
          break;
        }
        nr_token++;
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

int find_matched_brace_idx(int p, int q)
{

  // for (int i=p ; i <= q; i++) {
  //   printf("  %s  ", tokens[i].str);
  // }

  int matched = 1;
  if (tokens[p].type != TK_LEFT_BRACE)
  {
    return p;
  }

  for( ++p; p <= q ; p++ ) {
    // printf(" matched %d \n", matched);
    if ( matched == 0 ) {
      // printf("the matched position is %d\n", p);
      return p;
    }

    if (tokens[p].type == TK_LEFT_BRACE ) {
      matched = matched + 1;
    } else if (tokens[p].type == TK_RIGHT_BRACE) {
      matched = matched - 1;
    }
  }
  // printf("not found the matched position\n");
  return -1;
}

int find_matched_brace_postion(int p, int q)
{
  if (tokens[p].type != TK_LEFT_BRACE)
  {
    return p;
  }

  for( ; q >= p ; q--) {
    if (tokens[q].type == TK_RIGHT_BRACE) {
      return q;
    }
  }
  return -1;
}
int get_next_operation_idx(int p, int q)
{ 
  // for (int i=p ; i <= q; i++) {
  //   printf("  %s  ", tokens[i].str);
  // }
  int preOperation = -1;

  if( tokens[p].type == TK_LEFT_BRACE) {
    p = find_matched_brace_idx(p, q);
    if ( p < 0) {
      return -1;
    }
    p++;
  }
  

  for( ; p <= q ; p++) {
    // printf("find next one: %d - %d\n", p, q);
    if( tokens[p].type < TK_HEX ) {
      preOperation = p; 
      break;
    }
  }

  for (; p <= q; p++)
  {
    // printf("token type: %d\n", tokens[p].type);
    switch (tokens[p].type)
    {
    case TK_NOT:
      continue;
    case TK_HEX:
      continue;
    case TK_DECI:
      continue;
    case TK_REG:
      continue;
    case TK_OCT:
      continue;
    case TK_VAR:
      continue;
    case TK_SPACE:
      continue;
    case TK_LEFT_BRACE:
      p = find_matched_brace_idx(p, q);
      continue;
    case TK_OR:
      preOperation = p;
      continue;
    case TK_AND:
      if (tokens[preOperation].type < TK_AND)
      {
        preOperation = p;
      }
      continue;
    case TK_NTEQ:
      if (tokens[preOperation].type < TK_NTEQ)
      {
        preOperation = p;
      }
      continue;
    case TK_EQ:
      if (tokens[preOperation].type < TK_EQ)
      {
        preOperation = p;
      }
      continue;
    case TK_ADD:
      if (tokens[preOperation].type <= TK_ADD)
      {
        preOperation = p;
      }
      continue;
    case TK_MINUS:
      if (tokens[preOperation].type < TK_MINUS)
      {
        preOperation = p;
      }
      continue;
    case TK_DIVISION:
      if ( (tokens[preOperation].type <= TK_MULTIPLY || tokens[preOperation].type <= TK_DIVISION))
      {
        preOperation = p;
      }
      continue;
    case TK_MULTIPLY:
      if ((tokens[preOperation].type <= TK_MULTIPLY || tokens[preOperation].type <= TK_DIVISION) )
      {
        preOperation = p;
      }
      continue;
    default:
      continue;
    }
  }
  return preOperation;
}

bool valid_parentheses(int start, int end)
{
  int top = -1;
  for (int i = start; i <= end; i++)
  {
    if (tokens[i].type == TK_LEFT_BRACE)
    {
      ++top;
    }
    else if (tokens[i].type == TK_RIGHT_BRACE)
    {
      if (top == -1)
      {
        return false;
      }
      else
      {
        top--;
      }
    }
  }
  return top == -1;
}

int eval(int p, int q, bool *success)
{

  if (!valid_parentheses(p,q)) {
    // printf("invalid parentheses\n");
    assert(0);
  }

  if ( ! *success) {
    assert(0);
  }
  // printf("p=%d, q=%d\n", p, q);
  assert(p <= q);

  if (p == q)
  {
    switch (tokens[p].type)
    {
    case TK_DECI:
      return strtol(tokens[p].str, NULL, 10);
    case TK_HEX:
      return strtol(tokens[p].str, NULL, 16);
    case TK_REG:
      // TODO: add register here
      return 1;
    default:
      assert(0);
    }
  }
  else if ( tokens[p].type == TK_LEFT_BRACE && tokens[q].type == TK_RIGHT_BRACE && find_matched_brace_idx(p, q) == q ) 
  {
    // printf("in two braces\n");
    if( valid_parentheses(p, q)) {
      return eval(p + 1, q - 1, success);
    } else {
      // printf("invalid parentheses\n");
      assert(0);
    }
  }
  else
  {
    int operator;
    if (q - p == 1)
    {
      if (tokens[p].type == TK_MINUS)
      {
        return -eval(q, q, success);
      }
      else if (tokens[p].type == TK_NOT)
      {
        return !eval(p + 1, q, success);
      }
    }

    operator= get_next_operation_idx(p, q);
    if( operator < 0 ) {
      assert(0);
    }

    int leftOps = eval(p, operator- 1, success);
    int rightOPs = eval(operator+ 1, q, success);
    switch (tokens[operator].type)
    {
    case TK_ADD:
      return leftOps + rightOPs;
    case TK_MINUS:
      return leftOps - rightOPs;
    case TK_MULTIPLY:
      return leftOps * rightOPs;
    case TK_DIVISION:
      return leftOps / rightOPs;
    case TK_EQ:
      return (leftOps == rightOPs);
    case TK_NTEQ:
      return (leftOps != rightOPs);
    case TK_AND:
      return (leftOps && rightOPs);
    case TK_OR:
      return (leftOps || rightOPs);
    default:
      assert(0);
    }
  }
}

word_t expr(char *e, bool *success)
{
  if (!make_token(e))
  {
    assert(0);
  }

  *success = true;

  return eval(0, nr_token-1, success);
}
