#include "list.h"

extern int is_mru;

struct node {
  void *addr;
  void *real_va;
  struct list_head list;
};

int evict_pp(void *addr);
int pin_pp(void *addr);
// int unpin_pp(void *addr);
int unpin_list_pp();
struct node *get_pp(void *addr);

// void empty_list_pp();
void evict_list_pp();

int read_pp(void *addr);
