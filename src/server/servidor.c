#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

#include "common/protocol.h"

#define READWRITE 0666
#define BUFSIZE 4096
#define POSLEITURA 0
#define POSESCRITA 1

long long nextTID = 0;

void strip_extra_spaces(char* str) {
    int i, x;
    for (i=x=0; str[i]; ++i) {
        if (str[i] == '\n') {
            break;
        } else if (!isspace(str[i]) || (i > 0 && !isspace(str[i - 1]) && str[i - 1] != '|' && str[i + 1] != '|')) {
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

void executeCommand(char* command, int input, int output) {
    int words = countWords(command);
    char *args[words + 1];
    char* line = strtok(command, " ");
    for (int i = 0; i < words; ++i) {
        args[i] = line;
        line = strtok(NULL, " ");
    }
    args[words] = NULL;

    if (fork() == 0) {
        if (input != -1) {
            dup2(input, 0);
            close(input);
        }
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

    if (input != -1) {
        close(input);
    }
}

int countPipes(const char* buffer) {
    int counter = 0;

    for (int i = 0; buffer[i]; i++) {
        if (buffer[i] == '|') {
            counter++;
        }
    }

    return counter;
}

int countNumberOfChars(long long n) {
    int count = 0;
    while (n != 0) {
        n /= 10;
        ++count;
    }
    return count;
}

int createTaskFile(long long tID, int fdOut, char* buf) {
    int numCharstID = countNumberOfChars(tID);
    char *name = malloc(sizeof(char) * (4 + numCharstID));
    sprintf(name, "/tmp/%lld", tID);
    int fdO = open(name, O_CREAT | O_RDWR | O_TRUNC, READWRITE);
    free(name);

    char *tid = malloc(sizeof(char) * (14 + numCharstID));
    sprintf(tid, "nova tarefa #%lld\n", tID);

    write(fdOut, tid, strlen(tid));

    write(fdO, "> ", 2);
    write(fdO, buf, strlen(buf));
    write(fdO, "\n", 1);

    return fdO;
}

void finishExecution(int *p, int fdOut, int fdO) {
    int n;
    char results[BUFSIZE];

    close(p[POSESCRITA]);

    while ((n = read(p[POSLEITURA], results, BUFSIZE)) > 0) {
        write(fdOut, results, n);
        write(fdO, results, n);
    }

    write(fdOut, "Done\n", 5);
    close(fdOut);
    close(fdO);
    close(p[POSLEITURA]);
}

void executeCommands(char *buf, char* out) {
    long long tID = nextTID++;
    int fdOut = open(out, O_RDWR);
    int numPipes = countPipes(buf);
    int pipes[numPipes][2];
    char* strtoks[numPipes + 1];

    int fdO = createTaskFile(tID, fdOut, buf);

    strtoks[0] = strtok(buf, "|");
    for (int i = 0; i < numPipes; i++) {
        pipe(pipes[i]);
        strtoks[i+1] = strtok(NULL, "|");
    }

    int j;
    for (j = 0; j < numPipes; ++j) {
        int input = pipes[j-1][POSLEITURA];
        if(j == 0) {
            input = -1;
        }
        executeCommand(strtoks[j], input, pipes[j][POSESCRITA]);
        close(pipes[j][POSESCRITA]);
    }

    int input = pipes[j-1][POSLEITURA];
    if(j == 0) {
        input = -1;
    }
    int p[2];
    pipe(p);

    executeCommand(strtoks[j], input, p[POSESCRITA]);

    finishExecution(p, fdOut, fdO);
}

void sendOutput(char *buf, char* out) {
    int pip = open(out, O_RDWR);

    char *filename = malloc(sizeof(char) * (4 + strlen(buf)));
    sprintf(filename, "/tmp/%s", buf);

    int file = open(filename, O_RDONLY);
    char buffer[BUFSIZE];
    int lido;

    while ((lido = read(file, buffer, BUFSIZE)) > 0) {
        write(pip, buffer, lido);
    }
    write(pip, "Done\n", 5);

    free(filename);
    close(file);
    close(pip);
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

void printHistory(char *out) {
    int fdOut = open(out, O_RDWR);

    for (int i = 0; i < nextTID; ++i) {
        char *filename = malloc(sizeof(char) * (4 + countNumberOfChars(i)));
        sprintf(filename, "/tmp/%d", i);

        int file = open(filename, O_RDONLY);
        char buf[BUFSIZE];
        int lido = readln(file, buf, BUFSIZE);

        char *printLine = malloc(sizeof(char) * (lido + countNumberOfChars(i) + 3));
        sprintf(printLine, "#%d: %s", i, buf);
        write(fdOut, printLine, strlen(printLine));

        free(printLine);
        close(file);
        free(filename);
    }

    write(fdOut, "Done\n", 5);
    close(fdOut);
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

        if (strncmp(buf, "exit", 4) == 0) {
            fdOut = open(out, O_RDWR);
            write(fdOut, "Done\n", 5);
            close(fdIn);
            close(fdOut);
            break;
        } else if (strncmp(buf, "output ", 7) == 0) {
            sendOutput(buf + 7, out);
        } else if (strncmp(buf, "historico", 9) == 0) {
            printHistory(out);
        }
        else {
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
