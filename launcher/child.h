#ifndef __LAUNCHER_CHILD_H__
#define __LAUNCHER_CHILD_H__

struct child {
	pid_t		pid, ppid;
	int		fd;
	int		remote_fd;
	int		state;
};

enum {
	CS_NEW = 0,	/* created */
	CS_RUNNING,	/* initialized */
	CS_EXITING,	/* about to exit */
	CS_DONE,
};

struct child *child_new(pid_t pid, pid_t ppid);
int child_find_idx_by_pid(pid_t pid);
struct child *child_find_by_pid(pid_t pid);
void child_remove_by_idx(int idx);
void child_free(struct child *child);
int children_wait(pid_t pid);

#endif /* __LAUNCHER_CHILD_H__ */
