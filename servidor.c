#include <sys/stat.h>

#define READWRITE 0666

int main(int argc, char **argv)
{
        char *serverPipe = "/tmp/server";
        mkfifo(serverPipe, READWRITE);
}
