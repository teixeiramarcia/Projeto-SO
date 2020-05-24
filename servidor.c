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
	char buffer[80];
	int rd;
 	
	int n; 	
 	write(1,"Server is starting..\n",strlen("Server is starting..\n"));	
 	
 	pipe(fds);
	mkfifo(server,0666);
	
	
	
		
		// Só vamos ler por parte do cliente, portanto usamos o RDONLY 
		fds[0] = open(cliente,O_RDONLY);
		// Como vamos ler, temos que escrever, logo usamos o WRONLY 
		fds[1] = open(server,O_WRONLY);
		
		read(fds[0],buffer,sizeof(buffer));
		write(1,buffer,sizeof(buffer));		
		if(strcmp("teste",buffer)==0){
			fd = open(buffer,O_WRONLY);

			if(fork()==0){
				dup2(fds[1],1);
				close(fds[1]);
				write(1,"teste",strlen("teste"));
				write(fd,buffer,n);
				close(fds[0]);
				close(fds[1]);
			}

			else {
				dup2(fds[0],0);
				n = read(fds[0],buffer,sizeof(buffer));
				write(fd,buffer,n);
				close(fds[0]);
				close(fds[1]);				
							
			}
			
		}	
		
		memset(buffer,0,sizeof(buffer));
		close(fd);
		close(fds[0]);
		close(fds[1]);
	
	
	
	unlink(server);
	return 1;

}
