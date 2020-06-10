#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>

#include "common/protocol.h"

#define READWRITE 0666
#define BUFSIZE 4096

int fdIn = -1;
int fdOut = -1;

char *randomPipeName() {
    int string_length = 10;
    char *string = malloc((string_length + 1) * sizeof(char));

    srand(time(NULL));

    for (int i = 0; i < string_length; ++i) {
        string[i] = 'a' + rand() % 24;
    }

    return string;
}

void createPipes(char *pipeName, char *pipeIn, char *pipeOut) {
    sprintf(pipeIn, "/tmp/%s-in", pipeName);
    sprintf(pipeOut, "/tmp/%s-out", pipeName);

    mkfifo(pipeIn, READWRITE);
    mkfifo(pipeOut, READWRITE);
}

int sendToServer(char *input) {
    write(fdOut, input, strlen(input));

    char output[BUFSIZE];
    int size;
    do {
        size = readMsg(fdIn, output, BUFSIZE);
        if (strncmp(output, CLOSE, size) == 0) {
            return -2;
        } else {
            write(1, output, size);
        }
    } while (output[size-1] != '\0');
    return 0;
}

void sendCommands() {
    while (1) {
        char input[BUFSIZE];
        write(1, "argus$ ", 7);
        int s = readln(0, input, BUFSIZE);

        if (s <= 0 || strncmp(input, "exit\n", s) == 0) {
            sendToServer("exit\n");
            write(1, "Bye!\n", 5);
            break;
        } else if(strncmp(input, "ajuda\n", s) == 0) {
            write(1, "tempo-inactividade (em segundos)\n", 33);
            write(1, "tempo-execucao (em segundos)\n", 29);
            write(1, "executar p1 | p2 ... | pn\n", 26);
            write(1, "listar (tarefas em execução)\n", 31);
            write(1, "terminar n (tarefa n em execução)\n", 36);
            write(1, "historico (de tarefas terminadas)\n", 34);
            write(1, "output n (output produzido pela tarefa n já executada)\n", 56);
        } else if (s > 1) {
            int i = sendToServer(input);
            if (i == -2) {
                WRITE_LITERAL(1, "Bye!\n");
                close(fdIn);
                close(fdOut);
                break;
            }
        }
    }
}

void start(char *in, char *out) {
    write(1, "Opening server pipe\n", 20);
    int serverPipe = open("/tmp/server", O_WRONLY);
    char buffer[CLIENTPIPES_LEN] = "";

    strcat(buffer, in);
    strcat(buffer, " ");
    strcat(buffer, out);

    fdOut = open(out, O_RDWR);
    fdIn = open(in, O_RDWR);

    write(1, "Sending pipe names to server\n", 29);
    write(serverPipe, buffer, CLIENTPIPES_LEN);

    close(serverPipe);

    char buf[SERVER_ACK_LEN];
    if (read(fdIn, buf, SERVER_ACK_LEN) > 0 && strncmp(buf, SERVER_ACK, SERVER_ACK_LEN) == 0) {
        sendCommands();
    }

    close(fdIn);
    close(fdOut);
}

void sig_handler(int signo) {
    if (signo != SIGINT) {
        return;
    }
    if (fdIn != -1 && fdOut != -1) {
        sendToServer("exit\n");
        write(1, "\nBye!\n", 6);
        close(fdIn);
        close(fdOut);
    }
    _exit(130);
}

int main() {
    signal(SIGINT, sig_handler);

    write(1, "Starting client\n", 16);
    char *pipeName = randomPipeName();

    char pipeIn[18] = "";
    char pipeOut[19] = "";

    createPipes(pipeName, pipeIn, pipeOut);

    start(pipeIn, pipeOut);

    free(pipeName);

    return 0;
}
