/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>
#include <utils.h>
#include <io.h>
#include <mm.h>
#include <mm_address.h>
#include <sched.h>
#include <errno.h>

#define LECTURA 0
#define ESCRIPTURA 1


extern unsigned int zeos_ticks;
int PID_COUNT;
int SEM_ID = 0;

int sys_sem_destroy(int n_sem);
    
int check_fd(int fd, int permissions)
{
  if (fd>1 || fd<0) return -9; /*EBADF*/
  if (fd==1 && permissions!=ESCRIPTURA) return -13; /*EACCES*/
  else if (fd==0 && permissions != LECTURA) return -13; /*EACCES*/
  return 0;
}

int sys_ni_syscall()
{
	return -38; /*ENOSYS*/
}

int sys_getpid()
{
	return current()->PID;
}

int ret_from_fork(){
	return 0;
}

int sys_fork() {    
	int i, j;
	int heap_pages = current()->heap_num_pages;
	int created_frames[NUM_PAG_DATA+heap_pages];
	unsigned long ebp_current,p;

	//see if there's empty tsks;
	if (list_empty(&freequeue)) {
		return -ENOMEM;
	}
	
	//Reserve frames if not free, get them free and return and error
	for (i = 0; i < NUM_PAG_DATA+heap_pages; ++i) {
		created_frames[i] = alloc_frame();
		if (created_frames[i] == -1) {
			for (j = 0; j < i; ++j) {
				free_frame(created_frames[j]);
			}
			return -ENOMEM;
		}
	}
	
	//PCB
	struct list_head *pointer = list_first(&freequeue);
	list_del(pointer);
	union task_union* child_task = (union task_union*)list_head_to_task_struct(pointer);
	union task_union* parent_task = (union task_union*)current();
	
	copy_data(parent_task,child_task,sizeof(union task_union)); //union to child
	allocate_DIR(&(child_task->task));  //dir pag to child	

	page_table_entry* child_page = get_PT(&(child_task->task));
	page_table_entry* parent_page = get_PT(&(parent_task->task));
	
	//copy code
	copy_data(parent_page+NUM_PAG_KERNEL,child_page+NUM_PAG_KERNEL,NUM_PAG_CODE*sizeof(page_table_entry));

	
	//copy data pages
	int max_page = NUM_PAG_DATA+parent_task->task.heap_num_pages;
	for(i = 0; i < max_page;++i) {//logic to frame
		set_ss_pag(child_page, PAG_LOG_INIT_DATA_P0+i ,created_frames[i]);
		set_ss_pag(parent_page, HEAP_START_PAGE+heap_pages+i,created_frames[i]);	
		copy_data((void *)((PAG_LOG_INIT_DATA_P0+i)*PAGE_SIZE),
		    	(void *)((HEAP_START_PAGE+heap_pages+i)*PAGE_SIZE), PAGE_SIZE);
		del_ss_pag(parent_page, HEAP_START_PAGE+heap_pages+i);
	}
	
	//Old fork without dynamic mem
	/*copy_data((void *)((PAG_LOG_INIT_DATA_P0)<<12),(void *) ((PAG_LOG_INIT_DATA_P0+NUM_PAG_DATA+heap_pages)<<12), (NUM_PAG_DATA+heap_pages)*PAGE_SIZE);
	
	//restore parent
	for(i = 0; i < NUM_PAG_DATA+heap_pages;i++) {
		del_ss_pag(parent_page, PAG_LOG_INIT_DATA_P0+NUM_PAG_DATA+heap_pages+i);
    }*/
    
    set_cr3(get_DIR(&(parent_task->task)));
    
	__asm__ __volatile__(
		"movl %%ebp,%0"
	: "=g" (ebp_current));
	p = ((unsigned int)ebp_current-(unsigned int)&(parent_task->stack))/4;
	//pp fill assegurem el -19 o -20??size-1 KERNEL_STACK_SIZE-19?	
	child_task->stack[p-1] = 0;
	child_task->stack[p] = (unsigned long)&ret_from_fork;
	child_task->task.stack_address = (int *)&(child_task->stack[p-1]);
	
	init_stats((struct task_struct *)child_task);    
	child_task->task.PID = ++PID_COUNT;
	//child_task->task.quantum = DEFAULT_QUANTUM;
	list_add_tail(&(child_task->task.list),&readyqueue);
    return child_task->task.PID;
}


void sys_exit(){
	//Old exit without semaphores        
	/*
	int i;
	page_table_entry* pte_task = get_PT(current());
	for(i = PAG_LOG_INIT_DATA_P0; i < PAG_LOG_INIT_DATA_P0 + NUM_PAG_DATA; i++){
		free_frame(get_frame(pte_task,i));
	}
	current()->PID = -1;
	list_add_tail(&(current()->list),&freequeue);
	int pos = calc_DIR(current());
	--dir_occupied[pos];
	//if(list_empty(&readyqueue)) task_switch((union task_union*) idle_task); 
	sched_next_rr();*/
  	int i;
  	for (i = 0; i < NR_SEMAPHORES; ++i) {
    	if (sem[i].owner_pid == current()->PID) {
      		sys_sem_destroy(i);
    	}
  	}
	current()->PID = -1;
  	--dir_occupied[calc_DIR(current())];
  	if (dir_occupied[calc_DIR(current())] <= 0) {
    	free_user_pages(current());
  	}

  	update_current_state_rr(&freequeue);
  	sched_next_rr();
}


int sys_write(int fd, char *buffer, int size){
	/*fd:file descriptor. In this delivery it must be always 1.
	buffer: pointer to the bytes.
	size: number of bytes.
	Return negative number in case of error(specifying the kind of error) and the number of bytes written if 		OK*/
	
	int check = check_fd(fd, ESCRIPTURA);
    if(check != 0) {
    	return check;
   	}
    	
    if (buffer == NULL){
     	return -EFAULT;
     }
    if (size < 0) {
    	return -EINVAL;
    }

	char buff[4];
	int ret = 0;
	int size_original = size;

	while (size > 4) {
		copy_from_user(buffer, buff, 4);
		ret += sys_write_console(buff,4);
		buffer += 4;
		size -= 4;
	}
	copy_from_user(buffer, buff, size);
	ret += sys_write_console(buff,size);
	if(ret != size_original) return -ENODEV;
	else {
		return ret;
	}
}

int sys_gettime() {
    return zeos_ticks;
}

int sys_get_stats(int pid, struct stats *st) {
	if (!access_ok(VERIFY_WRITE, st, sizeof(struct stats))) {
		return -EFAULT;
	}
	if (pid < 0) {
    	return -EINVAL;
  	}
	
	int i;
	for (i=0; i<NR_TASKS; ++i) {
		if (task[i].task.PID == pid) {
			struct stats * st_aux = &(task[i].task.st);
			copy_to_user(st_aux, st, sizeof(struct stats));
			return 0;
		}
	}
	return -ESRCH;
}


int sys_sem_init(int n_sem, unsigned int value) {
  if (n_sem < 0 || n_sem >= NR_SEMAPHORES) return -EINVAL;//EINVAL?
  if (sem[n_sem].owner_pid != -1) return -EBUSY;
  sem[n_sem].owner_pid = current()->PID;
  sem[n_sem].counter = value;
  sem[n_sem].sem_id = ++SEM_ID;
  INIT_LIST_HEAD(&(sem[n_sem].blocked_queue));
  return 0;
}

int sys_sem_wait(int n_sem) {  
  if (n_sem < 0 || n_sem >= NR_SEMAPHORES) return -EINVAL;;
  if (sem[n_sem].owner_pid == -1) return -EINVAL;
  if (sem[n_sem].counter > 0) {
    --sem[n_sem].counter;
    return 0;
  }
  else {
    // block process
    current()->sem_id = sem[n_sem].sem_id;
    update_current_state_rr(&(sem[n_sem].blocked_queue));
    sched_next_rr();
	// if semaphore still exists, 0; if it doesn't, -1.
	// we return here when we are unblocked
	if (sem[n_sem].owner_pid < 0 || sem[n_sem].sem_id != current()->sem_id) {
	    return -1;
	}
	return 0;
  }
}

int sys_sem_signal(int n_sem) {   
  if (n_sem < 0 || n_sem >= NR_SEMAPHORES) return -EINVAL;
  if (sem[n_sem].owner_pid == -1) return -EINVAL;
  if (list_empty(&(sem[n_sem].blocked_queue))) {
    ++sem[n_sem].counter;
  }
  else {
    // unblock first process
    struct list_head * blocked_process = list_first(&(sem[n_sem].blocked_queue));
    list_del(blocked_process);
    list_add_tail(blocked_process, &readyqueue);
  }
  return 0;
}

int sys_sem_destroy(int n_sem) {
  if (n_sem < 0 || n_sem > NR_SEMAPHORES) return -EINVAL;;
  if (sem[n_sem].owner_pid == -1) return -EINVAL;
  if (current()->PID != sem[n_sem].owner_pid) return -EPERM;
  sem[n_sem].owner_pid = -1;
  sem[n_sem].sem_id = -1;
  current()->sem_id = -1;
  // unblock all processes
  while (!list_empty(&(sem[n_sem].blocked_queue))) {
    	struct list_head * blocked_process = list_first(&(sem[n_sem].blocked_queue));
    	list_del(blocked_process);
    	list_add_tail(blocked_process, &readyqueue);
  }
  return 0;
}


int sys_clone(void(*function)(void), void *stack){
	//pointer control error
	 if (!access_ok(VERIFY_WRITE, stack,4) || !access_ok(VERIFY_READ, function, 4)){
		return -EFAULT;
  	}
	//see if there's empty tsks;
	if (list_empty(&freequeue)) {
		return -ENOMEM;
	}

	//PCB
	struct list_head *pointer = list_first(&freequeue);
	list_del(pointer);
	union task_union* child_thread = (union task_union*)list_head_to_task_struct(pointer);
	union task_union* parent_thread = (union task_union*)current();
	
	copy_data(parent_thread,child_thread,sizeof(union task_union)); //union to child
	
	child_thread->stack[KERNEL_STACK_SIZE-5] = (unsigned long)function;
	child_thread->stack[KERNEL_STACK_SIZE-2] = (unsigned long)stack;	
	child_thread->stack[KERNEL_STACK_SIZE-18] = (unsigned long)(&ret_from_fork);
  	child_thread->stack[KERNEL_STACK_SIZE-19] = 0; //no cal
  	child_thread->task.stack_address = (int *)&child_thread->stack[KERNEL_STACK_SIZE-19];
	
	init_stats((struct task_struct *)child_thread);	
	//incrementar num ref al directorio
	int pos = calc_DIR(&parent_thread->task);
	++dir_occupied[pos];
	
	child_thread->task.PID = ++PID_COUNT;
	list_add_tail(&(child_thread->task.list),&readyqueue);
	return child_thread->task.PID;
}


int sys_read(int fd, char *buf, int count) {
    //read() attempts to read up to 'count' bytes from file descriptor 'fd' 
    //into the buffer dtarting at 'buf'
    if (!access_ok(VERIFY_WRITE, buf, count)) return -EFAULT;
    if(check_fd(fd, LECTURA) != 0) return -EBADF;
    if (count <= 0) return -EINVAL;
    return sys_read_keyboard(fd, buf, count);
}

void *sys_sbrk(int increment) {
	struct task_struct * process = current();
    if (process->heap_address == NULL) {
        int frame = alloc_frame();
        if (frame < 0) return frame;
        set_ss_pag(get_PT(process), HEAP_START_PAGE, frame);
        process->heap_address = HEAP_START;
        process->heap_num_pages = 1;
    }
    if (increment == 0) return (process->heap_address + process->heap_bytes);
    
    else if (increment > 0) {
        void * current_heap_address = process->heap_address + process->heap_bytes;
        if ((process->heap_bytes)%PAGE_SIZE + increment < PAGE_SIZE) {
            process->heap_bytes += increment;
        }
        else {
            process->heap_bytes += increment;
            while((process->heap_num_pages*PAGE_SIZE) < process->heap_bytes) {
                int frame = alloc_frame();
                if (frame < 0) {
                    process->heap_bytes -= increment;
                    while((process->heap_num_pages*PAGE_SIZE)-process->heap_bytes > PAGE_SIZE) {
                        int logical_page = HEAP_START_PAGE + process->heap_num_pages - 1;
                        free_frame(get_frame(get_PT(process),logical_page));
                        del_ss_pag(get_PT(process), logical_page);
                        --process->heap_num_pages;
                    }
                    return frame;
                }
                set_ss_pag(get_PT(process), (HEAP_START_PAGE + process->heap_num_pages), frame);
                ++process->heap_num_pages;
            }
        }
        return current_heap_address;
    }
    else {
    	if (process->heap_bytes + increment <= 0) process->heap_bytes = 0;
    	else process->heap_bytes += increment;
        while((process->heap_num_pages*PAGE_SIZE)-process->heap_bytes > PAGE_SIZE) {
            int logical_page = HEAP_START_PAGE + process->heap_num_pages - 1;
            free_frame(get_frame(get_PT(process),logical_page ));
            del_ss_pag(get_PT(process), logical_page);
            --process->heap_num_pages;
        }
        return process->heap_address + process->heap_bytes;
    }
}
