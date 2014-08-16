#include <io.h>
#include <utils.h>
#include <list.h>
#include <errno.h>
#include <sched.h>

// Queue for blocked processes in I/O 
//struct list_head blocked;

int read_char_count;

int sys_write_console(char *buffer,int size)
{
  int i;
  
  for (i=0; i<size; i++)
    printc(buffer[i]);
  
  return size;
}

int keyboard_buf_full() {
	if ( (kbf.start - kbf.end == 1) || (kbf.end == BUFSIZE-1 && kbf.start == 0) ) {
		return 1;
	}
	return 0;
}

int keyboard_buf_char_count() {
	return kbf.char_count;
}

int keyboard_buf_copy(int num, char *buf) {
    if (num <= 0) return -EINVAL;
    if (buf == NULL) return -EFAULT;
    char * buf_init = buf;
    if (kbf.start < kbf.end) {
        while (num > 0 && kbf.start < kbf.end) {
        	// as we increment buf, there is no need to have position counter
           buf[0] = kbf.buf[kbf.start];
           ++kbf.start;
           --kbf.char_count;
            ++buf;
            --num;
        }
    }
    else {
        while (num > 0 && kbf.start < BUFSIZE) {
            buf[0] = kbf.buf[kbf.start];
            ++kbf.start;
            --kbf.char_count;
            ++buf;
            --num;
        }
        if (kbf.start == BUFSIZE) kbf.start = 0;
        while (num > 0 && kbf.start < kbf.end) {
            buf[0] = kbf.buf[kbf.start];
            ++kbf.start;
            --kbf.char_count;
            ++buf;
            --num;
        }
    }
    return (buf - buf_init);
}

int sys_read_keyboard(int fd, char *buf, int count) {
    if (!list_empty(&keyboardqueue)) {
    	block(&(current()->list), &keyboardqueue);
    }
    if (keyboard_buf_char_count() == 0) {
    	read_char_count = count;
    	block(&(current()->list), &keyboardqueue);
    }
    char temp_buf[BUFSIZE];
    int aux = keyboard_buf_copy(count, (char *)&temp_buf);
    if (aux < 0) return aux;
    copy_to_user(temp_buf, buf, aux);
    if (aux == count){
        return aux;
    }
    else if (aux < count) {
        //copy to the user buffer and block at keyboardqueue
        char * init_buf = buf;
        buf += aux;
        count -= aux;
        read_char_count = count;
        while (count > 0) {
        	block(&(current()->list), &keyboardqueue);
            aux = keyboard_buf_copy(count, (char *)&temp_buf);
            copy_to_user(temp_buf, buf, aux);
            buf += aux;
            count -= aux;
            read_char_count = count;
        }
        return (buf - init_buf);
    }
    else {
        //block at the beggining of keyboardqueue
        //we should not get here
        block(&(current()->list), &keyboardqueue);
        return 0;
    }    
}
