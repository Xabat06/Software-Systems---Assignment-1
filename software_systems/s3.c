#include "s3.h"

///Updated in Task 3 for cd
void construct_shell_prompt(char shell_prompt[], char lwd[])
{
    char cwd[MAX_PROMPT_LEN];

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd");
        strcpy(cwd, "?");
    }

    snprintf(shell_prompt, MAX_PROMPT_LEN, "[%s s3]$ ", cwd);
}

///Prints a shell prompt and reads input from the user
///Updated in task 3 for cd
void read_command_line(char line[], char lwd[])
{

    char shell_prompt[MAX_PROMPT_LEN];
    construct_shell_prompt(shell_prompt, lwd);
    printf("%s", shell_prompt);

    ///See man page of fgets(...)
    if (fgets(line, MAX_LINE, stdin) == NULL)
    {
        perror("fgets failed");
        exit(1);
    }
    ///Remove newline (enter)
    line[strcspn(line, "\n")] = '\0';
}

void parse_command(char line[], char *args[], int *argsc)
{
    ///Implements simple tokenization (space delimited)
    ///Note: strtok puts '\0' (null) characters within the existing storage, 
    ///to split it into logical cstrings.
    ///There is no dynamic allocation.

    ///See the man page of strtok(...)
    char *token = strtok(line, " ");
    *argsc = 0;
    while (token != NULL && *argsc < MAX_ARGS - 1)
    {
        args[(*argsc)++] = token;
        token = strtok(NULL, " ");
    }
    
    args[*argsc] = NULL; ///args must be null terminated
}

///Launch related functions
void child(char *args[], int argsc)
{
    ///Implement this function:

    ///Use execvp to load the binary 
    ///of the command specified in args[ARG_PROGNAME].
    ///For reference, see the code in lecture 3.

    execvp(args[ARG_PROGNAME],  args);

}

void launch_program(char *args[], int argsc)
{
    ///Implement this function:

    ///fork() a child process.
    ///In the child part of the code,
    ///call child(args, argv)
    ///For reference, see the code in lecture 2.

    ///Handle the 'exit' command;
    ///so that the shell, not the child process,
    ///exits.
    if (argsc > 0 && strcmp(args[0], "exit") == 0){
        exit(0);
    }
    ///Handles exit in the shell(parent) before forking
    int rc = fork();
    if(rc < 0){
        fprintf(stderr,"fork failed\n");
        exit(1);
    }
    else if(rc == 0){//in child process
        child(args, argsc); /////argv ??
    }
    else{//in parent process
        wait(NULL);
    }


}

void launch_program_with_redirection(char *args[], int argsc){

    int rc = fork();
    if(rc < 0){
        fprintf(stderr,"fork failed\n");
        exit(1);
    }
    else if(rc == 0){//in child process

        char * operator = identify_operator(args, argsc);
        char * file_rd = identify_file_to_read(args, argsc);
        char * file_wr = identify_file_to_write(args, argsc);
        //here we need to check if either side of the operator are there files, if both sides then we need to redirect input and output
        //else we redirect only one

        if(file_rd != NULL && file_wr == NULL){
            child_with_input_redirected(file_rd,operator,args,argsc);
        }
        else if(file_rd == NULL && file_wr != NULL){
            child_with_output_redirected(file_wr, operator, args, argsc);
        }
        else if(file_rd != NULL && file_wr != NULL){
            child_with_input_output_redirected(file_rd, file_wr ,operator,args,argsc);
        }
        else{
            fprintf(stderr, "error\n");
        }
    }
    else{//in parent process
        wait(NULL);
    }

}

void child_with_input_redirected(char* file_read ,char * operator ,char * args [], int argsc){


    int file_fd = open(file_read, O_RDONLY);
    dup2(file_fd, STDOUT_FILENO);

    execvp(args[ARG_PROGNAME], args);


}

void child_with_output_redirected(char* file_write, char * operator, char* args [], int argsc){

    int file_fd = open(file_write, O_WRONLY | O_CREAT);
    dup2(file_fd, STDIN_FILENO);
    
    execvp(args[ARG_PROGNAME],  args);

}

void child_with_input_output_redirected(char* file_read ,char* file_write, char * operator, char* args [], int argsc){

    int file_fd = open(file_write, O_WRONLY | O_CREAT);
    dup2(file_fd, STDIN_FILENO);

    file_fd = open(file_read, O_RDONLY);
    dup2(file_fd, STDOUT_FILENO);

    execvp(args[ARG_PROGNAME],  args);


}

char* identify_operator (char* args [], int argsc){
    
    char* operator;
    int i = 0;

    while(i<argsc){

        if(*args[i] == '>'){
            operator = ">";
            
            if((*(args[i]+1)) == '>'){
                operator = ">>";
            }
            return operator;
        }
        else if(*args[i] == '<'){
            operator = "<";

            if((*(args[i]+1)) == '<'){
                operator = "<<";
            }
            return operator;
        }
        else{
            i++;
        }
    }
    return NULL;
}

char* identify_file_to_write (char* args [],int argsc){

    //go to operator > return pointer to arg next to operator
    char* file;
    int i = 0;

    while(i<argsc){

        if(*args[i] == '>'){

            return args[i+1];
        }
        else{
            i++;
        }
    }
    return NULL; // if null no file to write to
   
    
}

char* identify_file_to_read (char* args [],int argsc){

    //go to operator if > and return pointer to arg before operator
    //go to operator if < return pointer to arg after operator

    char* operator;
    int i = 0;

    while(i<argsc){

        if(*args[i] == '>'){
        
            return args[i-1]; // need to consider if user gives wrong input like just input > will this cause seg fault
        }
        else if(*args[i] == '<'){

            return args[i+1];
        }
        else{
            i++;
        }
    }
    return NULL; // this means we aren't reading from a file
    
}

int command_with_redirection(char line[]){

    for(int i = 0; i<strlen(line); i++){

        if(line[i] == ('<' | '>')){
            return 1;
        }

    }
    return 0;

}

///Task 3 functions cd
void init_lwd(char lwd[]) {
    if (getcwd(lwd, MAX_PROMPT_LEN - 6) == NULL) {
        perror("getcwd failed");
        exit(1);
    }
}

int is_cd(char line[]) {
    while (*line == ' ') line++;

    return (strncmp(line, "cd", 2) == 0 && (line[2] == ' ' || line[2] == '\0'));
}

void run_cd(char *args[], int argsc, char lwd[])
{
    char cwd[MAX_PROMPT_LEN - 6];

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd failed");
        return;
    }

    int directory_exists = 0;

    if (argsc == 1) {
        char *home = getenv("HOME");
        if (chdir(home) == 0) 
            directory_exists = 1;
        else {
            perror("cd");
            return;
        }
    }

    else if (strcmp(args[1], "-") == 0) {
        if (chdir(lwd) == 0) 
            directory_exists = 1;
        else {
            perror("cd");
            return;
        }
    }

    else {
        if (chdir(args[1]) == 0) 
            directory_exists = 1;
        else {
            perror("cd");
            return;
        }
    }

    if (directory_exists)
        strcpy(lwd, cwd);
}
