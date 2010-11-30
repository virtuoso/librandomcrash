#include "util.h"
#include "child.h"

struct child *child_new(pid_t pid, pid_t ppid)
{
	struct child *child;
	int fdsv[2], i;

	if (runners.size == CHILDREN_MAX) { /* XXX: need better limit */
		error("too many child processes\n");
		/* XXX */
		return NULL;
	}

	/*fprintf(stderr, "### NULL: %d, 0x400000: %d\n",
		validate_addr(pid, 0, 0),
		validate_addr(pid, 0x400000, VA_OPT_READ | VA_OPT_EXEC));
	maps_save(pid, VA_OPT_OURS);*/

	child = xmalloc(sizeof(*child));
	child->pid = pid;
	child->ppid = ppid;

	i = socketpair(AF_UNIX, SOCK_STREAM, 0, fdsv);
	if (i) {
		error("Failed to create a pipe\n");
		exit(EXIT_FAILURE);
	}

	child->fd = fdsv[0];
	child->remote_fd = fdsv[1];
	child->state = CS_NEW;

	/* new children go to runners queue */
	runners.array = realloc(runners.array,
				sizeof(struct child *) * ++runners.size);
	runners.array[runners.size - 1] = child;

	return child;
}

int child_find_idx_by_pid(struct runqueue *rq, pid_t pid)
{
	int i;

	for (i = 0; i < rq->size; i++) {
		if (rq->array[i]->pid == pid)
			return i;
	}

	return -1;
}

struct child *child_find_by_pid(struct runqueue *rq, pid_t pid)
{
	int idx = child_find_idx_by_pid(rq, pid);

	return idx >= 0 ? rq->array[idx] : NULL;
}

/* move a child from runners to waiters */
void child_moveon_by_idx(int idx)
{
	runqueue_append(&waiters, runners.array[idx]);
	runqueue_remove(&runners, idx);
}

void child_free(struct child *child)
{
	if (!child) /* XXX: report */
		return;

	if (child->state != CS_EXITING)
		trace(DBG_CHILD, "child %d is in the wrong state %d\n",
		      child->pid, child->state);
	close(child->fd);
	close(child->remote_fd);
	xfree(child);
}

void runqueue_append(struct runqueue *rq, struct child *child)
{
	rq->array = realloc(rq->array,
				sizeof(struct child *) * ++rq->size);
	rq->array[rq->size - 1] = child;
}

void runqueue_remove(struct runqueue *rq, int idx)
{
	struct child **new_array;

	new_array = xmalloc((rq->size - 1) * sizeof(struct child *));
	memcpy(new_array, rq->array, idx * sizeof(struct child *));
	memcpy(new_array + idx, rq->array + idx + 1,
	       (rq->size - idx - 1) * sizeof(struct child *));

	xfree(rq->array);
	rq->array = new_array;
	rq->size--;
}

void runqueue_clean(struct runqueue *rq)
{
	while (rq->size) {
		child_free(rq->array[0]);
		runqueue_remove(rq, 0);
	}
}

void runqueue_list(struct runqueue *rq)
{
	int i;

	trace(DBG_CHILD, "nchildren=%d\n", rq->size);
	for (i = 0; i < rq->size; i++)
		trace(DBG_CHILD, "[child %d: state %d]\n",
		      rq->array[i]->pid, rq->array[i]->state);
	trace(DBG_CHILD, "\n");
}
