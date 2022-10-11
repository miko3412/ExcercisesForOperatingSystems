#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <pthread.h>

#include "sem.h"
#include "bbuffer.h"


#define MAX_BUF (2048)
char www_path[100];

/*
    Takes a socket from buffer and handles the connection (read HTTP request, open file and send HTTP response with html)

    @param bb: pointer to bounded buffer 
    @return
*/
void* thread_function(BNDBUF* bb){
    for(;;){
        unsigned char message[MAX_BUF], body[MAX_BUF], msg[MAX_BUF];
        int new_s = bb_get(bb);
        FILE *sendFile;
        int result = recv(new_s, message, MAX_BUF, 0);
        if(result == 0){
            printf("Peer was disconnected\n");
            break;
        }
        else if(result < 0){
            printf("ERROR: %s\n", strerror(errno));
            exit(-4);
        }
        message[result] = '\0';

        char toOpen[100];
        char requestPath[100];
        char* mess_ptr = &message;
        parseRequest(&mess_ptr, &requestPath);
        snprintf(toOpen, sizeof(toOpen), "%s%s", www_path, requestPath);
        sendFile = fopen(toOpen, "r");
        if(sendFile != NULL) {
            //below in the condition brackets serwer security problem is handled
            if(strstr(toOpen, ".html") != NULL && strstr(toOpen, "..") == NULL) {
                
                fseek(sendFile, 0, SEEK_END);
                long size = ftell(sendFile);
                fseek(sendFile, 0, SEEK_SET);
                char send_buffer[1000]={0};
                snprintf (msg, sizeof (msg),"HTTP/0.9 200 OK\nContent-Type: text/html\nContent-Length: %d\n\n",size);
                result=send(new_s, msg, strlen(msg), MSG_NOSIGNAL );
                while( !feof(sendFile) )
                {
                    int numread = fread(send_buffer, sizeof(unsigned char), 1000, sendFile);
                    if( numread < 1 ) break;
                    snprintf (msg, sizeof (msg),"%s", send_buffer);
                    result=send(new_s, msg, strlen(msg), MSG_NOSIGNAL );
                    memset(send_buffer,0,strlen(send_buffer));
                }
            }
            else{
                snprintf (body, sizeof (body),"<html>\n<body>\n<h1>ERROR 403 Requested unavailable resources</h1>\n</body>\n</html>\n");
                snprintf (msg, sizeof (msg),"HTTP/0.9 403 FORBIDDEN\nContent-Type: text/html\nContent-Length: %d\n\n%s", strlen(body), body);
                result=send(new_s, msg, strlen(msg), MSG_NOSIGNAL );
            }
            
            fclose(sendFile);
            
        }
        else{
            snprintf (body, sizeof (body),"<html>\n<body>\n<h1>ERROR 404</h1>\n</body>\n</html>\n");
            snprintf (msg, sizeof (msg),"HTTP/0.9 404 NOT FOUND\nContent-Type: text/html\nContent-Length: %d\n\n%s", strlen(body), body);
            result=send(new_s, msg, strlen(msg), MSG_NOSIGNAL );
        }
        if(result<0){
            printf("Peer was disconnected (errno: '%s')\n", strerror(errno));
            break;
        }
        memset(requestPath,0,strlen(requestPath));
        
    }
}

/*
    Auxilary function used to skip first word in string

    @param message: pointer to string in which word should be skipped
    @return
*/
void skipWord(char **message) {
	while (isalnum(**message)) ++ * message;
}

/*
    Auxilary function used to skip white spaces from the begining of a string

    @param message: pointer to string in which white spaces should be skipped
    @return
*/
void skipWhiteSpaces(char** message) {
	while (isspace(**message))++ * message;
}

/*
    Auxilary function used to skip chars until given pattern occurs

    @param message: pointer to string in which chars should be skipped
    @param pattern: string pattern
    @return
*/
void skipUntilPattern(char** message, const char* pattern) {
	int patternlen = strlen(pattern);
	while (strncmp(*message, pattern, patternlen) != 0)++* message;
}

/*
    Used to parse whole HTTP request

    @param message: pointer to HTTP request
    @param result: result data structure
    @return 
*/
void parseRequest(char** message, char* result) {
    skipWord(message);
    skipWhiteSpaces(message);
    char* beginning = *message;
    skipUntilPattern(message, " ");
    char* end = *message;
    memcpy(result, beginning, end - beginning);
}

/*
    Main function handles accepting the connection and adding to buffer

    @param argc: number of parameters
    @param argv: parameters from console
    @return 
*/
int main(int argc, char** argv){
    int s, new_s;
    unsigned char mip_str[INET_ADDRSTRLEN], message[MAX_BUF], body[MAX_BUF], msg[MAX_BUF];
    struct addrinfo h, *r;
    memset(&h, 0, sizeof(struct addrinfo));

    h.ai_family=PF_INET; 
    h.ai_socktype=SOCK_STREAM; 
    h.ai_flags=AI_PASSIVE;

    BNDBUF* buffer = bb_init(atoi(argv[4]));
    if(buffer == NULL){
        printf("ERROR in creating buffer\n");
        freeaddrinfo(r);
        close(s);
        exit(-4);
    }
    int threadNr = atoi(argv[3]);
    pthread_t threads[threadNr];

    snprintf(www_path, sizeof(www_path), "%s", argv[1]);

    for(int i = 0; i < threadNr; i++){
        pthread_create(&threads[i], NULL, thread_function, buffer);
    }


    if(getaddrinfo(NULL, argv[2], &h, &r) != 0){
        printf("ERROR: %s (%s:%d)\n", strerror(errno), __FILE__, __LINE__);
        bb_del(buffer);
        exit(-4);
    }
    if((s=socket(r->ai_family, r->ai_socktype, r->ai_protocol)) == -1){
        printf("ERROR in socket creation: %s (%s:%d)\n", strerror(errno), __FILE__, __LINE__);
        bb_del(buffer);
        freeaddrinfo(r);
        exit(-4);
    }
    if(bind(s, r->ai_addr, r->ai_addrlen) != 0){
        printf("ERROR in binding: %s (%s:%d)\n", strerror(errno), __FILE__, __LINE__);
        bb_del(buffer);
        freeaddrinfo(r);
        close(s);
        exit(-4);
    }

    for(;;){
        if(listen(s, 1) != 0){
            printf("ERROR in listening: %s (%s:%d)\n", strerror(errno), __FILE__, __LINE__);
        }
        struct sockaddr_in their_addr;
        socklen_t addr_size=sizeof(their_addr);
        if((new_s=accept(s, (struct sockaddr *)&their_addr, &addr_size))==-1){
            printf("ERROR accepting connection: %s (%s:%d)\n", strerror(errno), __FILE__, __LINE__);
        }
        if(inet_ntop(AF_INET, &(their_addr.sin_addr), mip_str, INET_ADDRSTRLEN)!=NULL){
            printf("IP: %s, new sock desc.: %d\n", mip_str, new_s);
        }
        bb_add(buffer, new_s);
    }
    bb_del(buffer);
    freeaddrinfo(r);
    close(s);
}