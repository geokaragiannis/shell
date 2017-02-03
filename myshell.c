//Name: Georgios Karagiannis (gekaragi)
//ID: 1441585
//email: gekaragi@ucsc.edu
//Class: CMPS 111-02
//Winter 2017

#include    <stdio.h>
#include    <unistd.h>
#include    <sys/types.h>
#include    <errno.h>
#include    <stdlib.h>
#include    <unistd.h>
#include    <fcntl.h>
#include    <assert.h>
#include    <string.h>
#include    <sys/wait.h>
#include    <sys/stat.h>
#include    <signal.h>
extern char **getline();


// NodeObj
typedef struct NodeObj{
    char* cmd;
    char* input;
    char* output;
    char** flags;
    int numFlags;
    struct NodeObj* next;
} NodeObj;


// Node
typedef NodeObj* Node; // Node now points to a NodeObj Object

Node newNode(char* c, char* i, char* o, char** f,int numFlags) {
    Node N = malloc(sizeof(NodeObj));
    assert(N!=NULL);
    N->cmd = c;
    N->input = i;
    N->output = o;

    N->flags = (char**)calloc(20,sizeof(char*));

    int j;
    for(j = 0; j < 20; j++){
       
      N->flags[j] = (char*)malloc(50*sizeof(char));
      N->flags[j] = NULL;
    }
    
    for(j = 0; j < numFlags; j++){
        N->flags[j] = strdup(f[j]);
      
    }

    N->numFlags = numFlags;

    N->next = NULL;
    return(N);
}


// freeNode()
// destructor for the Node type
void freeNode(Node* pN){
    if( pN!=NULL && *pN!=NULL ){
        free(*pN);
        *pN = NULL;
    }
}

// LinkedList Obj
typedef struct LinkedListObj{
    Node head;
    Node tail;
    int numItems;
} LinkedListObj;

typedef struct LinkedListObj* LinkedList;

// newLinkedList()
// constructor for the LinkedList type
LinkedList newLinkedList(void){
    LinkedList L = malloc(sizeof(LinkedListObj));
    assert(L!=NULL);
    L->head = NULL;
    L->tail = NULL;
    L->numItems = 0;
    return L;
}


// freeLinkedList()
// destructor for the LinkedList type
void freeLinkedList(LinkedList* pL){
    if( pL!=NULL && *pL!=NULL ){
        free(*pL);
        *pL = NULL;
    }
}

//insert()
// inserts a new node at the tail
void insert(LinkedList L, char* c, char* i, char* o, char** f,int numFlags){
    Node N = newNode(c,i,o,f,numFlags);

    if(L->numItems == 0){
        N->next = L->head;
        L->head = N;
        L->tail = N;
    } else{
        L->tail->next = N;
        L->tail = N;
    }
    L->numItems++;

}


//init()
//parses args and initializes the linked list
void init(char* cmd, char* in_file, char* out_file, char** flags, char** args, LinkedList L){

  int i;
  int k = 0;
  

  if(args[0]!=NULL){
          cmd = args[0];
      } else {
          exit(0);
      }

      for(i = 1; args[i] != NULL; i++) {

        int in,out,pipe;
        
          in = strcmp(args[i],"<");
          out = strcmp(args[i],">");
          pipe = strcmp(args[i],"|");
          
          if(strcmp(args[i-1], "|") == 0){
              cmd = args[i];
          }
          
          
          if(in == 0){
              if(args[i+1] != NULL){
                  in_file = args[i+1];
              } 
              
          }
          
          if(out == 0){
              if(args[i+1] != NULL){
                  out_file = args[i+1];
              }
          }
          
          int isIO;
          if(in_file == NULL && out_file == NULL){
              isIO = 1;
          } else{
              isIO = 0;
          }
          
          // if the current argument is not a command
          // and there it is not input,output or a pipe
          // and it is not an IO file, then it is a flag.
          if(strcmp(args[i], cmd) != 0 && in != 0 && out != 0 && pipe != 0 && isIO == 1){
              flags[k++] = args[i];
              int m = k-1;
          }
          
          if(pipe == 0){
              
              // transfer everything to node obj. k is the number of the flags
              insert(L, cmd, in_file, out_file, flags,k);
              
              // reset everything for the new pipe.
              int j = 0;
              in_file = NULL;
              out_file = NULL;
              cmd = NULL;
              while(flags[j] != NULL){
                  flags[j] = NULL;
                  j++;
              }
              k = 0;  
          }


      } // end of for loop

      insert(L, cmd, in_file, out_file, flags,k);

      //reset everything
      cmd = in_file = out_file = NULL;
      k = 0;
      int j = 0;
      while(flags[j] != NULL){
          flags[j] = NULL;
          j++;
      }


}

// get_whole_command()
// merges the command with its flags array
void get_whole_command(char** whole_command, Node N){


  // first element is the command
  whole_command[0] = N->cmd;

  int k,m = 0;
  // if only one element, make the last NULL
  // Else copy from N->flags
  if(N->numFlags == 0){
    whole_command[1] = NULL;
  } else{

    for(k = 0; N->flags[k] != NULL; k++){
      m = k+1;
      whole_command[m] = N->flags[k];
    }
    // last element is NULL
    whole_command[++m] = NULL;
  }

}

//error_report
// reports an error using perror
void error_report(int q, char *str){

  if(q < 0){
    perror(str);
  }
}

// execute()
// executes a command recursively 
void execute(Node N, int *p){
  int fd_in, fd_out,status;
  pid_t pid;

  int check;

  // whole_command is an array that has both the command and the flags
  // this will be the string array input for execvp. Last element
  // of whole_command must be NULL.
  char* whole_command[N->numFlags + 2];

  int i;
  for(i = 0; i < N->numFlags + 2; i++){

    whole_command[i] = (char*)malloc(50*sizeof(char));
    whole_command[i] = NULL;
  }

  get_whole_command(whole_command,N);

  // make the input of the command the output of the previous pipe command (if any)
  if(p[0] != -1){
    dup2(p[0], STDIN_FILENO);
    close(p[1]);
  }


  if(N->input !=NULL){
    fd_in = open(N->input,O_RDONLY);
    if(fd_in < 0){
      perror(N->input);
      // kill child process, since we won't get to execvp and this
      // is the only way the child process will finish
      kill((int) getpid(), SIGTERM);
    }
    check = dup2(fd_in,STDIN_FILENO);
    error_report(check, N->input);
  }

  if(N->output != NULL){
    fd_out = open(N->output, O_WRONLY | O_APPEND | O_CREAT);
    if(fd_out < 0){
      perror(N->output);
      kill((int) getpid(), SIGTERM);
    }
    check = dup2(fd_out,STDOUT_FILENO);
    error_report(check, N->input);
  }

  // if there is another command (i.e piping)
  if(N->next != NULL){
    int a[2] = {-1,-1};
    check = pipe(a);
    error_report(check,"pipe");

    pid = fork();

    if(pid == 0){ 
      // recursive call with args: next node , pipe array
      execute(N->next,a);
    } else{
      // pipe the output
      dup2(a[1],STDOUT_FILENO);
      close(a[0]);
    }
    error_report(pid,"fork");
  }


  if(execvp(N->cmd,whole_command) < 0){
        perror(N->cmd);
        // kill the process if there is an error.
        kill((int) getpid(), SIGTERM);
  }

}


int main() {

    char *cmd = NULL;
    char *in_file = NULL;
    char *out_file = NULL;
    char **flags = (char**)calloc(20,sizeof(char*));;
    char **args;
    pid_t pid, wpid;
  
    int status;


    while(1) {
      printf("~$");
      args = getline();
      
      LinkedList L = newLinkedList();

      //if we typed exit, then we exit
      if(args[0] != NULL){
        if(strcmp(args[0],"exit") == 0){
          freeLinkedList(&L);

          exit(0);
        }
      }

      if(args[0] != NULL){
        const char *directory = getenv("HOME");
        if(strcmp(args[0],"cd") == 0){
          freeLinkedList(&L);
          int x = chdir(directory);
          
          if(x == 1){
            perror(directory);
          } else{
            continue;
          }
        }
      }

      // initializes the linked list
      init(cmd,in_file,out_file,flags, args, L);

      // initialize piping with bogus values
      int p[2] = {-1,-1};

      pid = fork();

      if(pid == 0){
        // child
        execute(L->head, p);
      } else{
        //parent
        wait(&status);
      }
      error_report(pid,"fork");
      

  } // end of while loop

  return 0;

}