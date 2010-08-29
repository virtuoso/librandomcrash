#include <sys/types.h>
#include <dirent.h>

int main()
{
	DIR *d = opendir("/");
	return !!d;
}
