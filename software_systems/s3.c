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

    /*int i = 0; // testing what args is:
    while(args[i] != NULL){
        printf("%c", *args[i++]);
    }*/
}

///Launch related functions
void child(char *args[], int argsc)
{
    ///Implement this function:

    ///Use execvp to load the binary 
    ///of the command specified in args[ARG_PROGNAME].
    ///For reference, see the code in lecture 3.

    execvp(args[ARG_PROGNAME],  args);

    perror("execvp");
    exit(1);

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

//REDIRECTION functions
void launch_program_with_redirection(char *args[], int argsc){
    
    int rc = fork();
    if(rc < 0){
        fprintf(stderr,"fork failed\n");
        exit(1);
    }
    else if(rc == 0){//in child process
        char * operator = identify_operator(args, argsc);
        char * file_to_redirect_to = identify_file_to_redirect_to(args, argsc,operator);

        int i = should_append(args,operator);

        clean_args(args);

        if(*operator == '<'){

            child_with_input_redirected(file_to_redirect_to, operator,args,argsc);
        }
        else if(*operator == '>'){
            
            child_with_output_redirected(file_to_redirect_to, operator, args, argsc, i);
        }
        else{
            fprintf(stderr, "error\n");
        }
    }
    else{//in parent process
        wait(NULL);

    }

}

void child_with_input_redirected(char* rd_file ,char * operator ,char * args [], int argsc){

    int file_fd = open(rd_file, O_RDONLY);

    if (file_fd == -1) {
        perror("open");
        exit(1);
    }

    dup2(file_fd, STDIN_FILENO);
    close(file_fd);
    //should we close ? yes

    execvp(args[ARG_PROGNAME], args);

    perror("execvp");
    exit(1);

}

void child_with_output_redirected(char* rd_file, char * operator, char* args [], int argsc, int append){

    int file_fd = 0;

    if(append == 0){
        file_fd = open(rd_file, O_WRONLY | O_CREAT | O_TRUNC, 0644); // || or |, assume its | as flag values?
    }
    else{
        file_fd = open(rd_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
    }

    if (file_fd == -1) {
        perror("open");
        exit(1);
    }

    dup2(file_fd, STDOUT_FILENO);
    close(file_fd);
    //should we close ?
    execvp(args[ARG_PROGNAME],  args);

    perror("execvp");
    exit(1);

}

char* identify_operator (char* args [], int argsc){
    
    char* operator;
    int i = 0;

    while(i<argsc){

        if(*args[i] == '>'){
            operator = ">";

            return operator;
        }
        else if(*args[i] == '<'){
            operator = "<";

            return operator;
        }
        else{
            i++;
        }
    }
    return NULL;
}

char* identify_file_to_redirect_to (char* args [],int argsc, char* operator){

    //go to operator > return pointer to arg next to operator
    char* file;
    int i = 0;

    while(i<argsc){

        if(*args[i] == *operator){

            return args[i+1];
        }
        else{
            i++;
        }
    }

    return NULL; // if null no file to write to
    
}

int command_with_redirection(char line[]){

    for(int i = 0; i<strlen(line); i++){

        if(line[i] == '<'){
            return 1;
        }
        else if(line[i] == '>'){
            return 1;
        }

    }
    return 0;

}

int should_append(char* args[], char* operator){

    int operator_found = 0;
    int i = 0;

    while(operator_found == 0 && args[i] != NULL) {  //should we worry about accessing out of bounds here? good practice

        if(*args[i] == *operator){
            operator_found = 1;
        }
        else{
            i++;
        }

    }

    if (!operator_found) return 0;

    if(args[i][1] == *operator){

        return 1;
    }
    else{
        return 0;
    }

}


void clean_args(char *args[]){

    int set_null = 0;
    int i = 0;

    while(args[i] != NULL){

        if(*args[i] == '<'){
            set_null = 1;
        }
        else if(*args[i] == '>'){
            set_null = 1;
        }
        if(set_null == 1){
            args[i] = NULL;
            i++;
        }
        else{
            i++;
        }
    }

}

//CD functions
void init_lwd(char lwd[]){
    if (getcwd(lwd, MAX_PROMPT_LEN - 6) == NULL) {
        perror("getcwd failed");
        exit(1);
    }
}

int is_cd(char line[]){
    while (*line == ' ') line++;

    return (strncmp(line, "cd", 2) == 0 && (line[2] == ' ' || line[2] == '\0'));
}

void run_cd(char *args[], int argsc, char lwd[]){
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
