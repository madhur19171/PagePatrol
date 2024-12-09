#include "pagepatrol.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#define THRESHOLD 1024

int is_mru = 0;
int my_pid = -1;

static LIST_HEAD(generic_list);
static LIST_HEAD(pinned_list);
int list_size = 0;

int check_pid() {
  if (my_pid == -1) {
    my_pid = getpid();
  }
  return my_pid;
}

int evict_pp(void *addr) {
  int _pid = check_pid();
  // printf("evict %p\n", addr);
  unsigned long value = (unsigned long)addr;
  /* 0 - all good, !0 - bad */
  int ret = syscall(548, _pid, 0, value, 0, 1, 100);
  return ret;
  if (ret)
    return ret;
  return syscall(548, 0, 0, value, 0, 0, 400);
}

int pin_pp(void *addr) {
  int _pid = check_pid();

  // return mlock(addr, 1024);
  unsigned long value = (unsigned long)addr;

  /* 0 - all good, !0 - bad */
  // return syscall(548, 0, 0, value, 0, 1, 200);
  unsigned long paddr = syscall(548, _pid, 0, value, 0, 1, 20);

  struct node *ret_node = (struct node *)malloc(sizeof(struct node));
  if (!ret_node)
    return 0;

  ret_node->addr = (void *)paddr;
  ret_node->real_va = addr;

  // printf("pin %p - %p - %ld - %ld\n", (void *)addr, (void *)paddr,
  //        (unsigned long)addr, paddr);

  ret_node->list.prev = &ret_node->list;
  ret_node->list.next = &ret_node->list;

  list_add(&ret_node->list, &pinned_list);

  return 0;
}

int unpin_pp2(struct node *node) {

  int _pid = check_pid();
  unsigned long va = (unsigned long)node->real_va;
  unsigned long pvalue = (unsigned long)node->addr;

  // printf("unpin 2 %p - %p - %ld - %ld\n", (void *)va, (void *)pvalue, va,
  //        pvalue);
  return syscall(548, _pid, 0, va, 0, pvalue, 5);
}

int unpin_pp(void *addr) {

  int _pid = check_pid();
  unsigned long value = (unsigned long)addr;

  return syscall(548, _pid, 0, value, 0, value, 5);

  return munlock(addr, 1);
}

int unpin_list_pp() {
  struct list_head *item;
  list_for_each(item, &pinned_list) {
    struct node *node = list_entry(item, struct node, list);
    if (node) {
      // unpin_pp(node->addr);
      unpin_pp2(node);
    }
  }
  return 0;
}

/*
 *
 * Inside struct folio we have the user space address of a list node
 * if this syscall returns 0
 *   -> first time
 * else
 *   -> already exists
 *
 */
struct node *get_pp_2(void *addr) {
  int _pid = check_pid();
  unsigned long value = (unsigned long)addr;
  /* 0 - all good, !0 - bad */
  unsigned long alist = syscall(548, _pid, 0, value, 0, 1, 300);
  struct node *ret_node = NULL;
  if (alist <= 0) {
    /* not found */
    ret_node = (struct node *)malloc(sizeof(struct node));
    if (!ret_node)
      return NULL;

    ret_node->addr = addr;

    ret_node->list.prev = &ret_node->list;
    ret_node->list.next = &ret_node->list;

    unsigned long laddr = (unsigned long)&(ret_node->list);
    syscall(548, _pid, 0, value, 0, laddr, 400);
    // printf("set %p -> %p -> %p\n", addr, ret_node, &ret_node->list);
  } else {
    /* yay */
    struct list_head *head = ((struct list_head *)alist);
    if (!head)
      return NULL;
    // printf("found %p -> %p\n", addr, head);
    ret_node = (struct node *)list_entry(head, struct node, list);
  }
  if (ret_node)
    list_add(&ret_node->list, &generic_list);

  // list_size++;
  // if (list_size >= 1) {
  // }
  // evict_pp(addr);
  // empty_list_pp(addr);
  return ret_node;
}

struct node *get_pp(void *addr) {
  struct node *ret_node = NULL;
  ret_node = (struct node *)malloc(sizeof(struct node));
  if (!ret_node)
    return NULL;

  ret_node->addr = addr;

  ret_node->list.prev = &ret_node->list;
  ret_node->list.next = &ret_node->list;

  list_add(&ret_node->list, &generic_list);

  list_size++;
  if (list_size >= THRESHOLD) {
    // empty_list_pp(addr);
  }
  return ret_node;
}

void empty_list_pp() {
  if (!is_mru) {
    return;
  }
  // if (list_empty(&generic_list))
  //   return;
  // struct list_head *item;
  // list_for_each(item, &generic_list) {
  //   struct node *node = list_entry(item, struct node, list);
  //   if (node) {
  //     // printf("evict %p - %p\n", node->addr, node);
  //     evict_pp(node->addr);
  //   }
  // }

  // printf("start %d\n", c);
  int tot = 0;
  struct list_head *begin = &generic_list;
  struct list_head *head = generic_list.next;
  while (head) {
    struct node *node = list_entry(head, struct node, list);
    struct list_head *next = head->next;
    if (node) {
      tot++;
      evict_pp(node->addr);
      list_del(&node->list);
      free(node);
    }
    head = next;
    if (head == begin)
      break;
  }
  // assert(tot == THRESHOLD);
  list_size = 0;
}

void evict_list_pp() {
  if (!is_mru) {
    return;
  }
  // if (list_empty(&generic_list))
  //   return;
  struct list_head *item;
  int max = 1000000;
  int i = 0;
  list_for_each(item, &generic_list) {
    struct node *node = list_entry(item, struct node, list);
    if (node) {
      evict_pp(node->addr);
    }
    if (i++ > max)
      break;
  }
}

int read_pp(void *addr) {
  get_pp(addr);
  return 0;
  return evict_pp(addr);
}
