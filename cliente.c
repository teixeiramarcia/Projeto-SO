#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>

#define READWRITE 0666

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

int main(int argc, char **argv)
{
        char *pipeName = randomPipeName();

        char pipeIn[18] = "";
        char pipeOut[19] = "";

        createPipes(pipeName, pipeIn, pipeOut);

        free(pipeName);

        return 0;
}
