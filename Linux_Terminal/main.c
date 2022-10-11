#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h> 
#include <fcntl.h>
#include <errno.h>

#define MAX_BUFF (1024)

typedef struct list list;
typedef struct element element;

// data type for list element
struct element {
    int value;              // to store the PID
    char command[MAX_BUFF]; // to store user input
    element *next;          // pointer to next element
};

// data type for list administration 
struct list {
    element *head;  // first element of list
    element *tail;  // last element of list
};

// initializing a new list
list* list_init(){
    list *ll = malloc(sizeof(list));
    ll -> head = NULL;
    ll -> tail = NULL;
    return ll;
}

// initializing a new element
element* element_init(int value, char* command){
    element *ele = malloc(sizeof(element));
    ele -> value = value;
    snprintf(ele -> command, MAX_BUFF, command);
    ele -> next = NULL;
    return ele;
}

// adding a new element to the end of the list 
void enque (list *list, element *item) {
    // list is empty
    if(list -> head == NULL){
        list -> head = item;
        list -> tail = item;
    }   
    // list is not empty
    else{
        list -> tail -> next = item;
        list -> tail = item;
        }       
}

// removing an element from the list 
int deque (list *list, int value) {
    // list is empty
    if(list -> head == NULL){
        printf("Empty list! \n");
        return -1;
    } 
    // element is head
    element* tmp = list -> head;
    if(tmp -> value == value){
        list -> head = tmp -> next; 
        free(tmp);
    }
    // element is not head
    else{
        while(tmp -> next -> value != value){
            // element not in list
            if (tmp -> next == list -> tail){
                printf("Element %d not in list! \n", value);
                return -1;
            }
            tmp = tmp -> next;  
        }
        // element is tail
        if(tmp -> next == list -> tail){
            element* del = list -> tail;
            list -> tail = tmp;
            tmp -> next  = NULL;
            free(del);
        }
        // element is in between head and tail
        else{
            element* del = tmp -> next;
            tmp -> next  = del -> next;
            free(del);
        }
    }       
    return 1;
}

// debugging function: check for all the list functions
void list_print(list *list){
    // list is empty
    if(list -> head == NULL){
        printf("Empty list! \n");
        return;
    }
    // list is not empty
    element* current;
    current = list -> head;
    printf("List: "); 
    while(current != NULL){
        printf("| %d |",current -> value);
        current = current -> next;
    }
    printf("\n\n");
}

// catching terminated backup processes
void catch_zombies(list *list){
    int status_catch = 0;
    element* del;
    element* tmp = list -> head;
    // cycling through list and executing waitpid with "WNOHANG"
    while(tmp != NULL){
        int kid = waitpid(tmp -> value, &status_catch, WNOHANG);
        del = tmp;
        tmp = tmp -> next;
        // if child process terminated
        if(kid){
            printf("Exit status [%s] = %d \n", del -> command, status_catch);
            if (!deque(list, del -> value)){
                printf("Removing process from list failed! \n");
            }
        }
    }
} 

// killing and catching all backup processes before leaving terminal
void clean_up(list* list){
	int status_clean;
    element* tmp = list -> head;
    // cycling through list, killing the processes and executing waitpid with "0"
	while(tmp != NULL){
		kill(tmp -> value, SIGKILL);
		waitpid(tmp -> value, &status_clean, 0);
        tmp = tmp -> next;
	}
}

// listing all the current background processes 
void jobs(list* list){
    element* tmp = list -> head;
    while(tmp != NULL){
        printf("PID: %d | Input: %s \n", tmp -> value, tmp -> command);
        tmp = tmp -> next;
    }
}

// debugging function: check for catching_zombies
void monitor_processes(){
    pid_t child = fork();
    if(child == 0){
        char *argv[2] = {"ps", NULL};
        execv("/bin/ps", argv); 
        exit(0);
    }
    else if(child > 0){
        int status_monitor;
        sleep(2);
        waitpid(child, &status_monitor, 0);
    }   
}

//changing directory function
int cd(char **path, size_t path_size, char *cwd){
    if(sizeof(path) > 1){
        if(chdir(path[1]) == 0){
            getcwd(cwd, sizeof(cwd));
            return 0;
        }
        else{
            return 1;
        }
    }
    else{
        return 1;
    }
}

/*
 * function splits char* into char** by two possible delimiters
 * also updates length variable with nr of split elements
*/
char** str_split(char* a_str, size_t* length)
{
    char** result = 0;
    size_t count = 0;
    char* tmp = a_str;
    char* last_comma = 0;
    // delimiters hardcoded below, easy to change to function argument if necessary
    char delimiters[] = { ' ', '\t' };

    while (*tmp)
    {
        if (delimiters[0] == *tmp || delimiters[1] == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }
    count += last_comma < (a_str + strlen(a_str) - 1);
    count++;
    result = malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx = 0;
        char* token = strtok(a_str, delimiters);
        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delimiters);
        }
        *length = idx;
        *(result + idx) = 0;
    }
    return result;
}

// main function
int main() {

	char command[MAX_BUFF];
    char currentCommand[MAX_BUFF];
    char cwd[MAX_BUFF];
    char input[MAX_BUFF], output[MAX_BUFF];
    char** tokens;
    size_t size;
    int flag_direction = 0;
    int flag_wait;
    list* childs = list_init();

    for (;;) {

        // getting user input
        memset(currentCommand, 0, MAX_BUFF);
        int flag_in=0;
        int flag_out = 0;
        getcwd(cwd, sizeof(cwd)); 
        printf("%s: ", cwd);
        fgets(currentCommand, MAX_BUFF, stdin);
        flag_wait = 1;
        // exiting loop with control-D input
        if(currentCommand[0] == NULL){
            //monitor_processes();
            clean_up(childs);
            //monitor_processes();
            printf("\n");
            break;
        }

        if ((strlen(currentCommand) > 0) && (currentCommand[strlen(currentCommand) - 1] == '\n'))
            currentCommand[strlen(currentCommand) - 1] = '\0';

        sprintf(command, currentCommand);
        tokens = str_split(currentCommand, &size);

        // check for foreground or background process
        if(size > 1){
            // background process 
            if(strcmp(tokens[(size - 1)], "&") == 0){
                //printf("flag switch to no wait \n");
                flag_wait = 0;
                tokens[size - 1] = NULL;
            }
            // foreground process 
            else{
                flag_wait = 1;
            }
        }
        
        //Checking for I/O redirections
        if (size>2){
            for(int i = size-2; i >= 0; i--){
                if(strcmp(tokens[i], ">") == 0){
                    flag_out = 1;
                    sprintf(output, "%s", tokens[i + 1]);
                    tokens[i + 1] = NULL;
                    tokens[i] = NULL;
                }
                else if(strcmp(tokens[i], "<") == 0){
                    flag_in = 1;
                    sprintf(input, "%s", tokens[i + 1]);
                    tokens[i + 1] = NULL;
                    tokens[i] = NULL;
                }
            }
        }
        
        // catching zombies before command execution
        catch_zombies(childs);  
    
        if(tokens[0] != NULL){
            // execution internal command "cd" in parent process
            if(strcmp(tokens[0], "cd") == 0){
                int err = cd(tokens, size, cwd);
                if(err){
                    printf("Exit status [%s] = %d \n", command, errno);
                }
            }
            // execution internal command "jobs" in parent process
            else if(strcmp(tokens[0], "jobs") == 0){
                jobs(childs);
            }
            // execution external command in child process
            else{
                int pid;
                pid = fork();
                // child process
                if(pid == 0){
                    
                    fflush(0); 
                    if(flag_out == 1){
                        int fd1 = creat(output, 0644) ;
                        dup2(fd1, STDOUT_FILENO);
                        close(fd1);
                    }
                    if(flag_in == 1){
                        int fd0 = open(input, O_RDONLY);
                        dup2(fd0, STDIN_FILENO);
                        close(fd0);
                    }
                    // executing command with exec
                    execvp(tokens[0], tokens);
                    exit(errno);
                } 
                // parent process
                else if (pid > 0){     
                    // add background process to list       
                    if(!flag_wait){
                        element* child = element_init(pid, command);
                        enque(childs, child);
                    }
                    // wait for foreground process
                    else{
                        int status_exit;
                        int kid = waitpid(pid, &status_exit, 0);
                        if(kid){
                            printf("Exit status [%s] = %d \n", command, status_exit);
                        }
                        else{
                            printf("waitpid failed! \n");
                        }
                    }
                } 
                else{
                    perror("Fork failed! \n");
                }
            }
        }
        free(tokens);
    }
	return 0;
}