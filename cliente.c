#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define READWRITE 0666

void start(char *in, char *out);

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

void start(char *in, char *out)
{
        write(1, "Opening server pipe\n", 20);
        int serverPipe = open("/tmp/server", O_WRONLY);
        char buffer[39] = "";

        strcat(buffer, in);
        strcat(buffer, " ");
        strcat(buffer, out);

        write(1, "Sending pipe names to server\n", 29);
        write(serverPipe, buffer, 39);

        close(serverPipe);
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
