#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define READWRITE 0666

int main(int argc, char **argv)
{
        write(1, "Starting server\n", 16);
        char *serverPipeName = "/tmp/server";
        mkfifo(serverPipeName, READWRITE);

        write(1, "Opening serverPipe\n", 19);
        char clientPipes[39];
        int serverPipe = open(serverPipeName, O_RDWR);

        write(1, "Reading from serverPipe\n", 24);
        while (read(serverPipe, clientPipes, 39) > 0) {
                write(1, "\n\nA client connected\n", 21);
                write(1, clientPipes, 39);
        }
}
