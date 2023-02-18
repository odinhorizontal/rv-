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
#include <string.h>
#include <cpu/cpu.h>
#include <memory/paddr.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include "../ftrace.h"

static int is_batch_mode = false;

static inline bool in_pmem(paddr_t addr);
word_t paddr_read(paddr_t addr, int len);
void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char *rl_gets()
{
  static char *line_read = NULL;

  if (line_read)
  {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

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

static int cmd_help(char *args);
static int cmd_scan_memory(char *args);
static int cmd_si(char *args);
static int cmd_s(char *args);
static int cmd_p(char *args);
static int cmd_info(char *args);
static int cmd_w(char *args);
static int cmd_d(char *args);

static struct
{
  const char *name;
  const char *description;
  int (*handler)(char *);
} cmd_table[] = {
    {"help", "Display information about all supported commands", cmd_help},
    {"c", "Continue the execution of the program", cmd_c},
    {"q", "Exit NEMU", cmd_q},
    {"si", "Step Execute", cmd_si},
    {"p", "Evaluate a Math", cmd_p},
    {"info", "Print Register or WatchPoint Information", cmd_info},
    {"w", "Set WatchPoint", cmd_w},
    {"d", "delete WatchPoint", cmd_d}, 
    {"x", "Scan Memory", cmd_scan_memory},
    {"s", "Print Call Stack", cmd_s},
    /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_w(char *args) {
  if (args == NULL)
  {
    printf("invalid arguments: w $expr \n");
    return 0;
  }

  new_wp(args);
  return 0;
}

static int cmd_d(char *args) { 
  if (args == NULL)
  {
    printf("invalid arguments: d $expr \n");
    return 0;
  }

  int num = atoi(args);
  free_wp(num);
  return 0;
}

static int cmd_p(char *args)
{
  if (args == NULL)
  {
    printf("invalid arguments: p $expr \n");
    return 0;
  }

  bool success = false;
  word_t res = expr(args, &success);
  if (success)
  {
    printf("%d\n", res);
  }
  
  return 0;
}

static int cmd_scan_memory(char *args)
{
  if (args == NULL)
  {
    printf("invalid arguments: x n $addr \n");
    return 0;
  }
  char *p;
  p = strtok(args, " ");

  int n;
  if (p == NULL)
  {
    printf("invalid arguments: x n $addr \n");
    return 0;
  }

  n = atoi(args);
  if (n <= 0)
  {
    printf("illegal arguments %d\n", n);
    return 0;
  }
  // printf("n=%d", n);
  p = strtok(NULL, ",");
  uint32_t addr;
  if (p)
  {
    int num = strtol(p, NULL, 16);
    addr = (uint32_t)num;
  }
  else
  {
    printf("illegal address %d\n", n);
    return 0;
  }

  if (!in_pmem((paddr_t)addr))
  {
    printf("illegal address %d, not in PMEM\n", n);
    return 0;
  }

  for (uint32_t i = 0; i < n; i++)
  {
    if (!in_pmem((paddr_t)addr))
    {
      break;
    }
    word_t var = paddr_read((paddr_t)addr, 4);
    printf("0x%08x\n", var);
    addr += 4;
  }

  return 0;
}

static int cmd_si(char *args)
{
  if (args == NULL)
  {
    cpu_exec(1);
    return 1;
  }
  else
  {
    int n = atoi(args);
    if (n < 0)
    {
      printf("illegal step %d\n", n);
      return 0;
    }
    cpu_exec((uint64_t)n);
    return n;
  }
}

static int cmd_s(char *args)
{
  // print_stack_trace();
  return 0;
}

static int cmd_info(char *args)
{
  if (args == NULL)
  {
    printf("invalid arguments: info r || info w \n");
    return 0;
  }

  if ( strcmp(args , "r") == 0)
    isa_reg_display();
  else if ( strcmp(args, "w") == 0) {
    print_wp();
  } else {
    printf("invalid command\n");
  }
  return 0;
}

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

void sdb_set_batch_mode()
{
  is_batch_mode = true;
}

void sdb_mainloop()
{
  if (is_batch_mode)
  {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL;)
  {
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

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

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

void init_sdb()
{
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
