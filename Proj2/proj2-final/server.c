//Wenxuan Liu
//Email: allenliux01@163.com
//ID: 805152602

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <limits.h>
#include <fcntl.h>

#define MAX_BUFFER 524
#define TIMEOUT 500

struct packet{
	uint16_t seq_no;
	uint16_t ack_no;
	uint16_t ack_flag;
	uint16_t syn_flag;
	uint16_t fin_flag;
	uint16_t padding;
	char data[512];
};

int sockfd, newsockfd;
char *port;
int connection_no = 0;
int window_size = 5120;
uint16_t seq_cur;
uint16_t ack_next;
char content[20000000];

void error(char* msg){
	fprintf(stderr, "Error: %s.\n", msg);
	exit(1);
}

void print_header(int type, struct packet p){
	if(type == 0)
		printf("SEND %d %d", p.seq_no, p.ack_no);
	if(type == 1)
		printf("RECV %d %d", p.seq_no, p.ack_no);
	if(type == 2)
		printf("RESEND %d %d", p.seq_no, p.ack_no);
	if(type == 3){
		printf("SEND %d %d DUP-ACK\n", p.seq_no, p.ack_no);
		return;
	}
	if(p.syn_flag != 0){
		printf(" SYN");
	}
	if(p.fin_flag != 0){
		printf(" FIN");
	}
	if(p.ack_flag != 0){
		printf(" ACK");
	}
	printf("\n");
}

void print_timeout(struct packet p){
	printf("TIMEOUT %d\n", p.seq_no);
}

unsigned long sysTime() {
	struct timespec x;
	clock_gettime(CLOCK_REALTIME, &x);
	return (x.tv_sec) * 1000 + (x.tv_nsec) / 1000000;
}

struct packet init_packet(uint16_t seq, uint16_t ack, uint16_t ackf, uint8_t synf, uint8_t finf){
	struct packet pac;
	bzero(&pac, sizeof(pac));
	pac.seq_no = seq;
	pac.ack_no = ack;
	pac.ack_flag = ackf;
	pac.syn_flag = synf;
	pac.fin_flag = finf;
	return pac;
}


int main(int argc, char *argv[]){
	if(argc != 2){
		error("wrong input usage. Should be ./server <port>");
	}
	port = atoi(argv[1]);

	while(1){		//keep setting up new socket and waiting for new connections
		int sockfd;
		if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
			error("cannot create socket");
		}

		struct sockaddr_in servaddr;
		memset(&servaddr, 0, sizeof(struct sockaddr_in));
		servaddr.sin_family = AF_INET; //IPv4
		servaddr.sin_port = htons(port);
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY); //use local ip

		if(bind(sockfd, (const struct sockaddr*)&servaddr, sizeof(struct sockaddr_in)) < 0){
			printf("cannot bind\n");
			exit(2);
		}

		// int flags = fcntl(sockfd,F_GETFL,0);
		// fcntl(sockfd,F_SETFL,flags|O_NONBLOCK);

		connection_no += 1;

		struct sockaddr_in clientaddr;
		memset (&clientaddr, 0, sizeof(struct sockaddr_in));
		socklen_t len=sizeof(clientaddr);
		int recvlen=-1;

		time_t t;
		srand((unsigned) time(&t));
		seq_cur = rand()%25600;

		struct packet rec_pac;


		//Open File
		char filename[20] = {0};
		sprintf(filename, "%d", connection_no);
		strcat(filename, ".file");
		FILE *fp;

		int first_syn = 1;
		while(1){
			bzero(&rec_pac, sizeof(rec_pac));
			recvlen = -1;
			while(recvlen<0){
				recvlen = recvfrom(sockfd, &rec_pac, sizeof(rec_pac), MSG_DONTWAIT, (struct sockaddr*)&clientaddr, &len);
			}
			print_header(1, rec_pac);

			if(rec_pac.syn_flag){
				struct packet syn_ack = init_packet(seq_cur, (rec_pac.seq_no+1)%25601,1,1,0);
				sendto(sockfd, &syn_ack, 13, 0, (const struct sockaddr*)&clientaddr, len);
				ack_next = (rec_pac.seq_no+1)%25601;
				// printf("ACKNEXT:%d\n", ack_next);
				if(first_syn){
					first_syn = 0;
					print_header(0, syn_ack);
				}
				else{	//When the syn ack lost, server may receive another syn request
					print_header(3, syn_ack);
				}
				fp = fopen(filename, "w");	//only create file when syn received.
				if(fp == NULL){
					close(sockfd);
					error("Cannot create the file");
				}
				continue;
			}

			//when the data has all been received with ack, receive fin request here
			if(rec_pac.fin_flag){
				struct packet fin_ack = init_packet(seq_cur, (rec_pac.seq_no+1)%25601, 1,0,0);
				sendto(sockfd, &fin_ack, 13, 0, (const struct sockaddr*)&clientaddr, len);
				print_header(0, fin_ack);
				break;
			}

			// int data_size = 0;
			// for(;data_size<512; data_size++){
			// 	if(rec_pac.data[data_size] == '\0')
			// 		break;
			// }

			//Check in order. If not, discard and send dup ack; If yes, fwrite and forward.
			if(rec_pac.ack_flag){
				seq_cur = (seq_cur+1)%25601;
			}
			if(rec_pac.seq_no == ack_next){
				fwrite(rec_pac.data, recvlen-12, 1, fp);
				struct packet ack_pac = init_packet(seq_cur, (rec_pac.seq_no+recvlen-12)%25601, 1,0,0);
				sendto(sockfd, &ack_pac, 13, 0, (const struct sockaddr*)&clientaddr, len);
				print_header(0, ack_pac);	
				ack_next = (ack_next+recvlen-12)%25601;
			}
			else{
				struct packet ack_pac = init_packet(seq_cur, ack_next, 1,0,0);
				sendto(sockfd, &ack_pac, 13, 0, (const struct sockaddr*)&clientaddr, len);
				print_header(3, ack_pac);	
			}
			
		}

		//keep initiate fin until receive ack
		int resend = 0;
		int close_ready = 0;
		while(1){
			struct packet fin_pac = init_packet(seq_cur,0,0,0,1);
			sendto(sockfd, &fin_pac, 13, 0, (const struct sockaddr*)&clientaddr, len);
			if(resend)
				print_header(2, fin_pac);
			else{
				resend=1;
				print_header(0, fin_pac);
			}
			
			bzero(&rec_pac, sizeof(rec_pac));
			unsigned long expire = sysTime()+TIMEOUT;
			while(1){	//wait 500ms for receive the ack. if not, resend
				recvlen=recvfrom(sockfd, &rec_pac, sizeof(rec_pac), MSG_DONTWAIT, (struct sockaddr*)&clientaddr, &len);
				if(recvlen>0){
					if(rec_pac.fin_flag){	//receives the fin request again due to ack lost
						struct packet fin_ack = init_packet(seq_cur, (rec_pac.seq_no+1)%25601, 1,0,0);
						sendto(sockfd, &fin_ack, 13, 0, (const struct sockaddr*)&clientaddr, len);
						print_header(0, fin_ack);
						break;
					}
					print_header(1, rec_pac);
					close_ready = 1;
					break;
				}
				if(sysTime()>=expire){	//not receive anything
					print_timeout(fin_pac);
					break;
				}
			}
			if(close_ready){
				break;
			}
		}

		close(sockfd);
		fclose(fp);
	}
	return 0;
}










