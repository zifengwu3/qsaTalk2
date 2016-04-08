#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

int opentty(char * tty, int fd);
void closetty(int fd);

int opentty(char * tty, int fd) {

    fd = open (tty, O_RDONLY);
    printf("%s", tty);
    if (isatty(fd)) {
        printf("is a tty.\n");
        printf("ttyname = %s \n",ttyname(fd));
        return 1;
    } else {
        printf("is not a tty\n");
        return 0;
    }
}

void closetty(int fd) {
    close(fd);
    return;
}
