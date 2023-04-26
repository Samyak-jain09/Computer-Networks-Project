#include <stdio.h>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <stdlib.h> 
#include <string.h>
#include <unistd.h> 
#include <stdbool.h>
#include <time.h>
#include <sys/types.h>
#include <sys/select.h>
#define pid 0.1
#define TIMEOUT 2
#define BUFFSIZE 100
#define port 12392

void die(char *s){
    perror(s);
    exit(1);
}
typedef struct packet{
    int size;
    int seq;
    int type;
    int client; //to know which client is sending the packet
    char data[BUFFSIZE];
}PACKET;
bool discard=false;
void discardPacket(){
    srand(time(NULL)); // initialize random number generator
    if ((double) rand() / RAND_MAX <= pid) {
        discard = true;
    } else {
        discard = false;
    }   
}

int final_offset=0;
void calculate_last_offset(){
    FILE *fp1;
    char *token;
    fp1 = fopen("id.txt", "r");
    if (fp1 == NULL) {
        printf("Error opening file.\n");
        return ;
    }
    char line[10000];
    int offset = 0;
    while (fgets(line, sizeof(line), fp1)) {
        // remove trailing newline character
        line[strcspn(line, "\n")] = 0;
        
        // check if line ends with full stop
        if (line[strlen(line)-1] == '.') {
            line[strlen(line)-1] = '\0';
        }
        
        // tokenize the line based on comma delimiter
        token = strtok(line, ",");
        while (token != NULL) {
            int len = strlen(token);
            
            
            final_offset=offset;
            offset += len;
            
            // skip the comma, if present
            if (offset < strlen(line) && line[offset] == ',') {
                offset++;
            }
            
            token = strtok(NULL, ",");
        }
    }
    fclose(fp1);
}

int main(){
    int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) { printf ("Error in opening a socket"); exit (0);}
    printf ("Client Socket Created\n");
    struct sockaddr_in serverAddr;
    memset (&serverAddr,0,sizeof(serverAddr)); 
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.1.1"); //Specify server's IP address here
    printf ("Address assigned\n");
    int c = connect (sock, (struct sockaddr*) &serverAddr , sizeof(serverAddr));
  
    if (c < 0)
    { printf ("Error while establishing connection");
    exit (0);
    }
    printf ("Connection Established\n");
    FILE *fp;
    char filename[] = "id.txt";
    char line[10000];
    char *token;
    PACKET p;
    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Error opening file.\n");
        return 1;
    }
    int offset=0;
    int state=0;
    PACKET prev;
    calculate_last_offset();
    while (fgets(line, sizeof(line), fp)){
        line[strcspn(line, "\n")] = 0;
        
        // check if line ends with full stop
        if (line[strlen(line)-1] == '.') {
            line[strlen(line)-1] = '\0';
        }
        
        // tokenize the line based on comma delimiter
        token = strtok(line, ",");
        
        while (1) {
            int len=0;
            if(token!=NULL)
                len = strlen(token);
            
           // printf("Token: %s Offset: %d\n", token, offset);

            switch(state){
            case 0: {
                p.size=strlen(token);
                p.seq=offset;
                p.type=1;
                p.client=2;
                strcpy(p.data,token);
                prev=p;
                int bytesSent = send (sock, &p, sizeof(p), 0);
                if(bytesSent<0)
                    die("send() failed");
                else {
                    offset += len;
            
                    // skip the comma, if present
                    if (offset < strlen(line) && line[offset] == ',') {
                        offset++;
                    }
                    
                    token = strtok(NULL, ",");
                   printf("SENT PKT: Seq. No. = %d, Size = %d Bytes \n",p.seq,p.size);
                }
                state=1;
                break;
            }
            case 1: {
                discardPacket();

                fd_set rcvSet;
                int n;
                
                struct timeval tv;

                FD_ZERO(&rcvSet);
                FD_SET(sock, &rcvSet);

                tv.tv_sec = TIMEOUT;
                tv.tv_usec = 0;

                if((n = select(sock + 1, &rcvSet, NULL, NULL, &tv) ) < 0){
                    die("error on select");
                }
                if(n==0){
                    int bytesSent = send (sock, &prev, sizeof(prev), 0);
                    if(bytesSent<0)
                        die("send() failed");
                    else 
                        printf("RE-TRANSMIT PKT: Seq. No. = %d, Size = %d Bytes \n",prev.seq,prev.size);
                    break;
                }

                int bytesRecieved=recv(sock,&p,sizeof(p),0);
                if(bytesRecieved<0)
                    die("recv() failed");
                if(p.type==0 && p.seq==prev.seq && discard==false && p.client==2){
                    printf("RCVD ACK: Seq. No. = %d\n",p.seq);
                    if(p.seq==final_offset){
                        printf("All packets transferred\n");
                        exit(0);
                    }
                    state=2;
                }
                break;
                }
            case 2: {
                p.size=strlen(token);
                p.seq=offset;
                p.type=1;
                p.client=2;
                strcpy(p.data,token);
                prev=p;
                int bytesSent = send (sock, &p, sizeof(p), 0);
                if(bytesSent<0)
                    die("send() failed");
                else{
                    offset += len;
            
                    // skip the comma, if present
                    if (offset < strlen(line) && line[offset] == ',') {
                        offset++;
                    }
                    
                    token = strtok(NULL, ",");
                    printf("SENT PKT: Seq. No. = %d, Size = %d Bytes \n",p.seq,p.size);
                }
                state=3;
                break;
            }
            case 3:{
                discardPacket();

                fd_set rcvSet;
                int n;
                
                struct timeval tv;

                FD_ZERO(&rcvSet);
                FD_SET(sock, &rcvSet);

                tv.tv_sec = TIMEOUT;
                tv.tv_usec = 0;

                if((n = select(sock + 1, &rcvSet, NULL, NULL, &tv) ) < 0){
                    die("error on select");
                }
                if(n==0){
                    int bytesSent = send (sock, &prev, sizeof(prev), 0);
                    if(bytesSent<0)
                        die("send() failed");
                    else 
                        printf("RE-TRANSMIT PKT: Seq. No. = %d, Size = %d Bytes \n",prev.seq,prev.size);
                    break;
                }

                int bytesRecieved=recv(sock,&p,sizeof(p),0);
                if(bytesRecieved<0)
                    die("recv() failed");
                if(p.type==0 && p.seq==prev.seq && discard==false && p.client==2){
                    printf("RCVD ACK: Seq. No. = %d\n",p.seq);
                    if(p.seq==final_offset){
                        printf("All packets transferred\n");
                        exit(0);
                    }
                    state=0;
                }
                break;
            }
                
        }



            
        }

    }
    int bytesRecieved=recv(sock,&p,sizeof(p),0);
    if(bytesRecieved<0)
        die("recv() failed");
    if(p.type==0 && p.seq==prev.seq && discard==false && p.client==2){
        printf("RCVD ACK: Seq. No. = %d\n",p.seq);
        state=0;
    }
    fclose(fp);

}