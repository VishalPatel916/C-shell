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
#include <openssl/evp.h>
#include <openssl/md5.h>


int mode=1;
FILE *logm_file = NULL;
float LOSS_PROBABILITY=0;

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

int send_packet(int sockfd, struct sockaddr_in *client_addr, struct sham_header *header, char *data, int len) {
    
    socklen_t addr_len = sizeof(*client_addr);
    char buffer[PAYLOAD_SIZE + sizeof(struct sham_header)];  
    memcpy(buffer, header, sizeof(struct sham_header));
    
    if (data !=NULL && len>0){
        memcpy(buffer + sizeof(struct sham_header), data, len);
    }
    
    int sent = sendto(sockfd, buffer, sizeof(struct sham_header) + len, 0,(struct sockaddr *)client_addr, addr_len);
    
    return sent;
}


int recieve_packet(int sockfd, struct sockaddr_in *client_addr, struct sham_header *header, char *data, int *data_len) {
    
    socklen_t addr_len = sizeof(*client_addr);
    char buffer[PAYLOAD_SIZE + sizeof(struct sham_header)];

    int n = recvfrom(sockfd, buffer, sizeof(buffer), 0,(struct sockaddr *)client_addr, &addr_len);
    
    if (n < (int)sizeof(struct sham_header)){ 
        return -1; 
    }
    
    memcpy(header, buffer, sizeof(struct sham_header));
    
    if ((rand() % 100) < LOSS_PROBABILITY) {
        char logmmsg[200];
        sprintf(logmmsg, "DROP DATA SEQ=%u", header->seq_num);
        logm(logmmsg);
        return -1; 
    }

    if ( data!=NULL && n > (int)sizeof(struct sham_header)) {
        *data_len = n - sizeof(struct sham_header);
        memcpy(data, buffer + sizeof(struct sham_header), *data_len);
    }
    else {
        *data_len = 0;
    }
    if (*data_len > 0) {
    // For debug only: print hex or length
        if(mode==0){
            printf("client: %s\n", data);
        }
    }
    return n;
}

int connection_handshake(int *sockfd,struct sockaddr_in *client_addr){
    int data_len,seq_no=500;

    int f=1,retransmission=0;int rcvsyn=0;
    while(1){
        struct sham_header header;
        char buffer[PAYLOAD_SIZE];

        // Wait for initial SYN

        if (recieve_packet(*sockfd, client_addr, &header, buffer, &data_len) < 0) continue;
        
        rcvsyn=header.seq_num;
        char logmmsg[100];
        sprintf(logmmsg, "RCV SYN SEQ=%u", header.seq_num);
        logm(logmmsg);
       
        while(f){
            //if (header.flags & SYN) {
                
                
                // Send SYN-ACK
                
                struct sham_header synack;
                synack.seq_num = seq_no; // server initial seq
                synack.ack_num = header.seq_num + 1;
                synack.flags = SYN | ACK;
                synack.window_size = SLIDING_WIN * PAYLOAD_SIZE;
                
                send_packet(*sockfd, client_addr, &synack, NULL, 0);
                
                if(retransmission){
                    sprintf(logmmsg, "RTX SYN-ACK SEQ=%u ACK=%u", synack.seq_num, synack.ack_num);
                    logm(logmmsg);
                }
                else{
                    sprintf(logmmsg, "SND SYN-ACK SEQ=%u ACK=%u", synack.seq_num, synack.ack_num);
                    logm(logmmsg);
                }
    
                // Wait for final ACK
                struct sham_header header1;
                int n=recieve_packet(*sockfd, client_addr, &header1, buffer, &data_len);
                if(n<0){
                    retransmission=1;
                    sprintf(logmmsg, "TIMEOUT ACK FOR SYN");
                    logm(logmmsg);
                    continue;
                }
                if(header1.seq_num==rcvsyn){
                    continue;
                }
                if (header1.flags & ACK) {
                    sprintf(logmmsg, "RCV ACK FOR SYN");
                    logm(logmmsg);
                    printf("Handshake complete with client.\n");
                }
                else{
                    retransmission=1;
                    continue;
                }
                
           // }
            seq_no+=1;
            f=0;
        }
        break;
    }
    return seq_no;
}

void finish_rcv(struct sham_header header, int sockfd, struct sockaddr_in server_addr, int seq_no) {
    char logmmsg[200];
    char buffer[PAYLOAD_SIZE];
    int data_len;
    
    sprintf(logmmsg, "RCV FIN SEQ=%u", header.seq_num);
    logm(logmmsg);

    while (1) {
        struct sham_header fin_ack;
        fin_ack.ack_num = header.seq_num + 1;
        fin_ack.flags = ACK;
        fin_ack.seq_num = seq_no;
        fin_ack.window_size = SLIDING_WIN * PAYLOAD_SIZE;
        send_packet(sockfd, &server_addr, &fin_ack, NULL, 0);
        sprintf(logmmsg, "SND ACK=%u for FIN", fin_ack.ack_num);
        logm(logmmsg);

        struct timeval tv = {0, 300000}; 
        fd_set rd;
        FD_ZERO(&rd);
        FD_SET(sockfd, &rd);

        int ret = select(sockfd + 1, &rd, NULL, NULL, &tv);
        if (ret > 0 && FD_ISSET(sockfd, &rd)) {
            struct sham_header temp_header;
            
            recieve_packet(sockfd, &server_addr, &temp_header, buffer, &data_len);
            if (temp_header.flags & FIN) {
                sprintf(logmmsg, "RCV DUP FIN SEQ=%u, resending ACK", temp_header.seq_num);
                logm(logmmsg);
                continue; 
            }
        } else {
            
            break;
        }
    }

    
    int f = 1, retransmission = 0;
    while (f) {
        struct sham_header fin_syn;
        fin_syn.flags = FIN;
        fin_syn.seq_num = seq_no;
        fin_syn.window_size = SLIDING_WIN * PAYLOAD_SIZE;

        send_packet(sockfd, &server_addr, &fin_syn, NULL, 0);
        if (retransmission) {
            sprintf(logmmsg, "RTX FIN SEQ=%u", seq_no);
            logm(logmmsg);
        } else {
            sprintf(logmmsg, "SND FIN SEQ=%u", seq_no);
            logm(logmmsg);
        }

        int n = recieve_packet(sockfd, &server_addr, &header, buffer, &data_len);
        if (n < 0) {
            retransmission = 1;
            sprintf(logmmsg, "TIMEOUT for final ACK of our FIN SEQ=%u", fin_syn.seq_num);
            logm(logmmsg);
            continue;
        }

        if ((header.flags & ACK) && header.ack_num == seq_no + 1) {
            sprintf(logmmsg, "RCV FINAL ACK=%u", header.ack_num);
            logm(logmmsg);
            f = 0; 
        } else {
            retransmission = 1;
            continue;
        }
    }
   
}

void finish_snd(int sockfd, struct sockaddr_in server_addr, int seq_no) {
    char logmmsg[200];
    char buffer[PAYLOAD_SIZE];
    int data_len;
    struct sham_header header;
    int f = 1, retransmission = 0;

    while (f) {
        struct sham_header fin;
        fin.seq_num = seq_no;
        fin.flags = FIN;
        fin.window_size = SLIDING_WIN * PAYLOAD_SIZE;

        send_packet(sockfd, &server_addr, &fin, NULL, 0);
        if (retransmission) {
            sprintf(logmmsg, "RTX FIN SEQ=%u", seq_no);
            logm(logmmsg);
        } else {
            sprintf(logmmsg, "SND FIN SEQ=%u", seq_no);
            logm(logmmsg);
        }

        int n = recieve_packet(sockfd, &server_addr, &header, buffer, &data_len);
        if (n < 0) {
            retransmission = 1;
            sprintf(logmmsg, "TIMEOUT SEQ=%u for FIN's ACK", seq_no);
            logm(logmmsg);
            continue;
        }
        if ((header.flags & ACK) && header.ack_num == seq_no + 1) {
            sprintf(logmmsg, "RCV ACK=%u for our FIN", header.ack_num);
            logm(logmmsg);
            f = 0; 
        } else {
            retransmission = 1; 
            continue;
        }
    }

    seq_no += 1;

    while (1) {
       
        int n = recieve_packet(sockfd, &server_addr, &header, buffer, &data_len);
        
        if (n > 0 && (header.flags & FIN)) {
            sprintf(logmmsg, "RCV FIN SEQ=%u ", header.seq_num);
            logm(logmmsg);

            struct sham_header fin_ack;
            fin_ack.flags = ACK; 
            fin_ack.ack_num = header.seq_num + 1;
            fin_ack.seq_num = seq_no; 
            fin_ack.window_size = SLIDING_WIN * PAYLOAD_SIZE;
            send_packet(sockfd, &server_addr, &fin_ack, NULL, 0);
            sprintf(logmmsg, "SND FINAL ACK=%u", fin_ack.ack_num);
            logm(logmmsg);
            
            break; 
        } else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {

            continue;
        }
    }
    
}

void md5print(char* fname){
    
    FILE *fd = fopen(fname, "rb");
    if (!fd) {
        perror("fopen");
        return;
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    int length = 0;
    
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        perror("EVP_MD_CTX_new");
        fclose(fd);
        return;
    }

    EVP_DigestInit_ex(mdctx, EVP_md5(), NULL);

    char data[PAYLOAD_SIZE];
    int len;
    while ((len=fread(data, 1,PAYLOAD_SIZE,fd)) != 0) {
        EVP_DigestUpdate(mdctx,data,len);
    }

    EVP_DigestFinal_ex(mdctx, hash, &length);
    EVP_MD_CTX_free(mdctx);
    
    fclose(fd);

    char md5string[EVP_MAX_MD_SIZE*2 + 1];

    for (int i = 0; i < length; i++) {
        sprintf(&md5string[i*2],"%02x",hash[i]);
    }
    md5string[length*2] = '\0';

    printf("MD5: %s\n", md5string);
}

int main(int argc,char* argv[]) {
    
    int sockfd;
    srand(time(NULL));

    if(argc>2 && strcmp(argv[2],"--chat")==0){
        mode=0;
        printf("chat\n");
        LOSS_PROBABILITY=atof(argv[3]);
    }
    else{
        LOSS_PROBABILITY=atof(argv[2]);
    }
    
    struct sockaddr_in server_addr, client_addr;
    int SERVER_PORT;
    if(argc>=2){
        SERVER_PORT=atoi(argv[1]);
    }
    logm_file = fopen("server_log.txt", "w");
    
    if (logm_file==NULL) {
        perror("logm file failed");
        exit(EXIT_FAILURE);
    }
    
    if ((sockfd=socket(AF_INET,SOCK_DGRAM,0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    struct timeval time;
    time.tv_sec=0;
    time.tv_usec=500000;

    if (setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO, &time,sizeof(time)) < 0) {
        perror("socketopt failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", SERVER_PORT);
    
    int data_len;
    char buffer[PAYLOAD_SIZE];
    struct sham_header header;

    int seq_no=connection_handshake(&sockfd,&client_addr);

    if(mode){
        char data[PAYLOAD_SIZE];
        if(argc>1){
            char logmmsg[200];

            while(recieve_packet(sockfd, &client_addr, &header, data, &data_len)<0){
    
            }
            
            char fname[256];
            memcpy(fname,data,data_len);
            fname[data_len]='\0';
            FILE *f=fopen(fname,"w");

            sprintf(logmmsg, "RCV DATA SEQ=%u LEN=%u", header.seq_num,data_len);
            logm(logmmsg);

            struct sham_header ack_hdr;
            //ack_hdr.seq_num = seq_no;
            ack_hdr.ack_num=header.seq_num+data_len;
            ack_hdr.flags = ACK;
            ack_hdr.window_size = SLIDING_WIN * PAYLOAD_SIZE;
            send_packet(sockfd, &client_addr, &ack_hdr,data,0);
            sprintf(logmmsg, "SND ACK=%u WIN=%u", ack_hdr.ack_num, ack_hdr.window_size);
            logm(logmmsg);

            
            while(1){
                while(recieve_packet(sockfd, &client_addr, &header, data, &data_len)<0){
    
                }
                if(header.flags & FIN){
                    finish_rcv(header,sockfd,client_addr,seq_no);
                    
                    break;
                }
                sprintf(logmmsg, "RCV DATA SEQ=%u LEN=%u", header.seq_num,data_len);
                logm(logmmsg);
                fwrite(data,1,data_len,f);
                struct sham_header ack_hdr;
                //ack_hdr.seq_num = seq_no;
                
                ack_hdr.ack_num=header.seq_num+data_len;
                ack_hdr.flags = ACK;
                ack_hdr.window_size = SLIDING_WIN * PAYLOAD_SIZE;
                send_packet(sockfd, &client_addr, &ack_hdr,data, 0);
                sprintf(logmmsg, "SND ACK=%u WIN=%u", ack_hdr.ack_num, ack_hdr.window_size);
                logm(logmmsg);
            }
            
            fclose(f);
            md5print(fname);
        }
        
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
            int n = recieve_packet(sockfd, &client_addr, &header, buffer, &data_len);
            if (n < 0){
                continue;
            }
            if (header.flags & FIN) {
                finish_rcv(header,sockfd,client_addr,seq_no);
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
                send_packet(sockfd, &client_addr, &ack_hdr, NULL, 0);
                sprintf(logmmsg, "SND ACK=%u WIN=%d", ack_hdr.ack_num, ack_hdr.window_size);
                logm(logmmsg);
            }

        }

        else if( FD_ISSET(0,&rd)){
            char logmmsg[200];
            char* data=NULL;;
            size_t size=PAYLOAD_SIZE;
            int len=getline(&data,&size,stdin);
            if(data[len-1]=='\n'){
                data[len-1]='\0';
            }
            if(strcmp(data,"/quit")==0){
                finish_snd(sockfd,client_addr,seq_no);
                break;
            }
            if(len>0){
                int f=1,retransmission=0;
                while(f){
                    struct sham_header syn_hdr;
                    syn_hdr.seq_num = seq_no;
                    //syn_hdr.ack_num = header.seq_num + data_len;
                    syn_hdr.flags = 0;
                    syn_hdr.window_size = SLIDING_WIN * PAYLOAD_SIZE;
                    send_packet(sockfd, &client_addr, &syn_hdr,data, len);
                    
                    if(retransmission){
                        sprintf(logmmsg, "RETX DATA SEQ=%u LEN=%d", syn_hdr.seq_num, len);
                        logm(logmmsg);
                    }
                    else{
                        sprintf(logmmsg, "SND SYN SEQ=%u WIN=%d", syn_hdr.seq_num, syn_hdr.window_size);
                        logm(logmmsg);
                    }

                    int n = recieve_packet(sockfd, &client_addr, &header, buffer, &data_len);
                    if (n < 0){
                        retransmission=1;
                        sprintf(logmmsg, "TIMEOUT SEQ=%u", seq_no);
                        logm(logmmsg);
                        continue;
                    }
                    if(!((header.flags & ACK) && header.ack_num ==seq_no+len)){
                        retransmission=1;
                        continue;
                    }
                    char logmmsg[200];
                    sprintf(logmmsg, "RCV  ACK=%u", header.ack_num);
                    logm(logmmsg);
                    f=0;
                    seq_no+=len;
                }
            }
        }
    }
    

    close(sockfd);
    fclose(logm_file);
    return 0;
}
