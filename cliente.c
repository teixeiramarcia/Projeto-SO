#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define READWRITE 0666
#define BUFSIZE 1024 * 1024

char *randomPipeName()
{
        int string_length = 10;
        char *string = malloc((string_length + 1) * sizeof(char));

        srand(time(NULL));

        for (int i = 0; i < string_length; ++i) {
                string[i] = 'a' + rand() % 24;
        }

        return string;
}

void createPipes(char *pipeName, char *pipeIn, char *pipeOut)
{
        strcat(pipeIn, "/tmp/");
        strcat(pipeIn, pipeName);
        strcat(pipeIn, "-in");
        strcat(pipeOut, "/tmp/");
        strcat(pipeOut, pipeName);
        strcat(pipeOut, "-out");

        mkfifo(pipeIn, READWRITE);
        mkfifo(pipeOut, READWRITE);
}

ssize_t readln(int fildes, char *buf, size_t nbyte)
{
        for (int i = 0; i < nbyte; i++) {
                char bufI;
                int rd = read (fildes, &bufI, 1);

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

void sendToServer(char *input, int fdIn, int fdOut)
{
        write(fdOut, input, strlen(input));

        char output[BUFSIZE];
        int size = readln(fdIn, output, BUFSIZE);

        write(1, output, size);
}

void sendCommands(int fdIn, int fdOut)
{
        int flagcycle = 1;
        while(flagcycle) {
                char input[BUFSIZE];
                write(1, "# ", 2);
                int s = readln(0, input, BUFSIZE);

                if (strcmp(input, "exit\n") == 0 || s < 0) {
                        flagcycle = 0;
                }

                sendToServer(input, fdIn, fdOut);
        }
}

void start(char *in, char *out)
{
        write(1, "Opening server pipe\n", 20);
        int serverPipe = open("/tmp/server", O_WRONLY);
        char buffer[39] = "";

        strcat(buffer, in);
        strcat(buffer, " ");
        strcat(buffer, out);

        int fdIn = open(in, O_RDWR);
        int fdOut = open(out, O_RDWR);

        write(1, "Sending pipe names to server\n", 29);
        write(serverPipe, buffer, 39);

        close(serverPipe);

        char buf[16];
        if(read(fdIn, buf, 16) > 0) {
            sendCommands(fdIn, fdOut);
        }

        close(fdIn);
        close(fdOut);
}

int main(int argc, char **argv)
{
        write(1, "Starting client\n", 16);
        char *pipeName = randomPipeName();

        char pipeIn[18] = "";
        char pipeOut[19] = "";

        createPipes(pipeName, pipeIn, pipeOut);

        start(pipeIn, pipeOut);

        free(pipeName);

        return 0;
}
