#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define READWRITE 0666
#define BUFSIZE 1024 * 1024

void executeCommands(char *buf, int size, int fdOut)
{
        //TODO executar o(s) comando(s) existentes no buf
        write(fdOut, buf, size);
}

void handleClient(char clientPipes[39]) {
        write(1, "\n\nA client connected\n", 21);

        char* line = strtok(clientPipes, " ");
        char* out = malloc(20 * sizeof(char));
        strcat(out, strdup(line));

        line = strtok(NULL, "\0");
        char* in = malloc(20 * sizeof(char));
        strcat(in, strdup(line));

        int fdOut = open(out, O_WRONLY);
        int fdIn = open(in, O_RDONLY);

        write(1, "Ready for input\n", 16);
        write(fdOut, "Ready for input\n", 16);

        char buf[BUFSIZE];
        int size;
        int flagCycle = 1;

        while((size = read(fdIn, buf, BUFSIZE)) > 0 && flagCycle) {
                if (strcmp(buf, "exit\n") == 0) {
                        flagCycle = 0;
                        write(fdOut, "Bye!\n", 5);
                        close(fdIn);
                        close(fdOut);
                } else {
                        executeCommands(buf, size, fdOut);
                }
        }

        free(in);
        free(out);
}

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
                handleClient(clientPipes);
        }

        close(serverPipe);
}
