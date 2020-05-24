#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>




int main(int argc, char** argv) {

	int fds[2];
	
	char* server = "/tmp/server";
	char* cliente = "/tmp/cliente";
	
	int fd;
	int rd;
 	
	write(1,"Welcome folk\n",strlen("Welcome folk\n"));	
 	
	mkfifo(cliente,0666);
	
	fds[0] = open(server, O_RDONLY);
	fds[1] = open(cliente, O_WRONLY);

	char buffer[80];

	write(fds[1],"cliente",strlen(cliente));
	perror("erro : meh");
	read(fds[0],buffer,sizeof(buffer));
	perror("reee");
	write(fds[1],buffer,sizeof(buffer));

	close(fds[0]);
	close(fds[1]);
	unlink(cliente);

	



	return 1;

}
