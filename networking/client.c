#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/select.h>
#include <time.h>
#include <errno.h>
#include "sham.h"



FILE *logm_file = NULL;

void logm(char *msg) {
    if (logm_file==NULL){
        return;
    }
    struct timeval time;

    gettimeofday(&time, NULL);
    
    char time_buffer[30];

    struct tm *local = localtime(&time.tv_sec);

    strftime(time_buffer, 30, "%Y-%m-%d %H:%M:%S", local);
    
    fprintf(logm_file, "[%s.%06ld] [LOG] %s\n", time_buffer, time.tv_usec, msg);
    fflush(logm_file);
}

int send_packet(int sockfd, struct sockaddr_in *server_addr, struct sham_header *header, char *data, int len) {
    
    socklen_t addr_len = sizeof(*server_addr);
    char buffer[PAYLOAD_SIZE + sizeof(struct sham_header)];  
    memcpy(buffer, header, sizeof(struct sham_header));
    
    if (data !=NULL && len>0){
        memcpy(buffer + sizeof(struct sham_header), data, len);
    }
    
    int sent = sendto(sockfd, buffer, sizeof(struct sham_header) + len, 0,(struct sockaddr *)server_addr, addr_len);
    
    return sent;
}


int recieve_packet(int sockfd, struct sockaddr_in *server_addr, struct sham_header *header, char *data, int *data_len) {
    
    socklen_t addr_len = sizeof(*server_addr);
    char buffer[PAYLOAD_SIZE + sizeof(struct sham_header)];

    int n = recvfrom(sockfd, buffer, sizeof(buffer), 0,(struct sockaddr *)server_addr, &addr_len);
    
    if (n < (int)sizeof(struct sham_header)){ 
        return -1; 
    }
    
    memcpy(header, buffer, sizeof(struct sham_header));
    if ( data!=NULL && n > (int)sizeof(struct sham_header)) {
        *data_len = n - sizeof(struct sham_header);
        memcpy(data, buffer + sizeof(struct sham_header), *data_len);
    }
    else {
        *data_len = 0;
    }
    if (*data_len > 0) {
    // For debug only: print hex or length
        printf("server: %s\n", data);
    }
    return n;
}

int connection_handshake(int *sockfd,struct sockaddr_in *server_addr){
    int data_len,seq_no=1000;
    while (1) {
        struct sham_header header;
        char buffer[PAYLOAD_SIZE];
        char logmmsg[30];

         // Send SYN-ACK
        struct sham_header syn;
        syn.seq_num = seq_no; // server initial seq
        syn.flags = SYN ;
        syn.window_size = SLIDING_WIN * PAYLOAD_SIZE;
        send_packet(*sockfd, server_addr, &syn, NULL, 0);
        sprintf(logmmsg, "SND SYN SEQ=%u", syn.seq_num);
        logm(logmmsg);
        // Wait for final ACK
        if (recieve_packet(*sockfd, server_addr, &header, buffer, &data_len) < 0) continue;
        if (header.flags & (ACK|SYN)) {
            sprintf(logmmsg, "RCV SYN-ACK SEQ=%u ACK=%u",header.seq_num,header.ack_num);
            logm(logmmsg);
            
        }
        seq_no+=1;
        struct sham_header ack;
       // syn.seq_num = seq_no; 
        ack.ack_num = header.seq_num + 1;
        ack.flags = ACK ;
        ack.window_size = SLIDING_WIN * PAYLOAD_SIZE;
        send_packet(*sockfd, server_addr, &ack, NULL, 0);
        sprintf(logmmsg, "SND ACK ACK=%u", ack.ack_num);
        logm(logmmsg);
        printf("Handshake complete with server.\n");
        break;
    }
    return seq_no;
}

void finish_rcv(struct sham_header header,int sockfd,struct sockaddr_in server_addr,int seq_no){
    
    char logmmsg[30];
    char  buffer[PAYLOAD_SIZE];
    int data_len;
    sprintf(logmmsg, "RCV FIN SEQ=%u", header.seq_num);
    logm(logmmsg);
    struct sham_header fin_ack;
    //fin_ack.seq_num = seq_no;
    fin_ack.ack_num = header.seq_num + 1;
    fin_ack.flags = ACK;
    fin_ack.window_size =SLIDING_WIN * PAYLOAD_SIZE;
    send_packet(sockfd, &server_addr, &fin_ack, NULL, 0);
    sprintf(logmmsg, "SND ACK FOR FIN");
    logm(logmmsg);
    struct sham_header fin_syn;
    fin_syn.flags=FIN;
    fin_syn.seq_num = seq_no;
    fin_syn.window_size =SLIDING_WIN * PAYLOAD_SIZE;
    send_packet(sockfd, &server_addr, &fin_syn, NULL, 0);
    sprintf(logmmsg, "SND FIN SEQ=%u",seq_no);
    logm(logmmsg);
    seq_no+=1;

    while(recieve_packet(sockfd, &server_addr, &header, buffer, &data_len)<0){

    }
    if(header.flags==ACK && header.ack_num==seq_no){
        sprintf(logmmsg, "RCV ACK=%u",seq_no);
        logm(logmmsg);
    }

}

void finish_snd(int sockfd,struct sockaddr_in server_addr,int seq_no){
    char logmmsg[30];
    char  buffer[PAYLOAD_SIZE];
    int data_len;
    
    struct sham_header fin;
    //fin_ack.seq_num = seq_no;
    fin.seq_num =seq_no;
    fin.flags = FIN;
    fin.window_size =SLIDING_WIN * PAYLOAD_SIZE;
    send_packet(sockfd, &server_addr, &fin, NULL, 0);
    sprintf(logmmsg, "SND SEQ=%u FOR FIN",seq_no);
    logm(logmmsg);
    struct sham_header header;
    seq_no+=1;
    while(recieve_packet(sockfd, &server_addr, &header, buffer, &data_len)<0){

    }
    if(header.flags & ACK && header.ack_num == seq_no) {
        sprintf(logmmsg, "RCV ACK=%u for FIN",header.ack_num);
        logm(logmmsg);
    }
    while(recieve_packet(sockfd, &server_addr, &header, buffer, &data_len)<0){

    }
    if(header.flags && FIN){
        sprintf(logmmsg, "RCV FIN SYN=%u",header.seq_num);
        logm(logmmsg);
    }
    struct sham_header fin_ack;
    fin_ack.flags=FIN;
    fin_ack.ack_num =header.seq_num +1;
    fin_ack.window_size =SLIDING_WIN * PAYLOAD_SIZE;
    send_packet(sockfd, &server_addr, &fin_ack, NULL, 0);
    sprintf(logmmsg, "SND FIN ACK=%u",header.seq_num +1);
    logm(logmmsg);
    seq_no+=1;
}


int main(int argc,char* argv[]) {
    int sockfd;
    int mode=1;
    if(argc>3 && strcmp(argv[3],"--chat")==0){
        printf("chat\n");
        mode=0;
    }
    struct sockaddr_in server_addr;
    int SERVER_PORT;
    char *server_ip;
    if(argc>2){
        SERVER_PORT=atoi(argv[2]);
        server_ip=argv[1];
    }
    logm_file = fopen("client_log.txt", "w");
    
    if (logm_file==NULL) {
        perror("logm file failed");
        exit(EXIT_FAILURE);
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }
    
    int data_len;
    char buffer[PAYLOAD_SIZE];
    struct sham_header header;

    int seq_no=connection_handshake(&sockfd,&server_addr);

    if(mode){
        char data[PAYLOAD_SIZE];
        if(argc>4){
            char logmmsg[200];
            char*fname=argv[4];
            FILE* fdq=fopen(argv[3],"r");
            
            int len=strlen(fname);
            struct sham_header syn_hdr;
            syn_hdr.seq_num = seq_no;
            syn_hdr.flags = 0;
            syn_hdr.window_size = SLIDING_WIN * PAYLOAD_SIZE;
            send_packet(sockfd, &server_addr, &syn_hdr,fname, len);
            sprintf(logmmsg, "SND SYN SEQ=%u WIN=%d", syn_hdr.seq_num, syn_hdr.window_size);
            logm(logmmsg);
            seq_no+=len;
            while(1){
                while(recieve_packet(sockfd, &server_addr, &header, buffer, &data_len)<0){
    
                }
                if(header.ack_num==seq_no){
                    sprintf(logmmsg, "RCV ACK ACK=%u", header.ack_num);
                    logm(logmmsg);
                    break;
                }
            }
            size_t len_d;
            while( (len_d=fread(data,1,PAYLOAD_SIZE,fdq))>0){                
                
                struct sham_header syn_hdr;
                syn_hdr.seq_num = seq_no;
                //syn_hdr.flags = SYN;
                syn_hdr.window_size = SLIDING_WIN * PAYLOAD_SIZE;
                send_packet(sockfd, &server_addr, &syn_hdr,data, len_d);
                sprintf(logmmsg, "SND SYN SEQ=%u WIN=%d", syn_hdr.seq_num, syn_hdr.window_size);
                logm(logmmsg);
                seq_no+=len_d;
                while(1){
                    while(recieve_packet(sockfd, &server_addr, &header, buffer, &data_len)<0){
        
                    }
                    if(header.ack_num>=seq_no){
                        sprintf(logmmsg, "RCV ACK ACK=%u", header.ack_num);
                        logm(logmmsg);
                        break;
                    }
                }
            }
            fclose(fdq);
            
        }
        
        finish_snd(sockfd,server_addr,seq_no);
        close(sockfd);
        return 0;
  
    }

    // Example: receive data packets
    fd_set rd;

    while (1) {
        FD_ZERO(&rd);

        FD_SET(0,&rd);
        FD_SET(sockfd,&rd);

        int maxfd=(sockfd > 0 ? sockfd : 0)+1;

        int activity=select(maxfd,&rd,NULL,NULL,NULL);

        if( FD_ISSET(sockfd,&rd)){
            int n = recieve_packet(sockfd, &server_addr, &header, buffer, &data_len);
            if (n < 0){
                continue;
            }
            if (header.flags & FIN) {
                finish_rcv(header,sockfd,server_addr,seq_no);
                break;
            }
            char logmmsg[200];
            sprintf(logmmsg, "RCV DATA SEQ=%u LEN=%d", header.seq_num, data_len);
            logm(logmmsg);
            // Send ACK (cumulative)
            if(!(header.flags & ACK)){
                struct sham_header ack_hdr;
                ack_hdr.seq_num = seq_no;
                ack_hdr.ack_num = header.seq_num + data_len;
                ack_hdr.flags = ACK;
                ack_hdr.window_size = SLIDING_WIN * PAYLOAD_SIZE;
                send_packet(sockfd, &server_addr, &ack_hdr, NULL, 0);
                sprintf(logmmsg, "SND ACK=%u WIN=%d", ack_hdr.ack_num, ack_hdr.window_size);
                logm(logmmsg);
            }

        }

        else if( FD_ISSET(0,&rd)){
            char logmmsg[200];
            char* data=NULL;
            size_t size=PAYLOAD_SIZE;
            int len=getline(&data,&size,stdin);
            if(data[len-1]=='\n'){
                data[len-1]='\0';
            }
            
            if(strcmp(data,"/quit")==0){
                finish_snd(sockfd,server_addr,seq_no);
                break;
            }
            if(len>0){
            
                struct sham_header syn_hdr;
                syn_hdr.seq_num = seq_no;
                //syn_hdr.ack_num = header.seq_num + data_len;
                syn_hdr.flags = 0;
                syn_hdr.window_size = SLIDING_WIN * PAYLOAD_SIZE;
                send_packet(sockfd, &server_addr, &syn_hdr,data, len);
                
                sprintf(logmmsg, "SND SYN SEQ=%u WIN=%d", syn_hdr.seq_num, syn_hdr.window_size);
                logm(logmmsg);
                                
            }
        }
    }



    close(sockfd);
    fclose(logm_file);
    return 0;
}
