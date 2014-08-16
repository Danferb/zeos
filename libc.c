/*
 * libc.c 
 */

#include <libc.h>
#include<errno.h>
#include <types.h>
#include<errnodes.h>

int errno;

void itoa(int a, char *b)
{
  int i, i1;
  char c;
  
  if (a==0) { b[0]='0'; b[1]=0; return ;}
  
  i=0;
  while (a>0)
  {
    b[i]=(a%10)+'0';
    a=a/10;
    i++;
  }
  
  for (i1=0; i1<i/2; i1++)
  {
    c=b[i1];
    b[i1]=b[i-i1-1];
    b[i-i1-1]=c;
  }
  b[i]=0;
}

int strlen(char *a)
{
  int i;
  
  i=0;
  
  while (a[i]!=0) i++;
  
  return i;
}

int gettime() {
	int res = -1;
	__asm__ __volatile__ (
		"movl $10, %%eax;"	//10 es id system (sys table)
		"int $0x80;"
		"movl %%eax, %0;"
		: "=g" (res)		//sortides
		:			//entrades
		: "ax"	//sha de fer? i amb memory?	//registres modificats
	);
	if(res >= 0) return res;
	else {
		errno = -res;
		return -1;
	}
}

int write(int fd, char *buffer, int size){
	int res;
	//ebx,ecx,edx, esi, esi
	__asm__ __volatile__(
		"pushl %%ebx;"
		"movl 8(%%ebp), %%ebx;"
		"movl 12(%%ebp), %%ecx;"
		"movl 16(%%ebp), %%edx;"
		"movl $4, %%eax;"  		//identifier of the system
		"int $0x80;"			//generate the trap
		"movl %%eax, %0;"
		"popl %%ebx;"	
		:"=g"(res)	//salida 0 perque es la primera
		://memory?:
		: "bx","cx", "dx", "memory"
	);
	if(res >= 0) return res;
	else {
		errno = res*-1;
		return -1;
	}

}



void perror(void) {
  	if (errno < MIN_ERRNO_VALUE || errno > MAX_ERRNO_VALUE) errno = 11;
  	write(1,"SYSTEM ERROR MESSAGE: ",30);
  	write(1, errnodes[errno - 1], strlen(errnodes[errno - 1]));
  	write(1,"\n",1);
}

int getpid(){
	int res = 0;
	__asm__ __volatile__(
	"movl $20, %%eax;"
	"int $0x80;"
	"movl %%eax, %0;"
	: "=g" (res) 
	:
	: "ax"
	);
	
	if(res < 0){
		errno = -res; 	
		return -1;
	}
	return res;
}

int fork(){
	int res;
	__asm__ __volatile__(
	"movl $2, %%eax;"
	"int $0x80;"
	"movl %%eax, %0;"
	: "=g" (res)
	:
	:"ax"
	 );	
	if(res < 0){
		errno = -res; 	
		return -1;
	}
	return res;

}

void exit(){	
	__asm__ __volatile__(
	"movl $1, %%eax;"
	"int $0x80;"
	:::"ax"
	);
	return;
}

int get_stats(int pid, struct stats *st) {
	int res;
	//ebx,ecx,edx, esi, esi
	__asm__ __volatile__(
		"movl 8(%%ebp), %%ebx;"
		"movl 12(%%ebp), %%ecx;"
		"movl $35, %%eax;"  		//identifier of the system
		"int $0x80;"			//generate the trap
		"movl %%eax, %0;"	
		:"=g"(res)	//salida 0 perque es la primera
		:
		: "ax", "bx", "cx"
	);	
	if(res == 0) return res;
    else {
        errno = -res;
		return -1;	//return -ESRCH;
    }
}

int clone(void(*function)(void), void *stack){
	int res;
	//ebx,ecx,edx, esi, esi	
	__asm__ __volatile__(
	    "movl %1, %%ebx;"
        "movl %2, %%ecx;"
        "movl $19, %%eax;"
        "int $0x80;"
        "movl %%eax, %0;"
            : "=g" (res)
            : "g" (function), "g" (stack)
            : "ax", "bx","cx"
		
	    );
	if(res >= 0) return res;
	else {
		errno = res*-1;
		return -1;
	}

}

int sem_init (int n_sem, unsigned int value) {
  int res;
  __asm__ __volatile__(
		"movl 8(%%ebp), %%ebx;"
		"movl 12(%%ebp), %%ecx;"
		"movl $21, %%eax;"  		//identifier of the system
		"int $0x80;"			//generate the trap
		"movl %%eax, %0;"	
		:"=g"(res)	//salida 0 perque es la primera
		://memory?:
		: "bx","cx", "ax", "memory"
	);
	if(res >= 0) return res;
	else {
		errno = res*-1;
		return -1;
	}
}

int sem_wait (int n_sem) {
  int res;
  __asm__ __volatile__(
		"movl 8(%%ebp), %%ebx;"
		"movl $22, %%eax;"  		//identifier of the system
		"int $0x80;"			//generate the trap
		"movl %%eax, %0;"	
		:"=g"(res)	//salida 0 perque es la primera
		://memory?:
		: "bx","cx", "ax", "memory"
	);
    if(res >= 0) return res;
	else {
		errno = res*-1;
		return -1;
	}

}
int sem_signal (int n_sem) {
  int res;
  __asm__ __volatile__(
		"movl 8(%%ebp), %%ebx;"
		"movl $23, %%eax;"  		//identifier of the system
		"int $0x80;"			//generate the trap
		"movl %%eax, %0;"	
		:"=g"(res)	//salida 0 perque es la primera
		://memory?:
		: "bx","cx", "ax", "memory"
	);
	if(res >= 0) return res;
	else {
		errno = res*-1;
		return -1;
	}

}

int sem_destroy (int n_sem) {
  int res;
  __asm__ __volatile__(
		"movl 8(%%ebp), %%ebx;"
		"movl $24, %%eax;"  		//identifier of the system
		"int $0x80;"			//generate the trap
		"movl %%eax, %0;"	
		:"=g"(res)	//salida 0 perque es la primera
		://memory?:
		: "bx","cx",  "ax", "memory"
	);
	if(res >= 0) return res;
	else {
		errno = res*-1;
		return -1;
	}
}

int read(int fd, char *buf, int count){
	int res;
	//ebx,ecx,edx, esi, esi	
	__asm__ __volatile__(
	    "movl %1, %%ebx;"
        "movl %2, %%ecx;"
        "movl %3, %%edx;"
        "movl $3, %%eax;"
        "int $0x80;"
        "movl %%eax, %0;"
            : "=g" (res)
            : "g" (fd), "g" (buf), "g" (count)
            : "ax", "bx","cx", "dx"
		
	    );
	if(res >= 0) return res;
	else {
		errno = res*-1;
		return -1;
	}

}

void * sbrk (int increment) {
  int res;
  __asm__ __volatile__(
		"movl 8(%%ebp), %%ebx;"
		"movl $39, %%eax;"  		//identifier of the system
		"int $0x80;"			//generate the trap
		"movl %%eax, %0;"	
		:"=g"(res)	//salida 0 perque es la primera
		://memory?:
		: "bx","cx",  "ax", "memory"
	);
	if((int)res >= 0) return res;
	else {
		errno = ((int)res)*-1;
		return (void *)-1;
	}
}
