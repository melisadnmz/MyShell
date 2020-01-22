#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include<fcntl.h>


#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */


//-------------File flags for > type operations-------------
#define CREATE_FLAGS (O_WRONLY | O_TRUNC | O_CREAT )
//-------------File flags for >> type files--------------
#define CREATE_APPENDFLAGS (O_WRONLY | O_APPEND | O_CREAT )
//-------------File flags for input files-------------
#define CREATE_INPUTFLAGS (O_RDWR)
//-------------File flags for type mode--------------
#define CREATE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

void executeCommand(char *args[],int *background);
void redirectCommand(char *args[], int *background);

char history[100][MAX_LINE] = {{0}};
int size = 0;
int commandNum = 0;

int foreGround = 0;
int currentForeground;

typedef struct path{
    char name[15];
    struct path *next;
} pathList;

pathList *head = NULL;

void welcomeScreen()
{

    printf("\n\t============================================\n");
    printf("\t               Simple C Shell\n");
    printf("\t============================================\n\n\n");

}


/* The setup function below will not return any value, but it will just: read
in the next command line; separate it into distinct arguments (using blanks as
delimiters), and set the args array entries to point to the beginning of what
will become null-terminated, C-style strings. */
void setup(char inputBuffer[], char *args[],int *background)
{
    int length, /* # of characters in the command line */
            i,      /* loop index for accessing inputBuffer array */
            start,  /* index where beginning of next command parameter is */
            ct;     /* index of where to place the next parameter into args[] */

    ct = 0;

    /* read what the user enters on the command line */
    length = read(STDIN_FILENO,inputBuffer,MAX_LINE);

    /* 0 is the system predefined file descriptor for stdin (standard input),
       which is the user's screen in this case. inputBuffer by itself is the
       same as &inputBuffer[0], i.e. the starting address of where to store
       the command that is read, and length holds the number of characters
       read in. inputBuffer is not a null terminated C-string. */

    start = -1;
    if (length == 0)
        exit(0);            /* ^d was entered, end of user command stream */

/* the signal interrupted the read system call */
/* if the process is in the read() system call, read returns -1
  However, if this occurs, errno is set to EINTR. We can check this  value
  and disregard the -1 value */
    if ( (length < 0) && (errno != EINTR) ) {
        perror("error reading the command");
        exit(-1);           /* terminate with error code of -1 */
    }

    for (i=0;i<length;i++){ /* examine every character in the inputBuffer */

        switch (inputBuffer[i]){
            case ' ':
            case '\t' :               /* argument separators */
                if(start != -1){
                    args[ct] = &inputBuffer[start];    /* set up pointer */
                    ct++;
                }
                inputBuffer[i] = '\0'; /* add a null char; make a C string */
                start = -1;
                break;

            case '\n':                 /* should be the final char examined */
                if (start != -1){
                    args[ct] = &inputBuffer[start];
                    ct++;
                }
                inputBuffer[i] = '\0';
                args[ct] = NULL; /* no more arguments to this command */
                break;
            case '&':
                *background  = 1;
                inputBuffer[i-1] = '\0';
                break;

            default :             /* some other character */
                if (start == -1)
                    start = i;
        } /* end of switch */
    }    /* end of for */
    args[ct] = NULL; /* just in case the input line was > 80 */
} /* end of setup routine */

void insertHistory(int argc,char *args[]){
    if(strcmp(args[0],"history") != 0){
        strcpy(history[size],args[0]);
        commandNum++;
        int k = 1;
        while(args[k] != NULL){
            strcat(history[size], " ");
            strcat(history[size],args[k]);
            k++;
        }
        size++;
    }
}

void printHistory(){
    int i = 0;
    int k = commandNum;
    while(i < commandNum){
        printf("Command %d : %s\n",i,history[k-1]);
        i++;
        k--;
        if(i == 10)
            break;
    }
}

void redirectCommand(char *args[], int *background){
    char out[50];
    char err[50];
    char input[50];
    int i = 0;
    while (args[i] != NULL) {
        if (strcmp(">", args[i]) == 0 ) {
            int fd;
            strcpy(out,args[i+1]);
            fd = open(out,CREATE_FLAGS,CREATE_MODE);
            if(fd == -1){
                fprintf(stderr, "Failed to open file\n");
                return;
            }
            if(dup2(fd,STDOUT_FILENO) == -1){
                fprintf(stderr, "Failed to redirect standart output\n");
                return;
            }

            if(close(fd) == -1){
                fprintf(stderr, "Failed to close the file\n");
                return;
            }
            args[i] = NULL;
            args[i + 1] = NULL;
        }
        /*if (strcmp(">>", args[i]) == 0 ) {
            int fd;
            strcpy(out,args[i+1]);
            fd = open(out,CREATE_APPENDFLAGS,CREATE_MODE);
            if(fd == -1){
                fprintf(stderr, "Failed to open file\n");
                return;
            }
            if(dup2(fd,STDOUT_FILENO) == -1){
                fprintf(stderr, "Failed to redirect standart output\n");
                return;
            }

            if(close(fd) == -1){
                fprintf(stderr, "Failed to close the file\n");
                return;
            }
            args[i] = NULL;
            args[i + 1] = NULL;
        }*/
        /*if (strcmp("<", args[i]) == 0 ) {
            int fd;
            strcpy(input,args[i+1]);
            fd = open(input,CREATE_INPUTFLAGS,CREATE_MODE);
            if(fd == -1){
                fprintf(stderr, "Failed to open file\n");
                return;
            }
            if(dup2(fd,STDIN_FILENO) == -1){
                fprintf(stderr, "Failed to redirect standart output\n");
                return;
            }
            if(close(fd) == -1){
                fprintf(stderr, "Failed to close the file\n");
                return;
            }
            args[i] = NULL;
            args[i + 1] = NULL;
        }
        if (strcmp("2>", args[i]) == 0 ) {
            int fd;
            strcpy(err,args[i+1]);
            fd = open(err,CREATE_FLAGS,CREATE_MODE);
            if(fd == -1){
                fprintf(stderr, "Failed to open file\n");
                return ;
            }
            if(dup2(fd,STDERR_FILENO) == -1){
                fprintf(stderr, "Failed to redirect standart output\n");
                return;
            }
            if(close(fd) == -1){
                fprintf(stderr, "Failed to close the file\n");
                return;
            }
            args[i] = NULL;
            args[i + 1] = NULL;
        }*/

        i++;
    }
    executeCommand(args,&background);

}

void execHistory(char *args[],int *background){
    int k = atoi(args[2]);
    char *str = strdup(history[commandNum - k -1]);
    char *args2[100];
    args2[0] = strdup(str);
    executeCommand(args2,&background);

    //   args2[0] = strdup(token);
    //   char *temp ="";
    //strcpy(args2,token);
    //  executeCommand(args2,background);
    /* while( token != NULL )
       {
       printf("ggggg");
           strcpy(temp,token);
           printf("args2 %s \n",args2);
         // executeCommand(&args2,background);
           token = strtok(NULL, " ");
       }*/

}

void executeCommand(char *args[], int *background){
    pid_t pid = fork();

    if(pid == 0) // child
    {
        int i;
        char path[80] = "";
        char * pathEnv = getenv ("PATH");
        char *token = strtok(pathEnv, ":");

        while( token != NULL )
        {
            strcpy(path,token);
            strcat(path,"/");
            strcat(path,args[0]);
            execv(path, args);

            token = strtok(NULL, ":");
        }

    }
    else if (pid > 0){ // parent
        int status;
        if (*background == 0) {
            foreGround = 1;
            currentForeground = pid;
            wait(pid);
            foreGround = 0;

        }
        else{
            printf("Background p_id:%d\n",pid);
        }

        *background = 0;


    }
    else
    {
        fprintf(stderr, "Fork failed");
        exit(-1);
    }

}

void sigtstp_handler(int signum) {
    // If there is a foreground process
    if(foreGround != 1){
        printf("There is no foreground process");
    }
    if(foreGround == 1) {
        kill(currentForeground, SIGTSTP);
        if(errno == ESRCH) {
            fprintf(stderr, "\nprocess %d not found\n", currentForeground);
            foreGround = 0;
            printf("myshell: ");
            fflush(stdout);
        }

    }
    else {
        printf("\nmyshel: ");
        fflush(stdout);
    }
}

void path(){
    pathList *temp = head;
    while(temp!=NULL){
        printf("%s:",temp->name);
        temp=temp->next;
    }
    printf("\n");
}

void addPath(char name[]){
    pathList *temp = head;
    pathList *node = malloc(sizeof(pathList));
    strcpy(node->name,name);

    if(head==NULL){
        head=node;
    }else{
        while (1) {
            if(temp->next == NULL){
                temp->next = node;
                break;
            }
            temp = temp->next;
        }
    }
}

void deletePath(char name[]){
    pathList *temp = head;
    pathList *temp2 = temp;

    if(strcmp(head->name,name) == 0){
        head=head->next;
    }else{
        while(temp != NULL){
            if(strcmp(temp->name,name) == 0){
                temp2->next = temp->next;
                free(temp);
                break;
            }
            temp2 = temp;
            temp = temp->next;
        }
    }
}

int main(int argc, char *argv[])
{
    char inputBuffer[MAX_LINE]; /*buffer to hold command entered */
    int background; /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE/2 + 1]; /*command line arguments */
    welcomeScreen();
    /* Initialize signal */
    struct sigaction act;
    act.sa_handler = sigtstp_handler;
    act.sa_flags = SA_RESTART;

    // Set up signal handler for ^Z signal
    if ((sigemptyset(&act.sa_mask) == -1) || (sigaction(SIGTSTP, &act, NULL) == -1)) {
        fprintf(stderr, "Failed to set SIGTSTP handler\n");
        return 1;
    }

    /* Initialize the job list */
    // initjobs(jobs);


    while (1){


        background = 0;
        printf("myshell: ");
        fflush(NULL);
        /*setup() calls exit() when Control-D is entered */
        setup(inputBuffer, args, &background);
        insertHistory(argc,args);

        if(args[0] == 0){
            continue;
        }

        int i = 0;
        int redirect = 0;

        while (args[i] != NULL) {
            if (strcmp("<", args[i]) == 0 || strcmp(">>", args[i]) == 0 ||
                strcmp("2>", args[i]) == 0 || strcmp(">", args[i]) == 0) {
                redirect = 1;
                break;
            }
            i++;
        }

        if(redirect==1){
            redirectCommand(args,&background);
        }
        else if(strcmp(args[0], "history") == 0 && args[1] == NULL){
            printHistory();
        }
        else if(strcmp(args[0], "history") == 0 && strcmp(args[1], "-i") == 0){
            execHistory(args,background);
        }
        else if(strcmp(args[0], "path") == 0){
            if(args[1] == NULL){
                path();
            }else if(strcmp(args[1], "+") == 0 && args[2] != NULL){
                addPath(args[2]);
            }else if(strcmp(args[1], "-") == 0 && args[2] != NULL){
                deletePath(args[2]);
            }
        }
        else if(strcmp(args[0], "fg") == 0){

        }
        else if(strcmp(args[0], "exit") == 0){
            printf("Shell Process Terminated");
            exit(0);
        }
        else{
            executeCommand(args,&background);
        }

        /** the steps are:
        (1) fork a child process using fork()
        (2) the child process will invoke execv()
        (3) if background == 0, the parent will wait,
        otherwise it will invoke the setup() function again. */
    }
}
