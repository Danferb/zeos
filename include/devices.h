#ifndef DEVICES_H__
#define  DEVICES_H__

extern struct keyboardbuffer kbf;

int sys_write_console(char *buffer,int size);
int sys_read_keyboard(int fd, char *buf, int count);
int keyboard_buf_full();
int keyboard_buf_copy(int num, char *buf);
int keyboard_buf_char_count();
    
#endif /* DEVICES_H__*/
