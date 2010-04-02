#ifndef RANDOMCRASH_CONFIG_H
#define RANDOMCRASH_CONFIG_H

#ifndef LRC_CONFIG_ENV
#define LRC_CONFIG_ENV "LRC_CONFIG"
#endif

void lrc_configure(void);
int lrc_conf_int(int what);
int lrc_conf_str(int what);

enum {
	CONF_LOGDIR = 0,
	CONF_NOCRASH,
};

#endif /* RANDOMCRASH_CONFIG_H */

