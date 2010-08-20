#include <sys/types.h>
#include <dirent.h>

int main()
{
    DIR *d = opendir("/");
    if (d) {
        struct dirent *de = readdir(d);
        /* Did readdir() fail? */
        return !!de;
    }

    /* opendir() failed */
    /* XXX: so is this testing opendir or readdir? // ash */
    return 0;
}
