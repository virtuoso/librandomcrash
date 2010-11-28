#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ixp.h>

static IxpServer server;

static void fs_open(Ixp9Req *r);
static void fs_walk(Ixp9Req *r);
static void fs_read(Ixp9Req *r);
static void fs_stat(Ixp9Req *r);
static void fs_write(Ixp9Req *r);
static void fs_clunk(Ixp9Req *r);
static void fs_flush(Ixp9Req *r);
static void fs_attach(Ixp9Req *r);
static void fs_create(Ixp9Req *r);
static void fs_remove(Ixp9Req *r);
static void fs_freefid(IxpFid *f);

static Ixp9Srv p9srv = {
        .open		= fs_open,
        .walk		= fs_walk,
        .read		= fs_read,
        .stat		= fs_stat,
        .write		= fs_write,
        .clunk		= fs_clunk,
        .flush		= fs_flush,
        .attach		= fs_attach,
        .create		= fs_create,
        .remove		= fs_remove,
        .freefid	= fs_freefid
};

static char
        Enoperm[] = "permission denied",
        Enofile[] = "file not found",
        Ebadvalue[] = "bad value";

static char *user = "ash";

struct mypriv;

struct entry {
	char		*name;
	struct entry	*parent;
	struct entry	*children;
	mode_t		mode;
	unsigned int	ino;
	int (*open)(struct mypriv *);
	int (*read)(struct mypriv *, char *, uint32_t, uint64_t);
	int (*readdir)(struct mypriv *, struct entry *);
};

struct mypriv {
	char *name;
	struct entry *entry;
	int dirpos;
	void *data;
};

#define xmalloc(x) malloc(x)

static struct mypriv *priv_alloc(const char *name, struct entry *entry)
{
	struct mypriv *priv = xmalloc(sizeof(*priv));

	priv->name = strdup(name);
	priv->entry = entry;
	priv->dirpos = 0;
	priv->data = NULL;

	return priv;
}

static int verbosity = 0xff;
#define VERB_DEBUG	0x01

#define dbg(l, f, args...)						\
        do {								\
		if ((verbosity & (l)) == (l))				\
			fprintf(stderr, "%s: " f "\n", __func__,	\
				## args);				\
	} while (0)

static int status_read(struct mypriv *priv, char *buf, uint32_t len,
		       uint64_t off)
{
	if (priv->dirpos)
		return 0;

	priv->dirpos++;
	strcpy(buf, "Watup!\n");
	return 8;
}

static int proc_readdir(struct mypriv *priv, struct entry *entry)
{
	char *name = priv->data;

	if (name) {
		free(name);
		priv->data = NULL;
	}

	if (priv->dirpos < 0 || priv->dirpos > 10)
		return 0;

	asprintf(&name, "proc_%d[%d]", priv->dirpos, priv->dirpos);
	priv->data = entry->name = name;
	entry->mode = 0755 | S_IFDIR;
	entry->parent = priv->entry;
	entry->children = NULL;
	entry->ino = 10000 + priv->dirpos;
	priv->dirpos++;

	return 1;
}

static struct entry entries[];

static struct entry root = {
	.name		= "/",
	.mode		= 0755 | S_IFDIR,
	.parent		= NULL,
	.children	= entries,
	.ino		= 2,
};

static struct entry entries[] = {
	{
		.name	= "ctl",
		.mode	= 0644 | S_IFREG,
		.parent	= &root,
		.ino	= 3,
	},
	{
		.name	= "status",
		.mode	= 0644 | S_IFREG,
		.parent	= &root,
		.ino	= 4,
		.read	= status_read,
	},
	{
		.name	= "proc",
		.mode	= 0755 | S_IFDIR,
		.parent	= &root,
		.ino	= 5,
		.readdir = proc_readdir,
	},
	{ .parent = NULL },
};

static struct entry *lookup_children(struct entry *entry, char *name)
{
	int i = 0;

	if (!entry->children)
		return NULL;

	for (i = 0; entry->children[i].parent; i++)
		if (!strcmp(entry->children[i].name, name))
			return &entry->children[i];

	return NULL;
}

static void fs_open(Ixp9Req *r)
{
	struct mypriv *priv = r->fid->aux;

	dbg(VERB_DEBUG, "opening %s [%s]\n", priv->name, priv->entry->name);

	if (priv->entry->open)
		priv->entry->open(priv);

	respond(r, NULL);
}

static void fs_walk(Ixp9Req *r)
{
	struct mypriv *priv = r->fid->aux;
	char name[PATH_MAX];
	struct entry *entry = priv->entry;
	int i;

	dbg(VERB_DEBUG, "enter %s", priv->name);
	strcpy(name, "/");

	for (i = 0; i < r->ifcall.twalk.nwname; i++) {
		struct entry *sub;

		if (!strcmp(r->ifcall.twalk.wname[i], "/") && entry == &root)
			sub = &root;
		else {
			sub = lookup_children(entry, r->ifcall.twalk.wname[i]);
			if (!sub) {
				respond(r, Enofile);
				return;
			}
			strcat(name, sub->name);
		}

                r->ofcall.rwalk.wqid[i].type = sub->mode&S_IFMT;
                r->ofcall.rwalk.wqid[i].path = sub->ino;
		entry = sub;
	}

	dbg(VERB_DEBUG, "=> '%s'", name);
	r->newfid->aux = priv_alloc(name, entry);
	r->ofcall.rwalk.nwqid = i;
	respond(r, NULL);
}

static entry_stat(struct entry *entry, IxpStat *s)
{
        s->type = 0;
        s->dev = 0;
        s->qid.type = entry->mode & S_IFMT;
        s->qid.path = entry->ino;
        s->qid.version = 0;
        s->mode = entry->mode & 0777;
        if (S_ISDIR(entry->mode)) {
                s->mode |= P9_DMDIR;
                s->qid.type |= P9_QTDIR;
        }
        s->atime = 0;
        s->mtime = 0;
        s->length = 0;
        s->name = entry->name;
        s->uid = user;
        s->gid = user;
        s->muid = user;
}

static void fs_read(Ixp9Req *r)
{
	struct mypriv *priv = r->fid->aux;
	struct entry *entry = priv->entry;
	int size, n;
	IxpStat s;
	IxpMsg m;
	char *buf;

	dbg(VERB_DEBUG, "enter");
	if (S_ISDIR(entry->mode)) {
		size = r->ifcall.tread.count;
                buf = ixp_emallocz(size);
                m = ixp_message((uchar*)buf, size, MsgPack);
		if (entry->readdir) {
			struct entry current;

			n = entry->readdir(priv, &current);
			if (n) {
				entry_stat(&current, &s);
				n = ixp_sizeof_stat(&s);
				ixp_pstat(&m, &s);
			}
		} else if (entry->children[priv->dirpos].parent) {
			entry_stat(&entry->children[priv->dirpos++], &s);
			n = ixp_sizeof_stat(&s);
			ixp_pstat(&m, &s);
		} else
			n = 0;

		r->ofcall.rread.count = n;
                r->ofcall.rread.data = (char*)m.data;
                respond(r, NULL);
	} else {
		if (!entry->read) {
			respond(r, Enoperm);
			return;
		}

		r->ofcall.rread.data = ixp_emallocz(r->ifcall.tread.count);
		if (!r->ofcall.rread.data) {
			respond(r, NULL); /* XXX */
			return;
		}

		r->ofcall.rread.count = entry->read(priv,
						    r->ofcall.rread.data,
						    r->ofcall.tread.count,
						    r->ofcall.tread.offset);
		respond(r, NULL);
	}
}

static void fs_stat(Ixp9Req *r)
{
	struct mypriv *priv = r->fid->aux;
	IxpStat s;
	IxpMsg m;
        uchar *buf;
	int size;

	dbg(VERB_DEBUG, "enter '%s'", priv->name);
	if (!*priv->name) {
		respond(r, NULL);
		return;
	}
	entry_stat(priv->entry, &s);
	r->fid->qid = s.qid;
        r->ofcall.rstat.nstat = size = ixp_sizeof_stat(&s);
	buf = ixp_emallocz(size);

        m = ixp_message(buf, size, MsgPack);
        ixp_pstat(&m, &s);

        r->ofcall.rstat.stat = m.data;
        respond(r, NULL);
}

static void fs_write(Ixp9Req *r)
{
	dbg(VERB_DEBUG, "enter");
}

static void fs_clunk(Ixp9Req *r)
{
	struct mypriv *priv = r->fid->aux;

	dbg(VERB_DEBUG, "enter %s", priv->name);
	priv->dirpos = 0;
	if (priv->data) {
		free(priv->data);
		priv->data = NULL;
	}

	respond(r, NULL);
}

static void fs_flush(Ixp9Req *r)
{
	dbg(VERB_DEBUG, "enter");
}

static void fs_attach(Ixp9Req *r)
{
	r->fid->qid.type = P9_QTDIR;
        r->fid->qid.path = (uintptr_t)r->fid;
        r->ofcall.rattach.qid = r->fid->qid;
	r->fid->aux = priv_alloc("/", &root);
	dbg(VERB_DEBUG, "attaching");

        respond(r, NULL);
}

static void fs_create(Ixp9Req *r)
{
	dbg(VERB_DEBUG, "enter");
}

static void fs_remove(Ixp9Req *r)
{
	dbg(VERB_DEBUG, "enter");
}

static void fs_freefid(IxpFid *f)
{
	struct mypriv *priv = f->aux;

	if (!priv)
		return;

	dbg(VERB_DEBUG, "enter %s", priv->name);
	free(priv->name);
	free(priv);
}

int main()
{
	IxpConn *acceptor;
	int fd = ixp_announce("unix!/tmp/lrc.ixp");
	//int fd = ixp_announce("tcp!10.1.0.103!6564");

	if (fd == -1) {
		perror("ixp_announce");
		exit(EXIT_FAILURE);
	}

	acceptor = ixp_listen(&server, fd, &p9srv, serve_9pcon, NULL);
	ixp_serverloop(&server);
	return 0;
}
