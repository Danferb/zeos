/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>
#include <utils.h>
#include <errno.h>
#include <devices.h>

extern int read_char_count;

union task_union task[NR_TASKS]
  __attribute__((__section__(".data.task")));

struct task_struct* idle_task;

//#if 1
struct task_struct *list_head_to_task_struct(struct list_head *l)
{
  return list_entry( l, struct task_struct, list);
}
//#endif 

extern struct list_head blocked;


/* get_DIR - Returns the Page Directory address for task 't' */
page_table_entry * get_DIR (struct task_struct *t) 
{
	return t->dir_pages_baseAddr;
}

/* get_PT - Returns the Page Table address for task 't' */
page_table_entry * get_PT (struct task_struct *t) 
{
	return (page_table_entry *)(((unsigned int)(t->dir_pages_baseAddr->bits.pbase_addr))<<12);
}


int allocate_DIR(struct task_struct *t) 
{
	int i;
	for (i = 0; i < NR_TASKS; ++i) {
		if (dir_occupied[i] <= 0) {
			t->dir_pages_baseAddr = (page_table_entry*) &dir_pages[i];
			dir_occupied[i] = 1;
			return 1;
		}
	}
	// we should never get here
	return -ENOMEM;
	
	/*int pos;
	pos = ((int)t-(int)task)/sizeof(union task_union);
	t->dir_pages_baseAddr = (page_table_entry*) &dir_pages[pos]; 
	return 1;*/
}

int calc_DIR(struct task_struct *t) {
	int init_dir = (int)((page_table_entry*) &dir_pages[0]);
	int t_dir = (int)(t->dir_pages_baseAddr);
	return (t_dir - init_dir) / sizeof(dir_pages[0]);
}

void cpu_idle(void)
{
	__asm__ __volatile__("sti": : :"memory");
	while(1)
	{
	;
	}
}

void init_idle (void){
	struct list_head * pointer;
	pointer = list_first(&freequeue);
	list_del(pointer);
	idle_task = list_head_to_task_struct(pointer); 
	idle_task->PID = 0;
	allocate_DIR(idle_task);
	union task_union * ptr = (union task_union *)idle_task;
	
	ptr->stack[KERNEL_STACK_SIZE-1] = (unsigned long)&cpu_idle; // push the address of cpu_idle() to the stack
	ptr->stack[KERNEL_STACK_SIZE-2] = 0; // push ebp = 0 to the stack
	idle_task->stack_address = (int *)&(ptr->stack[KERNEL_STACK_SIZE-2]); // save
	idle_task->quantum = DEFAULT_QUANTUM;
	
	idle_task->heap_address = NULL;
	idle_task->heap_num_pages = 0;
	idle_task->heap_bytes = 0;
	init_stats(idle_task);	
}

void init_task1(void){
    struct list_head * pointer;
	pointer = list_first(&freequeue);
	list_del(pointer);
	init_task = list_head_to_task_struct(pointer);
	init_task->PID = 1;
	init_task->heap_address = NULL;
	init_task->heap_num_pages = 0;
	init_task->heap_bytes = 0;
	allocate_DIR(init_task);
	set_user_pages(init_task);
	union task_union * ptr = (union task_union *)init_task;
	tss.esp0 = (unsigned long)&(ptr->stack[KERNEL_STACK_SIZE]); 
	set_cr3(init_task->dir_pages_baseAddr);
	init_task->quantum = DEFAULT_QUANTUM;
	
	init_stats(init_task);	
}

void init_sched(){
  	int i;
  	PID_COUNT = 2;
  	read_char_count = -1;
  	quantum_ticks = DEFAULT_QUANTUM;
	INIT_LIST_HEAD(&freequeue);
	INIT_LIST_HEAD(&readyqueue);
	INIT_LIST_HEAD(&keyboardqueue);
	kbf.start = 0;
	kbf.end = 0;
	kbf.char_count = 0;
	for(i = 0; i < NR_TASKS; ++i){
		list_add_tail(&task[i].task.list, &freequeue);
		dir_occupied[i] = 0;
	}
	for(i = 0; i < NR_SEMAPHORES; ++i) {
    	sem[i].owner_pid = -1;
  	}
}

struct task_struct* current()
{
  int ret_value;
  
  __asm__ __volatile__(
  	"movl %%esp, %0"
	: "=g" (ret_value)
  );
  return (struct task_struct*)(ret_value&0xfffff000);
}


void task_switch (union task_union *t){
	__asm__ __volatile__(
		"pushl %edi;"
		"pushl %esi;"
		"pushl %ebx;"
	);
	inner_task_switch(t);
	__asm__ __volatile__(
		"popl %ebx;"
		"popl %esi;"
		"popl %edi;"
	);

}

void inner_task_switch( union task_union *t){
	struct task_struct *old = current();	
	tss.esp0 = (int)&(t->stack[KERNEL_STACK_SIZE]);
	set_cr3(t->task.dir_pages_baseAddr);	
	__asm__ __volatile__(
		"movl %%ebp,%0;"
		"movl %1,%%esp;"
		"popl %%ebp;"
		"ret"
		: "=g" (old->stack_address) 
		: "g" (t->task.stack_address)
	);
}

void update_sched_data_rr(){
	if(quantum_ticks > 0) {
		--quantum_ticks;
		current()->st.remaining_ticks = quantum_ticks;
	}
}

int needs_sched_rr(void) {
	return (!quantum_ticks ); 
}

void update_current_state_rr(struct list_head *dst_queue) {
	list_add_tail(&(current()->list), dst_queue);		
}

void sched_next_rr(){
    if(list_empty(&readyqueue)) task_switch((union task_union*) idle_task); 
	else {
	    struct list_head *pointer = list_first(&readyqueue);
	    list_del(pointer);
	    union task_union* task = (union task_union*)list_head_to_task_struct(pointer);
	    quantum_ticks = task->task.quantum;
	    ++(task->task.st.total_trans);
	    task_switch(task);
	}
}

int get_quantum(struct task_struct * t) {
	return current()->quantum;
}

void set_quantum(struct task_struct * t, int new_quantum) {
	current()->quantum = new_quantum;
}


void stats_system_to_user(){
	unsigned long current_ticks = get_ticks();
	current()->st.system_ticks += current_ticks - current()->st.elapsed_total_ticks;
	current()->st.elapsed_total_ticks = current_ticks;
}

void stats_system_to_ready(){
	unsigned long current_ticks = get_ticks();
	current()->st.system_ticks += current_ticks - current()->st.elapsed_total_ticks;
	current()->st.elapsed_total_ticks = current_ticks;
}
void stats_ready_to_system(){
	unsigned long current_ticks = get_ticks();	
	current()->st.ready_ticks += current_ticks - current()->st.elapsed_total_ticks;
	current()->st.elapsed_total_ticks = current_ticks;
}

void stats_user_to_system(){
	unsigned long current_ticks = get_ticks();
	current()->st.user_ticks += current_ticks - current()->st.elapsed_total_ticks;
	current()->st.elapsed_total_ticks = current_ticks;
}

void init_stats(struct task_struct * ts) {
	ts->st.user_ticks = 0;
  	ts->st.system_ticks = 0;
  	ts->st.blocked_ticks = 0;
  	ts->st.ready_ticks = 0;
  	ts->st.elapsed_total_ticks = get_ticks();
  	ts->st.total_trans = 0; /* Number of times the process has got the CPU: READY->RUN transitions */
  	ts->st.remaining_ticks = 0;
}


void block(struct list_head * process, struct list_head * dst_queue) {
    list_add_tail(process, dst_queue);
    sched_next_rr();
}

void unblock(struct list_head * process) {
    list_del(process);
    list_add(process, &readyqueue);
}
