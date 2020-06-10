#include <stddef.h>
#include <ntsid.h>
#include <zconf.h>

ssize_t readMsg(int fildes, char *buf, size_t nbyte) { //TODO - IDK WHAT THIS IS FOR
    for (size_t i = 0; i < nbyte; i++) {
        char bufI;
        int rd = read(fildes, &bufI, 1);

        if (rd == 0) {
            return i;
        }

        buf[i] = bufI;
        if (buf[i] == '\0') {
            buf[i + 1] = '\0';
            return i + 1;
        }
    }

    return nbyte;
}

ssize_t readln(int fildes, char *buf, size_t nbyte) {
    for (size_t i = 0; i < nbyte; i++) {
        char bufI;
        int rd = read(fildes, &bufI, 1);

        if (rd == 0) {
            return i;
        }

        buf[i] = bufI;
        if (buf[i] == '\n') {
            buf[i + 1] = '\0';
            return i + 1;
        }
    }

    return nbyte;
}

