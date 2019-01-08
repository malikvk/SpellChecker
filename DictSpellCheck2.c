/*
TO RUN:
 gcc -o Dict DictSpellCheck.c -w -std=c99 -pthread
 ./Dict
 duplicate server
 enter:
 ssh 'cis-client06' //or whatevr number the original was.
 telnet localhost 1025

*/
/****************************************************************/
#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<errno.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<semaphore.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_DATA 1024

char **dictionary;


void* getUserData(void *arg);
char **parseString(char *userWords);
int queueItems = 0;
int numOfThreads = 3;
sem_t mySema;
const char *mySemaphore1= "mySemaphore";
sem_t numberOfFullSpaces;
const char *numberOfFullSpaces1 = "numberOfFullSpaces";
int numberOfArguments;
int ArraySizeCounter = 0;

//Mutex initializers
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexQueLock = PTHREAD_MUTEX_INITIALIZER;

struct SocketQueue
{
    //will hold the socket descriptors for each connection
    int queueSock;
};

struct SocketQueue sockets[5];

int main(int argc , char ** argv)
{
    int ArraySize=0;
    FILE * dictFP;
    int read;
    char * line = NULL;
    size_t len =0;   

    int sock;    
    int newSock;
    struct sockaddr_in server;
    struct sockaddr_in client;    
    int sockaddr_len;
    sockaddr_len= sizeof(struct sockaddr_in);
   
   	sem_init(&mySema, 0, 1);
    if (sem_init(&mySema, 0, 1) == -1) 
	{
       perror("sem_init failed:");
    }//end if
     
    printf("\n%d\n",sem_init(&mySema, 0, 1));
    sem_init(&numberOfFullSpaces, 0, 0);
       
    //Default
    if (argc == 1) 
	{           
        dictFP = fopen("american-english", "r");
        if (dictFP == NULL) 
		{
            perror("file not found");
            exit(-1);
        }//end if
        while ((read = getline(&line, &len, dictFP) != -1)) 
		{
            ArraySize++;
        }//end while
        
        dictionary= (char**)calloc(ArraySize,sizeof(char*));           
      
        fclose(dictFP);
        dictFP = fopen("american-english", "r");
        int dictIndex = 0;
    
        while ((read = getline(&line, &len, dictFP) != -1)) 
		{
            dictionary[dictIndex]= (char *) calloc(strlen(line),sizeof(char));
            strncpy(dictionary[dictIndex],line,strlen(line)-1);
            strcat(dictionary[dictIndex],"\0");
            dictIndex++;
            
			ArraySizeCounter++;
        }//end while
        
        puts("");
        
    	fclose(dictFP);
        
        //port used to connect
        int spellCheckPort = 1025;
      
        //create the socket
         //used if statement for error checking
        if ((sock = socket(AF_INET,SOCK_STREAM, 0)) == -1) 
		{
            perror("server socket");
            exit(-1);
        }//end if
    	
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = INADDR_ANY;
        server.sin_port = htons(spellCheckPort);
        bzero(&server.sin_zero, 0);
        
   		// next we bind the socket to the port
        //used if statement for error checking
        if ((bind(sock, (struct sockaddr *)&server, sizeof(server))) == -1) 
		{
            perror("bind: ");
            exit(-1);
        }//end if
                
        if ((listen(sock, 5)) == -1) 
		{
            perror("listen: ");
            exit(-1);
        }//end if
             
        int emptyIndex = 0;
                
        // just a variable to hold the number of threads i want to create
        pthread_t tids[numOfThreads];
      	
		int b;
        for ( b = 0; b < numOfThreads; b++) 
		{
            pthread_create(&tids[b], NULL, getUserData,NULL);
        }//end for
                
        while(1) 
			{            	
            	if ( (newSock = accept(sock, (struct sockaddr *)&client, &sockaddr_len)) == -1) 
				{
                	perror("accept failed");
            	}//end if
                       
            	if (sem_wait(&mySema) == -1) 
				{
                 	perror("sem_wait failed");    
            	}//end if
                
            	printf("New Client connected from port no %d and IP %s\n", ntohs(client.sin_port), inet_ntoa(client.sin_addr));
            
            	pthread_mutex_lock(&mutexQueLock);
            
			int emptyIndex;
            for (emptyIndex = 0; emptyIndex < 5 ; emptyIndex++) 
			{
                if (sockets[emptyIndex].queueSock == NULL) 
				{
                    sockets[emptyIndex].queueSock= newSock;
                          
						   pthread_mutex_unlock(&mutexQueLock);
                    
                    if ( sem_post(&numberOfFullSpaces) == -1) 
					{
                            perror("sem_post failed");
                    }//end if                                       
                    		break;
                }//end if
            }//end for
         }//end while
    }//end if
    
    if (argc == 2) 
	{
        dictFP = fopen(argv[1], "r");
        if (dictFP == NULL) 
		{
            perror("file not found");
            exit(-1);
        }//end if
        
        while ((read = getline(&line, &len, dictFP) != -1)) 
		{
            ArraySize++;
        }//end while
        
        // allocate array of pointers here
        dictionary = (char**)calloc(ArraySize, sizeof(char*));
               
        fclose(dictFP);
        dictFP = fopen(argv[1], "r");
        int dictIndex = 0;
        while ((read = getline(&line, &len, dictFP) != -1)) 
		{
            dictionary[dictIndex] = (char *) calloc(strlen(line), sizeof(char));
            strncpy(dictionary[dictIndex], line, strlen(line)-1);
            strcat(dictionary[dictIndex],"\0");
            dictIndex++;           
            ArraySizeCounter++;
        }//end while        
        
        puts("");
        
        fclose(dictFP);       
        
        //port to connect
        int spellCheckPort = 1025;       
                
        if ((sock = socket(AF_INET,SOCK_STREAM, 0)) == -1) 
		{
            perror("server socket");
            exit(-1);
        }//end if
        
        
        server.sin_family=AF_INET;
        server.sin_addr.s_addr= INADDR_ANY;
        server.sin_port= htons(spellCheckPort);
        bzero(&server.sin_zero, 0);
        
        if ((bind(sock, (struct sockaddr *)&server, sizeof(server))) == -1) 
		{
            perror("bind: ");
            exit(-1);
        }//end if
        
        if ((listen(sock, 5)) == -1) 
		{
            perror("listen: ");
            exit(-1);
        }//end if
        
        pthread_t tids[numOfThreads];
        
        int b;
        for (b = 0; b < numOfThreads; b++) 
		{
            pthread_create(&tids[b], NULL, getUserData,NULL);
        }//end for
              
        while (1) 
		{
            
            if ((newSock = accept(sock, (struct sockaddr *)&client,&sockaddr_len)) == -1) 
			{
                perror("accept failed");
            }//end if
                      
            if (sem_wait(&mySema) == -1) 
			{
                perror("sem_wait failed");                
            }//end if
                     
            printf("New Client connected from port no %d and IP %s\n", ntohs(client.sin_port), inet_ntoa(client.sin_addr));
            
            pthread_mutex_lock(&mutexQueLock);
            
            int emptyIndex;
            for (emptyIndex = 0; emptyIndex < 5 ; emptyIndex++) 
			{
                if (sockets[emptyIndex].queueSock == NULL) 
				{
                    sockets[emptyIndex].queueSock= newSock;
                    pthread_mutex_unlock(&mutexQueLock);
                    
                    if ( sem_post(&numberOfFullSpaces) == -1) 
					{
                        perror("sem_post failed");
                    }//end if
                   			break;
                }//end if
            }//end for
    	}//end while
      
    }//end if
    
    if (argc == 3) 
	{
        dictFP = fopen(argv[1], "r");
        if (dictFP == NULL) 
		{
            perror("file not found");
            exit(-1);
        }//end if        
		while ((read = getline(&line, &len, dictFP) != -1)) 
		{
            ArraySize++;
        }//end while
        
        // allocate array of pointers
        dictionary = (char**)calloc(ArraySize,sizeof(char*));
               
        fclose(dictFP);
        dictFP = fopen(argv[1], "r");
        int dictIndex = 0;
        
        while ((read = getline(&line, &len, dictFP) != -1)) 
		{
            dictionary[dictIndex]= (char *) calloc(strlen(line),sizeof(char));
            strncpy(dictionary[dictIndex],line,strlen(line)-1);
            strcat(dictionary[dictIndex],"\0");
            dictIndex++;
            ArraySizeCounter++;
        }//end while
        
        puts("");
        
        fclose(dictFP);
                
        int spellCheckPort= atoi (argv[2]);
     
        if ((sock = socket(AF_INET,SOCK_STREAM, 0)) == -1) 
		{
            perror("server socket");
            exit(-1);
        }//end if
        
    
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = INADDR_ANY;
        server.sin_port= htons(spellCheckPort);
        bzero(&server.sin_zero, 0);
        
        if ((bind(sock, (struct sockaddr *)&server, sizeof(server))) == -1) 
		{
            perror("bind: ");
            exit(-1);
        }//end if
    
        if ((listen(sock, 5)) == -1) 
		{
            perror("listen: ");
            exit(-1);
        }//end if
      
        int emptyIndex = 0;
		
		pthread_t tids[numOfThreads];
                
        int b;
        for (b = 0; b < numOfThreads; b++) 
		{
            pthread_create(&tids[b], NULL, getUserData,NULL);
        }//end for
                
        while (1) 
			{
            	//do accept with error checking
            	if ((newSock = accept(sock, (struct sockaddr *)&client,&sockaddr_len)) == -1) 
				{
                	perror("accept failed");
            	}//end if   
                      
            
            	if (sem_wait(&mySema) == -1) 
				{
                	perror("sem_wait failed");
                }//end if
                        
            printf("New Client connected from port no %d and IP %s\n", ntohs(client.sin_port), inet_ntoa(client.sin_addr));
            
            pthread_mutex_lock(&mutexQueLock);
            
            int emptyIndex;
            for (emptyIndex=0;emptyIndex<5 ; emptyIndex++) 
			{
                if (sockets[emptyIndex].queueSock == NULL)
				 {
                    sockets[emptyIndex].queueSock= newSock;
                    pthread_mutex_unlock(&mutexQueLock);
                    
                    if ( sem_post(&numberOfFullSpaces) == -1)
					{
                        perror("sem_post failed");
                    }//end if            
                    
                    		break;
                }//end if
            }//end for             
        }//end while        
    }//end if(3)
}//end main

/****************************************************************************************/
char **parseString(char *userWords)
{
    numberOfArguments = 0;
    char *delimeter = " ";
    char *token;
    char **argArray;
    int i;
   
    argArray = (char**)calloc(50,sizeof(char*));
    
    userWords[strlen(userWords)-1] = ' ';
     token = strtok(userWords,delimeter);
    while (token != NULL) 
	{
        argArray[numberOfArguments] = (char *) calloc(strlen(token),sizeof(char));
        strcpy(argArray[numberOfArguments],token);
        token = strtok(NULL,delimeter);        
        numberOfArguments++;    
    }//end while   
    
    return argArray;
	    
}//end parseString
/*******************************************************************************************/

// will echo back to user if spelling is correct 
void* getUserData(void *arg)
{    
    char *data   = (char*) malloc(sizeof(char)*MAX_DATA);
    char *output = (char*) malloc(sizeof(char)*MAX_DATA);
	
    int isFound  = 0;
    int datalen  = 1;
    int newSock2 = 0;
    
	while (1) 
	{        
        sem_wait(&numberOfFullSpaces);
        pthread_mutex_lock(&mutex);
        int count = 0;
            
            int isFound;
            for (isFound=0; isFound<5; isFound++) 
			{                  
                if (sockets[isFound].queueSock != NULL) 
				{
                    newSock2 = sockets[isFound].queueSock;
                    char connected[250];
                    
                    strcpy(connected,"Spell-Check Server\n\"VALID\" will print if spelling is correct\nif not you will see a \"mispelled or not found error\" \nEXIT:\nPress Enter\n");
                    
                    send(newSock2, connected, strlen(connected), 0);
                    
                    while (datalen) 
					{                        
                        datalen=recv(newSock2, data, MAX_DATA, 0);
                        
                        if (datalen) 
						{                                                       
                            data= strtok(data, "\n");                          
                            char** userWords = parseString(data);
                            
                            int i;
                            for (i = 0; userWords[i]; i++) 
							{                               
                                int check = 0;
                                int c;
                                for (c = 0; c < ArraySizeCounter; c++) 
								{
                                    if (check == 1) 
									{
                                        break;
                                    }//end if
                                     
                                    if (strcmp(dictionary[c], userWords[i]) == 0) 
									{
                                        check = 1;
                                        char found[50];
                                        strcpy(found, " -> WORD IS VALID\n");
                                        char result[50];
                                        strcpy(result, userWords[i]);                                        
                                        strcat(result, found);                                        
                                        send(newSock2, result, strlen(result), 0);                                        
                                        fprintf(stderr,"%s WORD IS VALID\n",userWords[i]);              
                                    }//end if
                                }//end for
                                
                                if (check == 0) 
								{
                                    char notFound[50];
                                    strcpy(notFound," -> Word: NOT FOUND OR MISPELLED\n");
                                    char result[50];
                                    strcpy(result,userWords[i]);                                    
                                    strcat(result,notFound);                                    
                                    send(newSock2, result, strlen(result), 0);
                                } //end if          
                            }//end for        
                        }//end if
                    }//end while
                    
                    sockets[isFound].queueSock= NULL;
                    queueItems--;
                    pthread_mutex_unlock(&mutex);
                    sem_post(&mySema);
                    break;
            }//end if
        }//end for
    }//end while
        
   printf("Client Disconnected");
   close(newSock2);
  
}//end userget

/****************************************************************************************/



