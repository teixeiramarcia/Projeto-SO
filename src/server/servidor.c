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
#include <stdbool.h>

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
            close(input);
        } else {
            assert(setsid() == getpid());
        }
        dup2(output, 1);
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


    write(fdOut, tid, strlen(tid) + 1);
    free(tid);

    write(fdO, "-> ", 3);
    write(fdO, buf, strlen(buf));
    write(fdO, "\n", 1);

    return fdO;
}

void finishExecution(int *p, int fdO) {
    int n;
    char results[BUFSIZE];

    close(p[POSESCRITA]);

    while ((n = read(p[POSLEITURA], results, BUFSIZE)) > 0) {
        write(fdO, results, n);
    }
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
        close(outP1);
        close(inP2);
        _exit(0);
    }
    return pid;
}

void executeCommands(char *buf, int fdOut, long time) {
    long long tID = nextTID++;
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
        pid_t pid;
        if(j%2 == 0) {
            pid = executeCommand(strtoks[j], input, pipes[j][POSESCRITA]);
            if (input == -1) firstPid = pid;
        } else {
            pid = middleMan(input, pipes[j][POSESCRITA], time);
        }
        if (input != -1) setpgid(pid, firstPid);
        close(pipes[j][POSESCRITA]);
        if (input != -1) close(input);

        input = pipes[j][POSLEITURA];
    }
    finishExecution(pipes[numPipes], fdO);
}

void sendOutput(char *buf, int fdOut) {
    char *filename = malloc(sizeof(char) * (strlen("/tmp/") + strlen(buf) + 1));
    sprintf(filename, "/tmp/%s", buf);

    int file = open(filename, O_RDONLY);
    char buffer[BUFSIZE];
    int lido;

    while ((lido = read(file, buffer, BUFSIZE)) > 0) {
        write(fdOut, buffer, lido);
    }
    END_OF_MESSAGE(fdOut);
    free(filename);
    close(file);
}

void printHistory(int fdOut) {
    for (int i = 0; i < nextTID; ++i) {
        char *filename = malloc(sizeof(char) * (strlen("/tmp/") + countNumberOfChars(i) + 1));
        sprintf(filename, "/tmp/%d", i);

        int file = open(filename, O_RDONLY);
        char buf[BUFSIZE];
        int lido = readln(file, buf, BUFSIZE);

        char *printLine = malloc(sizeof(char) * (lido + countNumberOfChars(i) + strlen("#: ") + 1));
        sprintf(printLine, "#%d: %s", i, buf);
        write(fdOut, printLine, strlen(printLine));

        free(printLine);
        close(file);
        free(filename);
    }
    END_OF_MESSAGE(fdOut);
}

bool startsWith(char *buf, char *string, size_t size) {
    size_t len = strlen(string);
    if(len > size) {
        return false;
    }
    return strncmp(buf, string, len) == 0;
}

void handleClient(char *clientPipes) {
    WRITE_LITERAL(1, "\n\nA client connected\n");

    long time = -1;
    char *out = clientPipes;
    char *in = strchr(clientPipes, ' ');
    *in = '\0';
    in += 1;

    int fdIn = open(in, O_RDWR);
    int fdOut = open(out, O_RDWR);

    WRITE_LITERAL(fdOut, SERVER_ACK);
    WRITE_LITERAL(1, "Ready for input\n");

    char buf[BUFSIZE];

    int lido;
    while ((lido = read(fdIn, buf, BUFSIZE)) > 0) {
        strip_extra_spaces(buf);
        if (startsWith(buf, "exit", lido)) {
            WRITE_LITERAL(fdOut, CLOSE);
            close(fdIn);
            close(fdOut);
            break;
        } else if (startsWith(buf, "output ", lido)) {
            sendOutput(buf + 7, fdOut);
        } else if (startsWith(buf, "historico", lido)) {
            printHistory(fdOut);
        } else if (startsWith(buf, "tempo-inatividade ", lido)) {
            char *endpointer = NULL;
            long seconds = strtol(buf + strlen("tempo-inatividade "), &endpointer, 10);
            if (seconds == 0 && (buf + strlen("tempo-inatividade ")) == endpointer) {
                WRITE_LITERAL(fdOut, "Not a valid number, try again.\n");
            } else {
                WRITE_LITERAL(fdOut, "Got it, thanks!\n");
                time = seconds;
            }
        }
        else {
            executeCommands(buf, fdOut, time);
        }
    }
}

int main() {
    WRITE_LITERAL(1, "Starting server\n");
    char *serverPipeName = "/tmp/server";
    mkfifo(serverPipeName, READWRITE);

    WRITE_LITERAL(1, "Opening serverPipe\n");
    char clientPipes[CLIENTPIPES_LEN];
    int serverPipe = open(serverPipeName, O_RDWR);

    WRITE_LITERAL(1, "Reading from serverPipe\n");
    while (read(serverPipe, clientPipes, CLIENTPIPES_LEN) == CLIENTPIPES_LEN) {
        handleClient(clientPipes);
    }
    close(serverPipe);
}
