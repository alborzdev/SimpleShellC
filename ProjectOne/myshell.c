#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdbool.h>
#define MAX_LINE 80 /* The maximum length command */


int main(void) {
    int piper[2];
    char filename[] = "filler.txt";
    char *history[MAX_LINE/2 + 1];
    int should_run = 1; /* flag to determine when to exit program */
    bool hist = false;
    bool outputTxt = false;
    bool inputTxt = false;
    bool backgroundRun = false;
    bool piping = false;
    int l = 0;
    history[0] = "";
    while (should_run) {
        printf("mysh:~$ ");
        fflush(stdout);
        char *args[MAX_LINE/2 + 1]; //char pointer array of command line input
        char *redirectArgs[MAX_LINE/2 + 1];
        char *pipeCommand[MAX_LINE/2 + 1];
        char *bgCommand[MAX_LINE/2 + 1];
        int i = 0; //tokenizer loop counter
        int k = 0;
        char input[MAX_LINE/2 + 1]; //user input array
        char copy[MAX_LINE/2 + 1]; //user input array
        fgets(input, MAX_LINE/2 + 1, stdin);

        input[strcspn(input, "\n")] = 0;

        if(strcmp(input, "!!") != 0) {
            strcpy(copy, input);
            char *piece = strtok(copy, " "); //first token from input
            while (piece) { //loop as long as piece is not NULL
                history[i] = piece; //insert the current token in history array
                args[i] = piece; //insert the current token in args array
                piece = strtok(NULL, " "); //set piece to next token from command line
                i++;
            }
            args[i] = NULL;
            //strcpy(history, args);
            history[i] = NULL;

        } else {
            if(strcmp(history[0], "") == 0){
                printf("No commands in history\n");
                continue;
            } else {
                hist = true;
            }
        }

        if(strcmp(args[0], "exit") == 0 ){
            should_run = 0;
        }

        //printf("%s", history[0]);
        /** After reading user input, the steps are:
         * (1) fork a child process using fork()
         * (2) the child process will invoke execvp()
         * (3) parent will invoke wait() unless command included &
         */
        //This for loop increments j up until is finds a '>' character
        //Therefore args[j] = '>'
        //Now redirect output by setting outputTxt to true
        //Also set the file name by going one token after the >
        for (int j = 0; j < i; ++j) {
            if (strcmp(args[j], ">") == 0) {
                outputTxt = true;
                strcpy(filename, args[j+1]);
                //This for loop copies the args array up untill
                //the '>' character
                for (k = 0; k < j; ++k) {
                    redirectArgs[k] = args[k];
                }
                redirectArgs[k] = NULL;
                break;
            } else if (strcmp(args[j], "<") == 0) {
                inputTxt = true;
                strcpy(filename, args[j+1]);
                //This for loop copies the args array up untill
                //the '<' character
                for (k = 0; k < j; ++k) {
                    redirectArgs[k] = args[k];
                }
                redirectArgs[k] = NULL;
                break;
            } else if (strcmp(args[j], "|") == 0) {
                piping = true;
                int temp = j;
                for (int m = 0; m < (i - (j+1)); ++m) {
                    pipeCommand[m] = args[temp+1];
                    temp++;
                }
                //This for loop copies the args array up untill
                //the '<' character
                for (k = 0; k < j; ++k) {
                    redirectArgs[k] = args[k];
                }

                break;
            }
        }

        if(strcmp(args[i-1], "&") == 0) {
            backgroundRun = true;
            for (k = 0; k < i - 1; ++k) {
                bgCommand[k] = args[k];
            }
            bgCommand[k] = NULL;
        }
        //printf("%d %d %d %d %d", outputTxt, inputTxt, piping, backgroundRun, hist);
        if(!piping && strcmp(args[0], "cd") != 0) {
            pid_t pid = fork();
            if (pid < 0) {
                perror("Fork Error");
            } else if (pid == 0) {
                //if the user wants to repeat the last command
                //call execvp on the history array, which contains
                //the last called command
                if (hist) {
                    execvp(history[0], history);
                    //If we want to redirect output into a txt file
                } else if (outputTxt) {
                    int out = open(filename, O_RDWR | O_CREAT | O_APPEND, 0600);


                    int err = open("cerr.log", O_RDWR | O_CREAT | O_APPEND, 0600);


                    int save_out = dup(fileno(stdout));
                    int save_err = dup(fileno(stderr));

                    if (-1 == dup2(out, fileno(stdout))) {
                        perror("cannot redirect stdout");
                        return 255;
                    }
                    if (-1 == dup2(err, fileno(stderr))) {
                        perror("cannot redirect stderr");
                        return 255;
                    }

                    puts(execvp(redirectArgs[0], redirectArgs));
                    fflush(stdout);
                    close(out);
                    fflush(stderr);
                    close(err);

                    dup2(save_out, fileno(stdout));
                    dup2(save_err, fileno(stderr));

                    close(save_out);
                    close(save_err);

                    puts("back to normal output");
                } else if (inputTxt) {
                    int in = open(filename, O_RDWR | O_CREAT | O_APPEND, 0600); //new file descriptor 'out'

                    int err = open("cerr.log", O_RDWR | O_CREAT | O_APPEND, 0600);
                    //duplicating stdout file descriptor into save_out
                    int save_in = dup(fileno(stdin));
                    int save_err = dup(fileno(stderr));

                    if (-1 == dup2(in, fileno(stdin))) {
                        perror("cannot redirect stdout");
                        return 255;
                    }
                    if (-1 == dup2(err, fileno(stderr))) {
                        perror("cannot redirect stderr");
                        return 255;
                    }
                    //writes a string to stdout, but since stdout file descriptor
                    //is now duplicated to save_out, it writes it to save_out too
                    puts(execvp(redirectArgs[0], redirectArgs));

                    //clear stdout so the puts does not actually output it to
                    //stdout, and rather only save_out
                    fflush(stdin);
                    close(in);
                    fflush(stderr);
                    close(err);


                    dup2(save_in, fileno(stdin));
                    dup2(save_err, fileno(stderr));

                    close(save_in);
                    close(save_err);

                    puts("back to normal output");

                } else if (backgroundRun) {
                    int old_stdout = dup(1);
                    freopen("/dev/null", "w", stdout);
                    execvp(bgCommand[0], bgCommand);
                    fclose(stdout);
                    stdout = fdopen(old_stdout, "w");

                } else {
                    execvp(args[0], args);
                }
            }
            if(!backgroundRun) {
                wait(NULL);
                //wait(NULL);
                //wait(NULL);
                piping = false;
                hist = false;
                outputTxt = false;
                inputTxt = false;
            } else {
                backgroundRun = false;
            }

        } else if (strcmp(args[0], "cd") == 0) {
            chdir(args[1]);
        } else {

            pipe(piper);
            pid_t pif = fork();

            if(pif < 0) {

            }
            else if(pif == 0) {

                close(piper[0]);
                dup2(piper[1], stdout);
                execvp(redirectArgs[0], redirectArgs);
            }
            pif = fork();
            if(pif < 0){

            }
            else if ( pif == 0 ) {

                close(piper[1]);
                dup2(piper[0], stdin);
                execvp(pipeCommand[0], pipeCommand);
            }
            close(piper[0]);
            close(piper[1]);
            wait(NULL);
            wait(NULL);


        }


    }

    return 0;
}
