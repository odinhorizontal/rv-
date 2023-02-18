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

#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint
{
  int NO;
  struct watchpoint *next;
  char expr[256];
  int value;
  /* TODO: Add more members if necessary */

} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;
void new_wp(char *exp);
void free_wp(int no);
void print_wp();

void init_wp_pool()
{
  int i;
  for (i = 0; i < NR_WP; i++)
  {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

void new_wp(char *exp)
{
  if (free_ == NULL)
  {
    printf("no free_watchpoint\n");
    return;
  }

  bool success = false;
  int val = expr(exp, &success);
  if (!success)
  {
    return;
  }

  WP *t = free_;
  free_ = free_->next;
  t->next = NULL;
  t->value = val;
  strcpy(t->expr, exp);

  if (head == NULL) {
    head = t;
    t->NO = 0;
  }
  else
  {
    WP *p = head;
    while (p->next)
      p = p->next;
    
    p->next = t;
    t->NO = p->NO + 1;
  }
  return;
}

void free_wp(int no)
{
  if (head == NULL)
    return;
  else if( head->NO == no ) {
    head = head->next;
  } else {
    WP *p = head;
    WP *q = head->next;
    while (q != NULL && q->NO != no) {
      p = p->next;
      q = q->next;
    } 
    if ( q == NULL ) {
      return;
    } else {
      p->next = q->next;
      free(q);
    }
  }
}

void print_wp()
{
  WP *ptr = head;
  if (!ptr)
  {
    printf("no remaining watcher point! \n");
  }
  else
  {
    while (ptr != NULL)
    {
      printf("%d   %s  %d\n", ptr->NO, ptr->expr, ptr->value);
      ptr = ptr->next;
    }
  }
}

/* TODO: Implement the functionality of watchpoint */

