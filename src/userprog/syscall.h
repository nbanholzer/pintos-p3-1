#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

// TODO: these are duplicative, normally defined
// in user/lib/syscall.h
typedef int pid_t;
typedef int mapid_t;

void syscall_init (void);
int sys_exit(int, int, int);

#endif /* userprog/syscall.h */
