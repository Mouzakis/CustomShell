#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>

#define STRING_COMMAND 1024
#define BUFF 254



void type_prompt(){
    //printf("%s\n", getlogin());
    char dir[1024];
    printf("\n%s@cs345sh%s/$",getlogin(),getcwd(dir,1024));
    //printf("\n%s\n", dir);

}


char **split_cmd(char *input){

    char *tmp = (char*)malloc(BUFF * sizeof(char));
    char **buffer = (char**)malloc(BUFF * sizeof(char*));
    int i=1;

    char *cut = " \"\n";
    tmp = strtok(input, cut);
    buffer[0]=tmp;
    
    while(tmp != '\0'){
        tmp = strtok(NULL, cut);
        buffer[i] = tmp;
        i++;
    }
    i=0;
    /*while(buffer[i] != NULL){
        printf("'%s'", buffer[i]);
        i++;
    }*/
    return buffer;
}

void cd_cmd(char **buffer){
    char tmp[BUFF];
    char *dir = (char*)malloc(BUFF * sizeof(char));;
    

    strcpy(tmp,buffer[1]);
    getcwd(dir,1024);
    strcat(dir, "/");
    strcat(dir,tmp);
    //dir = strtok(dir, "\n");
    //printf("%s", dir);
    

    if(chdir(dir)==-1){
            printf("path not found !");
    }

}

void command_simple(char **buffer) {
	if(strcmp(buffer[0], "cd") == 0)
		cd_cmd(buffer);
	if(strcmp(buffer[0], "exit") == 0)
		exit(1);	
	return;
}



void piped(char **buffer){


    int numPipes;
    int i;
    int y;
    int j;
    int k;

    i=0;
    numPipes=0;
    while(buffer[i] != NULL){
        if(strcmp(buffer[i],">") == 0){
            numPipes++;
        }
        i++;
    }

    //printf("%d", numPipes);

    int fds[2*numPipes];
	pid_t pid;

    for(i = 0; i < numPipes; i++){
        if(pipe((fds+i*2)) < 0) {
            perror("couldn't pipe");
            exit(EXIT_FAILURE);
        }
    }

    y=0;
    for(i=0; i<=numPipes; i++){

        char **cmd = (char**)malloc(BUFF * sizeof(char*));
        j=0;
        while(buffer[y]!=NULL && strcmp(buffer[y],">")!=0){
            cmd[j] = buffer[y];
            //printf("'%s' ", cmd[j]);
            y++;
            j++;
        }
        y++;
        //printf("'%s' ", buffer[y]);

        pid=fork();

        if(pid < 0){
            perror("error");
            exit(EXIT_FAILURE);
        }
        else if(pid==0){



            if(i==0){
                dup2(fds[i*2 + 1], 1);
                for(k = 0; k < 2*numPipes; k++){
                    close(fds[k]);
                }
                //printf("%d,0 | ", (i-1)*2);
                execvp(cmd[0], cmd);
            }
            else if(i==numPipes){
                dup2(fds[(i-1)*2], 0);
                for(k = 0; k < 2*numPipes; k++){
                    close(fds[k]);
                }
                execvp(cmd[0], cmd);
                //printf("%d, 1 |", i*2 + 1);
            }
            else{
                dup2(fds[i*2 + 1], 1);
                dup2(fds[(i-1)*2], 0);
                for(k = 0; k < 2*numPipes; k++){
                    close(fds[k]);
                }
                execvp(cmd[0], cmd);
            }

        }
    }

    for(i = 0; i < 2*numPipes; i++){
        close(fds[i]);
    }


    waitpid(-1,NULL,0);

}

void redirection(char **buffer, int type, char* file){

    char **cmd = (char **)malloc(BUFF * sizeof(char *));
    int i=0;

    while(buffer[i] != NULL && strcmp(buffer[i],"|")!=0 && strcmp(buffer[i],"||")!=0 && strcmp(buffer[i],"|||")!=0){
        cmd[i]=buffer[i];
        //printf("%s", cmd[i]);
        i++;
    }

    FILE *f;

    if(type==1){
        f=fopen(file,"r");
        dup2(fileno(f), fileno(stdin));
        fclose(f);
        execvp(cmd[0], cmd);
    }
    else if(type==2){

        f=fopen(file,"w+");

        dup2(fileno(f), fileno(stdout));
        fclose(f);

        execvp(cmd[0], cmd);
    }
    else{
        f = fopen(file,"a");

        dup2(fileno(f), fileno(stdout));
        fclose(f);
        
        execvp(cmd[0], cmd);
    }

}

int main(){

    char *input =  (char*)malloc(BUFF * sizeof(char));
    char **buffer = (char**)malloc(BUFF * sizeof(char*));
    pid_t pid;
    int i;
    int n;
    int type;
    while(1){

        type_prompt();

        fgets(input,BUFF,stdin);

        buffer = split_cmd(input);


        if(buffer[0] == NULL){
            continue;
        }

        if((strcmp(buffer[0], "cd") == 0) || 
        (strcmp(buffer[0], "exit") == 0)){
            command_simple(buffer);
            continue;
        }


        i=0;
        type=404;
        while(buffer[i] != NULL){
            
            if(strchr(buffer[i], '>') != NULL){
                 //printf("Pipe");
                 type = 0;
            }
            else if(strstr(buffer[i], "|||") != NULL){
                 //printf("Double Redirection");
                 type = 3;
            }
            else if(strstr(buffer[i], "||") != NULL){
                 //printf("Single Redirection");
                 type =2;
            }
            else if(strchr(buffer[i], '|') != NULL){
                 //printf("Input Redirection");
                 type = 1;
            }
            else{
                i++;
                continue;
            }
            i++;
        }


        pid=fork();
        if(pid<0){
            perror("Fork Failed !");
            exit(0);
        }
        if(pid == 0){

            if(type == 404){
                execvp(buffer[0], buffer);
            }
            else if(type==0){
                piped(buffer);
            }
            else if(type >=1 && type <=3){
                i=0;
                while(buffer[i] != NULL){
                    i++;
                }
                char *file=buffer[i-1];
                redirection(buffer, type, file);
            }
        }
        else if(pid > 0){
            waitpid(pid, NULL,0);
        }
               
        
    }
    free(input);
    free(buffer);
    return 0;
}