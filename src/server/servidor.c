#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "common/protocol.h"

#define READWRITE 0666
#define BUFSIZE 4096

void strip_extra_spaces(char* str) {
    int i, x;
    for (i=x=0; str[i]; ++i) {
        if (str[i] == '\n') {
            break;
        } else if (!isspace(str[i]) || (i > 0 && !isspace(str[i - 1]))) {
            str[x++] = str[i];
        }
    }
    str[x] = '\0';
}

int countWords(const char *command) {
    int counter = 1;
    for (int i = 0; command[i]; ++i) {
        if(command[i] == ' ') {
            counter++;
        }
    }
    return counter;
}

void executeCommand(char* command, int output) {
    int words = countWords(command);
    char *args[words + 1];
    char* line = strtok(command, " ");
    for (int i = 0; i < words; ++i) {
        args[i] = line;
        line = strtok(NULL, " ");
    }
    args[words] = NULL;

    if (fork() == 0) {
        dup2(output, 1);
        close(output);
        execvp(args[0], args);
        _exit(1);
    }

    int status;
    wait(&status);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        write(1, "Error: command execution failed.\n", 33);
    }

    write(output, "Done\n", 5);
    close(output);
}

void executeCommands(char *buf, char* out) {
    int fdOut = open(out, O_RDWR);
    char* line = strtok(buf, "|");

    executeCommand(line, fdOut);
}

void handleClient(char clientPipes[39]) {
    write(1, "\n\nA client connected\n", 21);

    char *out = clientPipes;
    char *in = strchr(clientPipes, ' ');
    *in = '\0';
    in += 1;

    int fdOut = open(out, O_RDWR);
    int fdIn = open(in, O_RDWR);

    write(fdOut, SERVER_ACK, SERVER_ACK_LEN);
    close(fdOut);
    write(1, "Ready for input\n", 16);

    char buf[BUFSIZE];

    while (read(fdIn, buf, BUFSIZE) > 0) {
        strip_extra_spaces(buf);

        if (strcmp(buf, "exit\n") == 0) {
            fdOut = open(out, O_RDWR);
            write(fdOut, "Bye!\n", 5);
            close(fdIn);
            close(fdOut);
            break;
        } else {
            executeCommands(buf, out);
        }
    }
}

int main() {
    write(1, "Starting server\n", 16);
    char *serverPipeName = "/tmp/server";
    mkfifo(serverPipeName, READWRITE);

    write(1, "Opening serverPipe\n", 19);
    char clientPipes[39];
    int serverPipe = open(serverPipeName, O_RDWR);

    write(1, "Reading from serverPipe\n", 24);
    while (read(serverPipe, clientPipes, 39) == 39) {
        handleClient(clientPipes);
    }

    close(serverPipe);
}
