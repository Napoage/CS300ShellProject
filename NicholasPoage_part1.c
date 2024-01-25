//Name: Nicholas Poage
//Cwid: 12250963
//Compile Instructions: gcc -Wall NicholasPoage_part1.c -std=c99
//Comments:Designed for Linux
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <fcntl.h>
#define MAX_LINE 80 /* The maximum length command */
int main(void) {

    char *args[MAX_LINE/2 + 1]; /* command line arguments */
    int should_run = 1; /* flag to determine when to exit program */
    while (should_run) {
        printf("poage_pid> ");
        fflush(stdout);

        //Reads in the command
        char input[MAX_LINE];
        fgets(input,MAX_LINE,stdin);

        if(input[strlen(input)-1] == '\n'){
            input[strlen(input)-1] = '\0';
        }

        //exits loop
        if (strcmp(input, "exit") == 0) {
            should_run = 0;
            break; 
        }
        bool hasAmp = false;
        int i = 0;

        char *arguments = strtok(input, " ");
        while (arguments != NULL) {
            if (strcmp(arguments, "&") == 0){
                hasAmp = true;
            }
            else {
                args[i++] = arguments;
            }
            //NULL contuines from where left off
            arguments = strtok(NULL," ");
        }
        args[i] = NULL;
        //used https://www.geeksforgeeks.org/fork-system-call/ to research how to handle forks
        //used https://www.geeksforgeeks.org/wait-system-call-c/ to research how to handle wait
        //forks to create a child process
        int pid = fork();
        //checks for failure
        if (pid < 0) {
            perror("fork failed");
            exit(1);
        } else if (pid == 0) { // executes statement
            if (execvp(args[0], args) < 0) {
                printf("execvp failed\n");
                return -1;
            }
        }
        else {
            if (!hasAmp) {
                wait(NULL);
            }
        }

        //fflush(stdin);
    }

    return 0;
}
