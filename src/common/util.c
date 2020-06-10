#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

/*
int main(int argc, char **argv)
{
	if (!strcmp(argv[1],"-i")){
		// Introduction 
		write(1,"Alright folk, you are needing help right? Here's a list of avalaible commands!\n\n",strlen("Alright folk, you are needing help right? Here's a list of avalaible commands!\n\n"));
		
		// Tempo Inactividade 
		write(1," -i <argumento1>, tempo-inactividade <argumento1>	Define o tempo máximo, em segundos, de inactividade de comunicação num pipe anónimo\n",strlen(" -i <argumento1>, tempo-inactividade <argumento1>	Define o tempo máximo, em segundos, de inactividade de comunicação num pipe anónimo\n"));
		
		// Tempo Execução 
		write(1," -m <argumento1>, tempo-execucao <argumento1>		Define o tempo máximo (segundos) de execução de uma tarefa\n",strlen(" -m <argumento1>, tempo-execucao <argumento1>		Define o tempo máximo (segundos) de execução de uma tarefa\n"));
		
		// Executar Tarefas 
		write(1," -e <p1|p2|...|pn>, executar <p1|..|pn>			Executa uma tarefa da linha de comandos\n",strlen(" -e <p1|p2|...|pn>, executar <p1|..|pn>			Executa uma tarefa da linha de comandos\n"));
		
		// Listar Tarefas 
		write(1," -l, listar 						Lista as tarefas em execução\n",strlen(" -l, listar 						Lista as tarefas em execução\n"));
		
		// Terminar Tarefas 
		write(1," -t n, terminar	n					Termina uma tarefa em execução\n",strlen(" -t n, terminar	n					Termina uma tarefa em execução\n"));
	
		// Historico
		write (1," -r, historico						Histórico de tarefas terminadas\n",strlen(" -r, historico						Histórico de tarefas terminadas\n"));	
	}
        return 1;

}
 */
