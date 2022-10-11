#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define SIZE 20

//This is our data structure for an alarm containing pid date and number
struct alarm{
	pid_t pid;
	time_t scheduledTime;
	char dateTime[20];
	int alarmNr;
};
struct alarm list[SIZE];

//This alarm is used to overwrite deleted alarms
struct alarm dummy;
//This integer is keeping track of the generated alarms
int counter = 0;

/*
Arguments: char array
Return: true if array contains only digits, else returns false
*/
bool digit_check(char key[])
{
	for (int i = 0; i < strlen(key); i++)
	{
		if (isdigit(key[i]) == 0)
		{
			return false;
		}
	}
	return true;
}

/*
Arguments: char array
Return: time_t value representing char array
*/
time_t parse_time(char s[])
{
	struct tm ltm;
	time_t out = 0;
	strptime(s, "%Y-%m-%d %H:%M:%S", &ltm);
	out = timegm(&ltm);
	out -= 3600;
	return out;
}

/*
Arguments: time_t value
Return: integer, difference in seconds between argument and current time
*/
int when_to_ring(time_t a)
{
	time_t rawtime;
	int ret;
	time(&rawtime);
	double seconds = difftime(a, rawtime);
	ret = seconds;
	return ret;
}

/*
Arguments: char array
Return: true if array is representing time in good format, else returns false
*/
bool time_format_check(char key[])
{
	if ((isdigit(key[0]) * isdigit(key[1]) * isdigit(key[2]) * isdigit(key[3]) * (key[4] == '-')
		* isdigit(key[5]) * isdigit(key[6]) * (key[7] == '-') * isdigit(key[8]) * isdigit(key[9]) 
		* (key[10] == ' ') * isdigit(key[11]) * isdigit(key[12]) * (key[13] == ':') * isdigit(key[14]) 
		* isdigit(key[15]) * (key[16] == ':') * isdigit(key[17]) * isdigit(key[18])) && strlen(key) == 19) {
		return true;
	}
	else return false;
}

/*
Arguments: char array
Return: true if array is in good time format and is not in the past. Else returns false
*/
bool check_date_input(char key[]) 
{
	if (time_format_check(key)) {
		time_t time = parse_time(key);
		if (when_to_ring(time) > 0) {
			return true;
		}
	}
	else return false;
}

/*
Arguments: char array representing time
Return: nothing

Description: creates alarm from char array and saves info about it to data structure
*/
void create_alarm(char tempo[])
{
	time_t timeWait = parse_time(tempo);

	//This algorithm checks, if there is still free space in the array, to create a new alarm
	if(counter>=SIZE){
		int foundSpace=0;
		for(int i=0;i<counter;i++){
			if(list[i].alarmNr==0){
				foundSpace=1;
			}
		}
		if(foundSpace==0){
			printf("\nNo more space for a new alarm!\n");
			return;
		}
	}

	int pid;
	pid = fork();

	//This child process will execute the sleep and exec command for the sound
	if(pid == 0){
		sleep(when_to_ring(timeWait));
		printf("RING \n");
		//.mp3 file will only be executed on linux (using mpg123)
		execlp("mpg123", "mpg123", "-q", "alarm.mp3", 0);
		exit(0);
	}
	//This algortihm stores the alarm information in the list spot with the smallest index
	else if(pid > 0){
		int found=0;
		for(int i=0;i<counter;i++){
			if(list[i].alarmNr==0){
				list[i].pid = pid;
				strcpy(list[i].dateTime,tempo);
				list[i].scheduledTime = timeWait;
				list[i].alarmNr=i+1;
				found=1;
				break;
			}
		}
		if(found==0){
			list[counter].pid = pid;
			strcpy(list[counter].dateTime,tempo);
			list[counter].scheduledTime = timeWait;
			list[counter].alarmNr=counter+1;
			counter++;
		}
	}
}

/*
Arguments: character pointer
Return: true if deleting an alarm is complete, else returns false
*/
bool delete_alarm(char* key){
	
	int toDelete = -1;
	int x;
    sscanf(key, "%d", &x);

	for(int i=0;i<counter;i++)
	{
		if(list[i].alarmNr==x){
			toDelete= list[i].pid;
			list[i].alarmNr=0; 
			if(counter==i+1){
				counter--;
			}
		}
	}

	if(toDelete!=-1){ 
		int status_delete=0;
		kill(toDelete, SIGKILL);
		waitpid(toDelete, &status_delete, 0);
		return true;
	}
	return false;
}

/*
Arguments: no arguments
Return: nothing

Description: it catches zombies for the whole alarm data structure
*/
void catch_zombies(){
	int status_catch=0;
	for (int j=0; j<2; j++){
		if((list[j]).alarmNr>0){
			pid_t kid = waitpid((list[j]).pid, &status_catch, WNOHANG);
			if(kid){
				list[j] = dummy;
			}
		}
	}
}

/*
Arguments: no arguments
Return: nothing

Description: Show active processes (executes ps command)
*/
void monitor_processes(){
	pid_t child = fork();
	if(child == 0){
		char *argv[2] = {"ps", NULL};
		execv("/bin/ps", argv); 
		exit(0);
	}
	else if(child>0){
		int status_monitor;
		sleep(2);
		waitpid(child, &status_monitor, 0);
	}	
}

/*
Arguments: no arguments
Return: nothing

Description: kills all the running alarm processes from data structure
*/
void clean_up(){
	int status_clean;
	for (int j=0; j<SIZE; j++){
		kill((list[j]).pid, SIGKILL);
		waitpid((list[j]).pid, &status_clean, 0);
	}
}

/*
Arguments: no arguments
Return: nothing

Description: Prints alarms that are scheduled to the console 
*/
void list_alarms()
{
	for(int i=0;i<counter;i++)
	{
        if(list[i].alarmNr!=0){
			struct tm tm = *localtime(&list[i].scheduledTime);
  			printf("Alarm nr %d scheduled on: %d-%02d-%02d %02d:%02d:%02d\n", list[i].alarmNr, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        }
    }
}

int main()
{	
	//Declaration of different variables
	char temp;
	time_t rawtime;
	struct tm* timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	printf("Welcome to the alarm clock! It is currently %s\n", asctime(timeinfo));
	char  menu[10];
	char cancelAlarm[10];
	char alarmDate[30];
	char test[20];
	
	//This endles loop is running, until the user exits it with the input 'x'
	for (;;) {
		
		do {
			printf("\nPlease enter \"s\" (schedule), \"l\" (list), \"c\" (cancel), \"x\" (exit)\n");
			//scanf("%c", &temp);
			scanf("  %[^\n]", &menu);
		} while (!(menu[0] == 's' || menu[0] == 'l' || menu[0] == 'c' || menu[0] == 'x'));

		catch_zombies();

		switch (menu[0])
		{
		case 's':
			printf("Schedule alarm at which date and time?\n");
			do {
				printf("Input a future date [yyyy-mm-dd hh:mm:ss]: ");
				scanf("%c", &temp);
				scanf("  %[^\n]", &alarmDate);
			} while (!check_date_input(alarmDate));
			create_alarm(alarmDate);
			printf("\nDate is accepted, input finished!\n");

			break;

		case 'l':
			list_alarms();
			//monitor_processes();

			break;

		case 'c':
			printf("Cancel which alarm?\n");
			do {
				printf("Input a number: ");
				scanf("%c", &temp);
				scanf("  %[^\n]", &cancelAlarm);
			} while (!digit_check(cancelAlarm));
			bool ret = delete_alarm(cancelAlarm);
			if(ret){
				printf("Number is accepted, input finished!\n");
			}
			else{
				printf("No such alarm!\n");
			}

			break;

		case 'x':
			printf("Goodbye!\n");
			clean_up();
			return 0;

			break;

		default:
			printf("Invalid input!\n");

			break;
		}
		
	}
	//The main function can be exited
	return 0;
}