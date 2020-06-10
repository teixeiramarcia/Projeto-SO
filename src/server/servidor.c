#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/select.h>
#include <assert.h>
#include <signal.h>

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

pid_t executeCommand(char* command, int input, int output) {
    int words = countWords(command);
    char *args[words + 1];
    char* line = strtok(command, " ");
    for (int i = 0; i < words; ++i) {
        args[i] = line;
        line = strtok(NULL, " ");
    }
    args[words] = NULL;
    pid_t pid = fork();
    if (pid == 0) {
        if (input != -1) {
            dup2(input, 0);
            fprintf(stderr, "C [%d] close %d\n", getpid(), input);
            close(input);
        } else {
            assert(setsid() == getpid());
        }
        dup2(output, 1);
        fprintf(stderr, "C [%d] close %d\n", getpid(), output);
        close(output);
        execvp(args[0], args);
        _exit(1);
    }
    return pid;
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

pid_t middleMan(int outP1, int inP2, long time) {
    pid_t pid = fork();
    if(pid == 0) {
        char buf[BUFSIZE];
        while(1) {
            if (time > 0) {
                struct timeval timeout;
                timeout.tv_sec = time;
                timeout.tv_usec = 0;
                fd_set set;
                FD_SET(outP1, &set);
                int res = select(outP1+1, &set, NULL, NULL, &timeout);
                if (res == 0) {
                    kill(0, SIGTERM);
                }
            }
            int lido = read(outP1, buf, BUFSIZE);
            if(lido > 0) {
                write(inP2, buf, lido);
            } else {
                break;
            }
        }
        fprintf(stderr, "[%d] close %d\n", getpid(), outP1);
        close(outP1);
        fprintf(stderr, "[%d] close %d\n", getpid(), inP2);
        close(inP2);
        _exit(0);
    }
    return pid;
}

void executeCommands(char *buf, char* out, long time) {
    long long tID = nextTID++;
    int fdOut = open(out, O_RDWR);
    int numPipes = countPipes(buf) * 2;
    int pipes[numPipes + 1][2];
    char *strtoks[numPipes + 1];

    int fdO = createTaskFile(tID, fdOut, buf);

    for (int l = 0; l <= numPipes; ++l) {
        pipes[l][0] = 0;
        pipes[l][1] = 0;
    }

    strtoks[0] = strtok(buf, "|");
    for (int i = 1; i <= numPipes; i++) {
        if(i%2 == 0) {
            strtoks[i] = strtok(NULL, "|");
        } else {
            strtoks[i] = NULL;
        }
    }
    int input = -1;
    pid_t firstPid;
    for (int j = 0; j <= numPipes; ++j) {
        pipe(pipes[j]);
        fprintf(stderr, "creating %d, %d\n", pipes[j][0], pipes[j][1]);
        pid_t pid;
        if(j%2 == 0) {
            fprintf(stderr, "executing: %s\n", strtoks[j]);
            pid = executeCommand(strtoks[j], input, pipes[j][POSESCRITA]);
            if (input == -1) firstPid = pid;
        } else {
            fprintf(stderr, "executing middleman\n");
            pid = middleMan(input, pipes[j][POSESCRITA], time);
        }
        if (input != -1) setpgid(pid, firstPid);
        fprintf(stderr, "close %d\n", pipes[j][1]);
        close(pipes[j][POSESCRITA]);
        fprintf(stderr, "close %d\n", input);
        if (input != -1) close(input);

        input = pipes[j][POSLEITURA];
    }
    finishExecution(pipes[numPipes], fdOut, fdO);
}

void sendOutput(char *buf, char* out) {
    int pip = open(out, O_RDWR);

    char *filename = malloc(sizeof(char) * (6 + strlen(buf)));
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

    long time = -1;
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
        } else if (strncmp(buf, "tempo-inatividade", 17) == 0) {
            fdOut = open(out, O_WRONLY);
            char *endpointer = NULL;
            long seconds = strtol(buf + 18, &endpointer, 10);
            if (seconds == 0 && (buf + 18) == endpointer) {
                write(fdOut, "Dude, thats not a  number\n", 26);
            } else {
                write(fdOut, "Ta\n", 3);
                time = seconds;
            }
            close(fdOut);
        }
        else {
            executeCommands(buf, out, time);
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
