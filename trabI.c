#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>

#define MAXARGS 100
#define PIPE_READ 0
#define PIPE_WRITE 1

typedef struct command {
  char *cmd;              // string apenas com o comando
  int argc;               // número de argumentos
  char *argv[MAXARGS+1];  // vector de argumentos do comando
  struct command *next;   // apontador para o próximo comando
} COMMAND;

// variáveis globais
char* inputfile = NULL;	    // nome de ficheiro (em caso de redireccionamento da entrada padrão)
char* outputfile = NULL;    // nome de ficheiro (em caso de redireccionamento da saída padrão)
int background_exec = 0;    // indicação de execução concorrente com a mini-shell (0/1)

// declaração de funções
COMMAND* parse(char* linha);
void print_parse(COMMAND* commlist);
void execute_commands(COMMAND* commlist);
void free_commlist(COMMAND* commlist);

// include do código do parser da linha de comandos
#include "parser.c"

void handler(int signal);

int main(int argc, char* argv[]) {
  char *linha;
  COMMAND *com;

  signal(SIGCHLD, handler);
  
  while (1) {
    if ((linha = readline("my_prompt$ ")) == NULL)
      exit(0);
    if (strlen(linha) != 0) {
      add_history(linha);
      com = parse(linha);
      if (com) {
	print_parse(com);
	execute_commands(com);
	free_commlist(com);
      }
    }
    free(linha);
  }
}


void print_parse(COMMAND* commlist) {
  int n, i;

  printf("---------------------------------------------------------\n");
  printf("BG: %d IN: %s OUT: %s\n", background_exec, inputfile, outputfile);
  n = 1;
  while (commlist != NULL) {
    printf("#%d: cmd '%s' argc '%d' argv[] '", n, commlist->cmd, commlist->argc);
    i = 0;
    while (commlist->argv[i] != NULL) {
      printf("%s,", commlist->argv[i]);
      i++;
    }
    printf("%s'\n", commlist->argv[i]);
    commlist = commlist->next;
    n++;
  }
  printf("---------------------------------------------------------\n");
}

void free_commlist(COMMAND *commlist){
  // ...
  // Esta função deverá libertar a memória alocada para a lista ligada.
  // ...
  if(commlist == NULL) return;
  free_commlist(commlist->next);
  free(commlist);
}

void handler(int signal) {
  if(signal == SIGCHLD) 
    waitpid(-1, NULL, WNOHANG);
}

void execute_commands(COMMAND *commlist) {
  // ...
  // Esta função deverá "executar" a "pipeline" de comandos da lista commlist.
  // ...
  
  //pid_t pid;
  int n = 0, i;
  COMMAND *tmp = commlist;
  
  while(tmp != NULL) {
    n++;
    tmp = tmp->next;
  }
  n--;
  
  int fd[n][2];
  pid_t child[n + 1];
  
  i = 0;
  tmp = commlist;
  
  while(tmp != NULL) {
    if(i < n) {
      if(pipe(fd[i]) < 0) {
	//Pipe error
	//Pipe alert: 0 rd 1 wr
	exit(1);
      }
    }
    
    if((child[i] = fork()) < 0) {
      //Fork error
    }
    else if(child[i] == 0) {
      //Child code after fork
      if(i == 0 && inputfile != NULL) {
	int fdi;
      
	if((fdi = open(inputfile, O_RDONLY)) < 0) {
	  //Error
	  exit(1);
	}
	
	dup2(fdi, STDIN_FILENO);
	close(fdi);
      }
     
      if(i == n && outputfile != NULL) {
	int fdo;
	
	if((fdo = open(outputfile, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)) < 0) {
	  //Error
	  exit(1);
	}
	
	dup2(fdo, STDOUT_FILENO);
	close(fdo);
      }
 
      if(i < n) {
	dup2(fd[i][PIPE_WRITE], STDOUT_FILENO);
      }
      
      if(i > 0) {
	dup2(fd[i - 1][PIPE_READ], STDIN_FILENO);
	close(fd[i - 1][PIPE_READ]);
      }

      if(execvp(tmp->cmd, tmp->argv) < 0) {
	//execvp error
	exit(1);
      }
    }
    else {
      //Parent code after fork
      if(fd[i][PIPE_WRITE]) {
	close(fd[i][PIPE_WRITE]);
      }
    }

    i++;
    tmp = tmp->next;
  }
  
  if(!background_exec){
    for(i=0; i<=n; i++){
      waitpid(child[i], NULL, 0);
    }
  }
}
