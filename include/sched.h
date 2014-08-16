/*
 * sched.h - Estructures i macros pel tractament de processos
 */

#ifndef __SCHED_H__
#define __SCHED_H__

#include <list.h>
#include <types.h>
#include <stats.h>
#include <mm_address.h>

#define NR_TASKS      10
#define NR_SEMAPHORES   20
#define KERNEL_STACK_SIZE	1024
#define DEFAULT_QUANTUM 20

struct list_head freequeue;
struct list_head readyqueue;
struct list_head keyboardqueue;

extern int PID_COUNT;
extern int SEM_ID;
unsigned long quantum_ticks;

enum state_t { ST_RUN, ST_READY, ST_BLOCKED };

struct task_struct {
  int PID;			/* Process ID */
  page_table_entry * dir_pages_baseAddr;
  struct list_head list;
  int* stack_address;
  unsigned long quantum;
  struct stats st;
  int sem_id;
  void * heap_address;
  int heap_bytes;
  int heap_num_pages;
};

union task_union {
  struct task_struct task;
  unsigned long stack[KERNEL_STACK_SIZE];    /* pila de sistema, per procés */
};

struct sem_t {
  int sem_id;
  int owner_pid;
  int counter;
  struct list_head blocked_queue;
};

extern union task_union task[NR_TASKS]; /* Vector de tasques */
extern struct task_struct *idle_task;
struct task_struct *init_task;

struct sem_t sem[NR_SEMAPHORES]; // Vector of sepmaphores

#define KERNEL_ESP(t)       	(DWord) &(t)->stack[KERNEL_STACK_SIZE]

#define INITIAL_ESP       	KERNEL_ESP(&task[1])

/* Inicialitza les dades del proces inicial */
void init_task1(void);

void init_idle(void);

void init_sched(void);

struct task_struct * current();

void task_switch(union task_union*t);

struct task_struct *list_head_to_task_struct(struct list_head *l);

int allocate_DIR(struct task_struct *t);

page_table_entry * get_PT (struct task_struct *t) ;

page_table_entry * get_DIR (struct task_struct *t) ;

void inner_task_switch( union task_union *t);
void task_switch (union task_union *t);

//variables nostres
int dir_occupied[NR_TASKS];

/* Headers for the scheduling policy */
void sched_next_rr();
void update_current_state_rr(struct list_head *dest);
int needs_sched_rr();
void update_sched_data_rr();

void stats_ready_to_system();
void stats_user_to_system();
void stats_system_to_user();
void stats_system_to_ready();

void init_stats(struct task_struct * ts);
int calc_DIR(struct task_struct *t);

void block(struct list_head * process, struct list_head * dst_queue);
void unblock(struct list_head * process);

#endif  /* __SCHED_H__ */
