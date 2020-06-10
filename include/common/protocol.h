#ifndef SO_PROJETO_PROTOCOL_H
#define SO_PROJETO_PROTOCOL_H


#define CLIENTPIPES_LEN 39
#define SERVER_ACK "Ready for input"
#define SERVER_ACK_LEN sizeof(SERVER_ACK)
#define CLOSE "\x90"
#define WRITE_LITERAL(fd, str) write(fd, str, sizeof(str))
#define END_OF_MESSAGE(fd) write(fd, "", sizeof(""))

ssize_t readMsg(int fildes, char *buf, size_t nbyte);
ssize_t readln(int fildes, char *buf, size_t nbyte);


#endif //SO_PROJETO_PROTOCOL_H
