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

#define TIMEOUT 2
#define BUFFSIZE 100
#define MAXPENDING 10
#define port 12430
float pid =0.2;
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
    double r = (double) rand() / (RAND_MAX);
    if (r < pid) {
        //printf("Discarded packet with probability %f\n", r);
        discard = true;
    } else {
        discard = false;
    }   
}

int main(){
    int serverSocket = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket < 0) { printf ("Error while server socketcreation"); exit (0); }
    printf ("Server Socket Created\n"); 
    struct sockaddr_in serverAddress, clientAddress1,clientAddress2;
    

    memset (&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    printf ("Server address assigned\n");
    int temp = bind(serverSocket, (struct sockaddr*) &serverAddress,
    sizeof(serverAddress));
    if (temp < 0)
    { 
        printf ("Error while binding, try to change the port number\n");
        exit (0);
    }
    printf ("Binding successful\n");
    int temp1 = listen(serverSocket, MAXPENDING);
    if (temp1 < 0)
    { 
        printf ("Error in listen");
        exit (0);
    }
    printf ("Now Listening\n");
    FILE* file;
    file = fopen("list.txt", "wb");
    if (file == NULL) {
        printf("Error: could not open file for writing.\n");
        return 1;
    }
    int state=0;
    int offset_1=0;
    int offset_2=0;
    PACKET ack_p;
    PACKET prev_ack1;
    PACKET prev_ack2;
    prev_ack1.type=0;
    prev_ack1.seq=-1;
    prev_ack1.client=1;
    prev_ack2.type=0;
    prev_ack2.seq=-1;
    prev_ack2.client=2;
    int clientLength1 = sizeof(clientAddress1);
    int clientLength2 = sizeof(clientAddress2);
    int clientSocket1 = accept (serverSocket, (struct sockaddr*)&clientAddress1, &clientLength1);
    int clientSocket2 = accept (serverSocket, (struct sockaddr*)&clientAddress2, &clientLength2);
    if (clientLength1 < 0) {printf ("Error in client socket"); exit(0);}
    if (clientLength2 < 0) {printf ("Error in client socket"); exit(0);}
    while(1){
        switch(state){
            case 0:{
                discardPacket();
                
               // printf ("Handling Client %s\n", inet_ntoa(clientAddress1.sin_addr));
                PACKET p;
                int temp2 = recv(clientSocket1, &p, sizeof(p), 0);
                if (temp2 <= 0)
                { 
                    //sleep(5);
                    printf ("All Packets transferred\n");
                    exit (0);
                }
            
                if(p.client==1 && p.seq==offset_1){
                    if(discard==false){
                        printf("RCVD PKT: Seq. No. = %d, Size = %d Bytes\n",p.seq,p.size);
                        fwrite(p.data,sizeof(char),p.size,file);
                        fwrite(",",sizeof(char),1,file);
                        offset_1+=p.size;
                        ack_p.seq=p.seq;
                        ack_p.type=0;
                        ack_p.client=1;
                        prev_ack1=ack_p;
                        int byteSent=send(clientSocket1, &ack_p, sizeof(ack_p), 0);
                        if (byteSent <0) {
                            die("send() failed");
                        }
                        printf("SENT ACK: Seq. No. = %d \n",ack_p.seq);
                        state=1;
                    
                    }
                    else if(discard==true){
                        
                        printf("DROP PKT: Seq. No. = %d\n",p.seq);
                        int byteSent=send(clientSocket1, &prev_ack1, sizeof(prev_ack1), 0);
                        if (byteSent <0) {
                            die("send() failed");
                        }
                    }
                    else{
                        int byteSent=send(clientSocket1, &prev_ack1, sizeof(prev_ack1), 0);
                        if (byteSent <0) {
                            die("send() failed");
                        }
                    }
                }
                

                break;

            }
            case 1:{
                discardPacket();
                
                //printf ("Handling Client %s\n", inet_ntoa(clientAddress2.sin_addr));
                PACKET p;
                int temp2 = recv(clientSocket2, &p, sizeof(p), 0);
                if (temp2 <= 0)
                { 
                    //sleep(5);
                    printf ("All Packets transferred\n");
                    exit (0);
                }
                if(p.client==2 && p.seq==offset_2){
                    if(discard==false){
                        printf("RCVD PKT: Seq. No. = %d, Size = %d Bytes\n",p.seq,p.size);
                        fwrite(p.data,sizeof(char),p.size,file);
                        fwrite(",",sizeof(char),1,file);
                        offset_2+=p.size;
                        ack_p.seq=p.seq;
                        ack_p.type=0;
                        ack_p.client=2;
                        prev_ack2=ack_p;
                        int byteSent=send(clientSocket2, &ack_p, sizeof(ack_p), 0);
                        if (byteSent <0) {
                            die("send() failed");
                        }
                        printf("SENT ACK: Seq. No. = %d \n",ack_p.seq);
                        state=2;
                    
                    }
                    else if (discard==true){
                        
                        printf("DROP PKT: Seq. No. = %d\n",p.seq);
                        
                        int byteSent=send(clientSocket2, &prev_ack2, sizeof(prev_ack2), 0);
                        if (byteSent <0) {
                            die("send() failed");
                        }
                    }
                    else{
                        int byteSent=send(clientSocket2, &prev_ack2, sizeof(prev_ack2), 0);
                        if (byteSent <0) {
                            die("send() failed");
                        }
                    }
                }
                break;
            }
            case 2:{
                discardPacket();
               
                //printf ("Handling Client %s\n", inet_ntoa(clientAddress1.sin_addr));
                PACKET p;
                int temp2 = recv(clientSocket1, &p, sizeof(p), 0);
                if (temp2 <= 0)
                { 
                    //sleep(5);
                    printf ("All Packets transferred\n");
                    exit (0);
                }
                //printf("offset = %d sequence = %d\n",offset_1,p.seq);
                if(p.client==1 && p.seq==offset_1) {
                    if(discard==false){
                    
                        printf("RCVD PKT: Seq. No. = %d, Size = %d Bytes\n",p.seq,p.size);
                        fwrite(p.data,sizeof(char),p.size,file);
                        fwrite(",",sizeof(char),1,file);
                        offset_1+=p.size;
                        ack_p.seq=p.seq;
                        ack_p.type=0;
                        ack_p.client=1;
                        prev_ack1=ack_p;
                        int byteSent=send(clientSocket1, &ack_p, sizeof(ack_p), 0);
                        if (byteSent <0) {
                            die("send() failed");
                        }
                        printf("SENT ACK: Seq. No. = %d \n",ack_p.seq);
                        state=3;
                    
                    }
                    
                    else if(discard==true){
                        
                        printf("DROP PKT: Seq. No. = %d\n",p.seq);
                        
                        int byteSent=send(clientSocket1, &prev_ack1, sizeof(prev_ack1), 0);
                        if (byteSent <0) {
                            die("send() failed");
                        }
                    }
                    else{
                        int byteSent=send(clientSocket1, &prev_ack1, sizeof(prev_ack1), 0);
                        if (byteSent <0) {
                            die("send() failed");
                        }
                    }
                }
                break;
            }
            case 3:{
                discardPacket();
                
                //printf ("Handling Client %s\n", inet_ntoa(clientAddress2.sin_addr));
                PACKET p;
                int temp2 = recv(clientSocket2, &p, sizeof(p), 0);
                if (temp2 <= 0)
                { 
                    //sleep(5);
                    printf ("All Packets transferred\n");
                    exit (0);
                }
                if(p.client==2 && p.seq==offset_2){
                    if(discard==false){
                        printf("RCVD PKT: Seq. No. = %d, Size = %d Bytes\n",p.seq,p.size);
                        fwrite(p.data,sizeof(char),p.size,file);
                        fwrite(",",sizeof(char),1,file);
                        offset_2+=p.size;
                        ack_p.seq=p.seq;
                        ack_p.type=0;
                        ack_p.client=2;
                        prev_ack2=ack_p;
                        int byteSent=send(clientSocket2, &ack_p, sizeof(ack_p), 0);
                        if (byteSent <0) {
                            die("send() failed");
                        }
                        printf("SENT ACK: Seq. No. = %d \n",ack_p.seq);
                        state=0;
                    
                    }
                    else if(discard==true){
                        
                        printf("DROP PKT: Seq. No. = %d\n",p.seq);
                        
                        int byteSent=send(clientSocket2, &prev_ack2, sizeof(prev_ack2), 0);
                        if (byteSent <0) {
                            die("send() failed");
                        }
                    }
                    else{
                        int byteSent=send(clientSocket2, &prev_ack2, sizeof(prev_ack2), 0);
                        if (byteSent <0) {
                            die("send() failed");
                        }
                    }
                }
                break;
            }
        }

    }
    close(serverSocket);
    fclose(file);
}