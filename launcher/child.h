#ifndef __LAUNCHER_CHILD_H__
#define __LAUNCHER_CHILD_H__

struct child {
	pid_t		pid, ppid;
	int		fd;
	int		remote_fd;
	int		state;
	int		exit_code;
};

enum {
	CS_NEW = 0,	/* created */
	CS_RUNNING,	/* initialized */
	CS_EXITING,	/* about to exit */
	CS_DONE,
};

struct runqueue {
	struct child	**array;
	int		size;
};

/*
 * maximum for the nchildren; since children can do all sorts
 * of setuid things, RLIMIT_NPROC is not really applicable here
 */
#define CHILDREN_MAX INT_MAX

extern struct runqueue runners, waiters;

struct child *child_new(pid_t pid, pid_t ppid);
int child_find_idx_by_pid(struct runqueue *rq, pid_t pid);
struct child *child_find_by_pid(struct runqueue *rq, pid_t pid);
void child_moveon_by_idx(int idx);
void child_free(struct child *child);
void children_wait(void);
void runqueue_append(struct runqueue *rq, struct child *child);
void runqueue_remove(struct runqueue *rq, int idx);
void runqueue_clean(struct runqueue *rq);
void runqueue_list(struct runqueue *rq);

#endif /* __LAUNCHER_CHILD_H__ */
