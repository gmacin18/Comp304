#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h> // termios, TCSANOW, ECHO, ICANON
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <stddef.h>

const char * sysname = "shellax";

enum return_codes {
	SUCCESS = 0,
	EXIT = 1,
	UNKNOWN = 2,
};

struct command_t {
	char *name;
	bool background;
	bool auto_complete;
	int arg_count;
	char **args;
	char *redirects[3]; // in/out redirection
	struct command_t *next; // for piping
};

const char* path_finder(char *cmd);
void pipe_handler (struct command_t *command, int temp_fd);


/**
 * Prints a command struct
 * @param struct command_t *
 */
void print_command(struct command_t * command)
{
	int i=0;
	printf("Command: <%s>\n", command->name);
	printf("\tIs Background: %s\n", command->background?"yes":"no");
	printf("\tNeeds Auto-complete: %s\n", command->auto_complete?"yes":"no");
	printf("\tRedirects:\n");
	for (i=0;i<3;i++)
		printf("\t\t%d: %s\n", i, command->redirects[i]?command->redirects[i]:"N/A");
	printf("\tArguments (%d):\n", command->arg_count);
	for (i=0;i<command->arg_count;++i)
		printf("\t\tArg %d: %s\n", i, command->args[i]);
	if (command->next)
	{
		printf("\tPiped to:\n");
		print_command(command->next);
	}


}
/**
 * Release allocated memory of a command
 * @param  command [description]
 * @return         [description]
 */
int free_command(struct command_t *command)
{
	if (command->arg_count)
	{
		for (int i=0; i<command->arg_count; ++i)
			free(command->args[i]);
                /// TODO: do your own exec with path resolving using execv()
		free(command->args);
	}
	for (int i=0;i<3;++i)
		if (command->redirects[i])
			free(command->redirects[i]);
	if (command->next)
	{
		free_command(command->next);
		command->next=NULL;
	}
	free(command->name);
	free(command);
	return 0;
}
/**
 * Show the command prompt
 * @return [description]
 */
int show_prompt()
{
	char cwd[1024], hostname[1024];
    gethostname(hostname, sizeof(hostname));
	getcwd(cwd, sizeof(cwd));
	printf("%s@%s:%s %s$ ", getenv("USER"), hostname, cwd, sysname);
	return 0;
}
/**
 * Parse a command string into a command struct
 * @param  buf     [description]
 * @param  command [description]
 * @return         0
 */
int parse_command(char *buf, struct command_t *command)
{
	const char *splitters=" \t"; // split at whitespace
	int index, len;
	len=strlen(buf);
	while (len>0 && strchr(splitters, buf[0])!=NULL) // trim left whitespace
	{
		buf++;
		len--;
	}
	while (len>0 && strchr(splitters, buf[len-1])!=NULL)
		buf[--len]=0; // trim right whitespace

	if (len>0 && buf[len-1]=='?') // auto-complete
		command->auto_complete=true;
	if (len>0 && buf[len-1]=='&') // background
		command->background=true;

	char *pch = strtok(buf, splitters);
	command->name=(char *)malloc(strlen(pch)+1);
	if (pch==NULL)
		command->name[0]=0;
	else
		strcpy(command->name, pch);

	command->args=(char **)malloc(sizeof(char *));

	int redirect_index;
	int arg_index=0;
	char temp_buf[1024], *arg;
	while (1)
	{
		// tokenize input on splitters
		pch = strtok(NULL, splitters);
		if (!pch) break;
		arg=temp_buf;
		strcpy(arg, pch);
		len=strlen(arg);

		if (len==0) continue; // empty arg, go for next
		while (len>0 && strchr(splitters, arg[0])!=NULL) // trim left whitespace
		{
			arg++;
			len--;
		}
		while (len>0 && strchr(splitters, arg[len-1])!=NULL) arg[--len]=0; // trim right whitespace
		if (len==0) continue; // empty arg, go for next

		// piping to another command
		if (strcmp(arg, "|")==0)
		{
			struct command_t *c=malloc(sizeof(struct command_t));
			int l=strlen(pch);
			pch[l]=splitters[0]; // restore strtok termination
			index=1;
			while (pch[index]==' ' || pch[index]=='\t') index++; // skip whitespaces

			parse_command(pch+index, c);
			pch[l]=0; // put back strtok termination
			command->next=c;
			continue;
		}

		// background process
		if (strcmp(arg, "&")==0)
			continue; // handled before

		// handle input redirection
		redirect_index=-1;
		if (arg[0]=='<')
			redirect_index=0;
		if (arg[0]=='>')
		{
			if (len>1 && arg[1]=='>')
			{
				redirect_index=2;
				arg++;
				len--;
			}
			else redirect_index=1;
		}
		if (redirect_index != -1)
		{
			command->redirects[redirect_index]=malloc(len);
			strcpy(command->redirects[redirect_index], arg+1);
			continue;
		}

		// normal arguments
		if (len>2 && ((arg[0]=='"' && arg[len-1]=='"')
			|| (arg[0]=='\'' && arg[len-1]=='\''))) // quote wrapped arg
		{
			arg[--len]=0;
			arg++;
		}
		command->args=(char **)realloc(command->args, sizeof(char *)*(arg_index+1));
		command->args[arg_index]=(char *)malloc(len+1);
		strcpy(command->args[arg_index++], arg);
	}
	command->arg_count=arg_index;
	return 0;
}

void prompt_backspace()
{
	putchar(8); // go back 1
	putchar(' '); // write empty over
	putchar(8); // go back 1 again
}
/**
 * Prompt a command from the user
 * @param  buf      [description]
 * @param  buf_size [description]
 * @return          [description]
 */
int prompt(struct command_t *command)
{
	int index=0;
	char c;
	char buf[4096];
	static char oldbuf[4096];

    // tcgetattr gets the parameters of the current terminal
    // STDIN_FILENO will tell tcgetattr that it should write the settings
    // of stdin to oldt
    static struct termios backup_termios, new_termios;
    tcgetattr(STDIN_FILENO, &backup_termios);
    new_termios = backup_termios;
    // ICANON normally takes care that one line at a time will be processed
    // that means it will return if it sees a "\n" or an EOF or an EOL
    new_termios.c_lflag &= ~(ICANON | ECHO); // Also disable automatic echo. We manually echo each char.
    // Those new settings will be set to STDIN
    // TCSANOW tells tcsetattr to change attributes immediately.
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);


    //FIXME: backspace is applied before printing chars
	show_prompt();
	int multicode_state=0;
	buf[0]=0;
  	while (1)
  	{
		c=getchar();
		// printf("Keycode: %u\n", c); // DEBUG: uncomment for debugging

		if (c==9) // handle tab
		{
			buf[index++]='?'; // autocomplete
			break;
		}

		if (c==127) // handle backspace
		{
			if (index>0)
			{
				prompt_backspace();
				index--;
			}
			continue;
		}
		if (c==27 && multicode_state==0) // handle multi-code keys
		{
			multicode_state=1;
			continue;
		}
		if (c==91 && multicode_state==1)
		{
			multicode_state=2;
			continue;
		}
		if (c==65 && multicode_state==2) // up arrow
		{
			int i;
			while (index>0)
			{
				prompt_backspace();
				index--;
			}
			for (i=0;oldbuf[i];++i)
			{
				putchar(oldbuf[i]);
				buf[i]=oldbuf[i];
			}
			index=i;
			continue;
		}
		else
			multicode_state=0;

		putchar(c); // echo the character
		buf[index++]=c;
		if (index>=sizeof(buf)-1) break;
		if (c=='\n') // enter key
			break;
		if (c==4) // Ctrl+D
			return EXIT;
  	}
  	if (index>0 && buf[index-1]=='\n') // trim newline from the end
  		index--;
  	buf[index++]=0; // null terminate string

  	strcpy(oldbuf, buf);

  	parse_command(buf, command);

  	// print_command(command); // DEBUG: uncomment for debugging

    // restore the old settings
    tcsetattr(STDIN_FILENO, TCSANOW, &backup_termios);
  	return SUCCESS;
}
int process_command(struct command_t *command);
int main()
{
	while (1)
	{
		struct command_t *command=malloc(sizeof(struct command_t));
		memset(command, 0, sizeof(struct command_t)); // set all bytes to 0

		int code;
		code = prompt(command);
		if (code==EXIT) break;

		code = process_command(command);
		if (code==EXIT) break;

		free_command(command);
	}

	printf("\n");
	return 0;
}

int process_command(struct command_t *command)
{
	int r;
	if (strcmp(command->name, "")==0) return SUCCESS;

	if (strcmp(command->name, "exit")==0)
		return EXIT;

	if (strcmp(command->name, "cd")==0)
	{
		if (command->arg_count > 0)
		{
			r=chdir(command->args[0]);
			if (r==-1)
				printf("-%s: %s: %s\n", sysname, command->name, strerror(errno));
			return SUCCESS;
		}
	}

	if (strcmp(command->name, "uniq")==0){
                FILE *f;

                char input[512]={"input"};

                f = fopen (command->args[command->arg_count-1], "r");
		int elem_count = 0;
		char elem_array[100][100];

		while(fgets(input, sizeof(input), f)!= NULL){
                	input[strlen(input)-1]='\0';
			strcpy(elem_array[elem_count++], input);
		}

		char uniq_elems[100][100];
		int count=0;
		for(int i=0; i<elem_count; i++){
			bool contains = false;
		
			for(int j=0; j<elem_count; j++){
				if(strcmp(uniq_elems[j], elem_array[i])==0){
					contains = true;
					break;
				}
			}
			if(!contains){
				strcpy(uniq_elems[count], elem_array[i]);
				count++;
			}
		}
			

		if(command->arg_count==1){
	                for(int i=0; i<count; i++){
        	                printf("%s\n", uniq_elems[i]);
               		}
		
		}else if(command->arg_count == 2){
			if(strcmp(command->args[0],"-c")||strcmp(command->args[0],"--count")){
				int occ_arr[count];
				for(int i=0; i<count; i++){
					occ_arr[i] = 0;
				}
				for(int k=0; k<elem_count; k++){
					for(int j=0; j<count; j++){
						if(strcmp(elem_array[k], uniq_elems[j])==0){
							occ_arr[j]++;
							break;
						}
					}
				}
	                        for(int i=0; i<count; i++){
	                                printf("%d  %s\n",occ_arr[i], uniq_elems[i]);
                        }

			}

		}else {
			printf("Error occured.\n");
		}
		fclose(f);	
        	return SUCCESS;
	}
	
	if(strcmp(command->name, "wiseman")==0){
		int min = 0;
		pid_t child;
		char temp[150];

		char *min_ptr = command->args[0];
		if (min_ptr != NULL)
		{
		  min = atoi(min_ptr);
		}
		
		FILE *file_ptr = fopen("temp.txt", "w");
		fprintf(file_ptr, "*/%d * * * * fortune | espeak\n", min);
		fclose(file_ptr);
		int childStatus;
		child = fork();
		  
		if(child == 0) {
		  char *cronArgs[] = {
		    "/usr/bin/crontab",
		    "temp.txt",
		    0
		  };
		  execv(cronArgs[0], cronArgs);
		}
		else {
		  waitpid(child, &childStatus, 0);
		}
		remove("temp.txt"); //remove temp
		return SUCCESS;

	}
	
	
	//Custom command from Gülbarin Maçin
	if(strcmp(command->name,"horoscope")==0){
        	int day,month;  
   	        char horoscope[100];

		char *day_ptr = command->args[0];
                if (day_ptr != NULL)
                {
                  day = atoi(day_ptr);
                }

		char *month_ptr = command->args[1];
                if (month_ptr != NULL)
                {
                  month = atoi(month_ptr);
                }
	
    	    if ((month == 12 && day >= 12) || (month == 1 & day<= 20))
            {
                strcpy(horoscope,"espeak 'Your horoscope is Capricorn'");
	
            }
 
            else if ((month == 9 & day >= 22) || (month == 10 & day <= 23))
            {
            
                strcpy(horoscope,"espeak 'Your horoscope is Libra'");
            }
 
            else if ((month == 1 & day >= 21) || (month == 2 & day <= 19))
            {
               strcpy(horoscope, "espeak 'Your horoscope is Aquarius'");
                
            }
 
            else if ((month == 2 & day >= 20) || (month == 3 & day <= 20))
            {
                strcpy(horoscope,"espeak 'Your horoscope is Pisces'");
            }
 
            else if ((month == 3 & day >= 21) || (month== 4 & day <= 20))
            {
                strcpy(horoscope,"espeak 'Your horoscope is Aries'");
            }
 
            else if ((month == 4 & day >= 21) || (month == 5 & day <= 21))
            {
                strcpy(horoscope,"espeak 'Your horoscope is Taurus'");
            }
 
            else if ((month == 5 & day >= 22) || (month == 6 & day <= 21))
            {
                strcpy(horoscope, "espeak 'Your horoscope is Gemini'");
            }
 
            else if ((month == 6 & day >= 22) || (month == 7 & day <= 23))
            {
                strcpy(horoscope, "espeak 'Your horoscope is Cancer'");
            }
 
            else if ((month == 7 & day >= 24) || (month == 8 & day <= 23))
            {
                strcpy(horoscope,"espeak 'Your horoscope is Leo'");
            }
 
            else if ((month == 8 & day >= 24) || (month == 9 & day <= 23))
            {
               	strcpy(horoscope, "espeak 'Your horoscope is Virgo'");
            }
  
            else if ((month == 10 & day >= 23) || (month == 11 & day <= 22))
            {
                strcpy(horoscope,"espeak 'Your horoscope is Scorpio'");
            }
 
            else if ((month == 11 & day >= 23) || (month == 12 & day <= 22))
            {
                strcpy(horoscope, "espeak 'Your horoscope is Sagittarius'");
            } 	
            
            system(horoscope);
	return SUCCESS;
}
	
	//Custom command from Çisem Özden
	if(strcmp(command->name, "ku-help")==0){
		printf("\nHello! From here you can reach any of the following helpful pages related to Koç University.\n");
		printf("1- Kusis\n2- Blackboard\n3- Faculty-page (Also specify the code of the faculty)\n     - eng (college of engineering)\n     - cssh (college of social sciences)\n     - case (college of administrative sciences and economics)\n     - science (college of sciences)\n\n");
		printf("4- OIP (Office of International Programs)\n\n");

		char cmd[100];

		printf("\nPlease specify the name  of the website you want to reach.\n");
		scanf("%[^\n]",cmd);
		
		if(strcmp(cmd, "Kusis")==0){
			system("xdg-open https://kusis.ku.edu.tr/psp/ps/?cmd=login");
			return SUCCESS;
		}else if(strcmp(cmd, "Blackboard")==0){
			system("xdg-open https://ku.blackboard.com/");
			return SUCCESS;
		}else if(strcmp(cmd, "OIP")==0){
			system("xdg-open https://oip.ku.edu.tr/");
			return SUCCESS;
		}else if(strchr(cmd, ' ') != NULL){
			char * arg1 = strtok(cmd, " ");
			char * arg2 = strtok(NULL, " ");
			if(strcmp("Faculty-page", arg1)==0 & (strcmp(arg2, "case")==0 | strcmp(arg2,"cssh" )==0 | strcmp(arg2, "eng")==0 | strcmp(arg2, "science")==0)){
				char arg[100] = "xdg-open https://";
				strcat(arg, arg2);
				strcat(arg, ".ku.edu.tr/");
				system(arg);
				return SUCCESS;
			}
			else{
			printf("You entered wrong code.\n");
			return EXIT;
			}
		}else{
			printf("You entered wrong code.\n");
			return EXIT;
		}	
	}
	
	if (strcmp(command->name, "psvis")==0){
                if(command->arg_count !=2){
                        printf("Psvis requires two arguments.");
                        return EXIT;
                }
        
          	pid_t pid = fork();
                if (pid == 0) //child process
                {
                        char function[150];
                        strcpy(function,"sudo insmod mymodule.ko ");
                        strcat(function, command->args[0]);
                        system(function);

                } else {
                        pid_t pid2 = fork();
                        if (pid2 == 0) {
                                char function[150];
                                strcpy(function, "sudo rmmod mymodule");
                                system(function);

                        } else {
                        char function[150];
                        strcpy(function,command->args[1]);
                        strcat(function, "sudo dmesg -c");

                        system(function);

                }
        }

        return SUCCESS;
}
	
	
	int output_f; //new

	pid_t pid=fork();
	if (pid==0) // child
	{
		/// This shows how to do exec with environ (but is not available on MacOs)
	        // extern char** environ; // environment variables
		// execvpe(command->name, command->args, environ); // exec+args+path+environ

		/// This shows how to do exec with auto-path resolve
		// add a NULL argument to the end of args, and the name to the beginning
		// as required by exec

		// increase args size by 2
		command->args=(char **)realloc(
			command->args, sizeof(char *)*(command->arg_count+=2));

		// shift everything forward by 1
		for (int i=command->arg_count-2;i>0;--i)
			command->args[i]=command->args[i-1];

		// set args[0] as a copy of name
		command->args[0]=strdup(command->name);
		// set args[arg_count-1] (last) to NULL
		command->args[command->arg_count-1]=NULL;

		command->arg_count--; //new
//		execvp(command->name, command->args); // exec+args+path
//		exit(0);

		//handles the I/O redirection
		if(command->redirects[1]!=NULL){  //for I/O redirection '>' (truncate)
		    output_f = open(command->args[command->arg_count-1],  O_WRONLY | O_CREAT | O_TRUNC ,S_IRUSR | S_IWUSR | S_IRGRP);
		    dup2(output_f,1);
		    close(output_f);
		    command->args[command->arg_count-1]=NULL;


		}else if(command->redirects[2]!=NULL){ //for I/O redirection '>>' (append)
		    output_f = open(command->args[command->arg_count-1], O_WRONLY | O_CREAT | O_APPEND,S_IRUSR | S_IWUSR | S_IRGRP);
		    dup2(output_f,1);
		    close(output_f);
		    command->args[command->arg_count-1]=NULL;
		}else if(command->redirects[0]!= NULL){ //for I/O redirection '<' (input)
		    FILE *f;
		    char input[150]={"input"};
		    f = fopen (command->args[1], "r");
		    fgets(input,150,f);
		    input[strlen(input)-1]='\0';

		    char *const *input_f= (char *const *) input;
		    execv(command->name, input_f);

		} else if(command->next != NULL) {  // pipe command
		    pipe_handler(command,STDIN_FILENO);  
		    return SUCCESS;
		}


		// TODO: do your own exec with path resolving using execv()
		char *env_path = getenv("PATH");
		char env_array[512][512];
	
		char *currentDir = getenv("PWD");
		char *token;

		strcat(currentDir, "/");
		strcat(currentDir, command->name);


		token = strtok(env_path, ":");


		int num_of_env = 0;
		while(1){
			if(token==NULL){
				break;
			}
			
			strcpy(env_array[num_of_env], token);
			num_of_env++;
			token = strtok(NULL, ":");
		}

		if (execv(currentDir, command->args) != -1) exit(0);

		for(int i = 0; i<num_of_env; i++) {
			strcat(env_array[i], "/");
			strcat(env_array[i], command->name);
			if (execv(env_array[i], command->args) != -1) exit(0);
		}

		// If command is not found in any of system paths or current path, printing error message.
		printf("-%s: %s: command not found\n", sysname, command->name);
		exit(0);
	}
	else
	{
    // TODO: implement background processes here
    		if(!command->background)
    			wait(0); // wait for child process to finish
		return SUCCESS;
	}

	// TODO: your implementation here

	printf("-%s: %s: command not found\n", sysname, command->name);
	return UNKNOWN;
}

const char* path_finder(char *c) {    
    FILE *f;
    char *command_path=malloc(150);
    char arg[150]="which ";
    strcat(arg,c); 
    strcat(arg," > temp.txt");
    system(arg); //writes the path to the text file
    f = fopen ("temp.txt", "r");   
    fgets(command_path,150,f);
    int length = strlen(command_path);
    command_path[length-1]='\0';
    system("rm temp.txt");//removes the temporarily used file  
    
    return command_path;
}

void pipe_handler (struct command_t *command, int temp_fd) {
    if (command->next==NULL) {
        const char *command_path=path_finder(command->name);
        dup2(temp_fd, 0);
        if (command->arg_count>0 & strcmp(command->name,command->args[0])!=0) {
		command->arg_count +=1;
		command->args = (char **) realloc(command->args, sizeof(char *) * (command->arg_count));
                command->args[0] = command->args[1];		
		for (int i=1; i<command->arg_count-1; i++){
			command->args[i] = command->args[i-1]; 
		}

                command->args[0] = strdup(command->name);
        }
        execv(command_path, command->args);
        return;
    }

    int fd[2]; 
    if(pipe(fd) == -1) { //pipe is created
        printf("Error when creating the pipe. \n");
        exit(1);
    }


    struct command_t *temp;
    temp = command->next;

    const char *path1=path_finder(command->name);
    const char *path2=path_finder(temp->name);


    if(fork() == 0){
        dup2(temp_fd,0);
        dup2(fd[1], 1);
        close(fd[0]); //read-end is closed
        if (command->arg_count>0 & strcmp(command->name,command->args[0])!=0) {
		command->arg_count +=1;
                command->args = (char **) realloc(command->args, sizeof(char *) * (command->arg_count));
		command->args[0] = command->args[1];
                for (int i=0; i<command->arg_count-1; i++){
			command->args[i] = command->args[i-1];
		}
                command->args[0] = strdup(command->name);
        }
        execv(path1, command->args);
    }
    else {
        wait(0);
        close(fd[1]); //write-end is closed
	if (temp->arg_count==0) {
	    temp->arg_count +=1;
            temp->args = (char **) realloc(temp->args, sizeof(char *) * (temp->arg_count));
            temp->args[0] = strdup(temp->name);
        }
        pipe_handler(temp,fd[0]);
        return;
    }
}
