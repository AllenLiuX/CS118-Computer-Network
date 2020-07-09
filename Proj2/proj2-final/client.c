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

uint16_t seq_cur;
int file_len = 0;
char* file_content = NULL;
int pac_num = 0;
struct packet window[10];
unsigned long timer[10];
int pac_size[10];

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

unsigned long sysTime() {	//msec
	struct timespec x;
	clock_gettime(CLOCK_REALTIME, &x);
	return (x.tv_sec) * 1000 + (x.tv_nsec) / 1000000;
}

void print_timeout(struct packet p){
	printf("TIMEOUT %d\n", p.seq_no);
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
	if(argc != 4){
		error("wrong input usage. Should be ./server <HOSTNAME-OR-IP> <PORT> <FILENAME>");
	}
	char* hostname = argv[1];
	int port = atoi(argv[2]);
	char* filename = argv[3];

	int sockfd;
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		error("cannot create socket");
	}

	time_t t;
	srand((unsigned) time(&t));
	seq_cur = rand()%25600;

	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(struct sockaddr_in));
	servaddr.sin_family = AF_INET; //IPv4
	servaddr.sin_port = htons(port);
	struct hostent *server = gethostbyname(hostname);
	if(server == NULL){
		error("cannot find the host");
	}
	bcopy((char*)server->h_addr, (char*)&servaddr.sin_addr.s_addr, server->h_length);

	unsigned int len;
	int recvlen;
	struct packet rec_pac;
	bzero(&rec_pac, sizeof(rec_pac));

	//Syn Part
	int first_syn = 1;
	while(1){
		struct packet syn_pac = init_packet(seq_cur,0,0,1,0);
		sendto(sockfd, &syn_pac, 13, 0, (const struct sockaddr*)&servaddr, sizeof(servaddr));
		if(first_syn){
			first_syn = 0;
			print_header(0, syn_pac);
		}
		else{
			print_timeout(syn_pac);
			print_header(2, syn_pac);	//when the fin request is lost
		}
		unsigned long expire = sysTime()+TIMEOUT;
		bzero(&rec_pac, sizeof(rec_pac));
		while(recvfrom(sockfd, &rec_pac, 13, MSG_DONTWAIT, (struct sockaddr*)&servaddr, &len)<0){
			if(sysTime()>expire){
				break;
			}
		}
		if(rec_pac.ack_flag && rec_pac.syn_flag){
			print_header(1, rec_pac);
			break;
		}
	}


	//Read file into a buffer
	FILE *fp = fopen(filename, "r");
	if(fp == NULL){
		close(sockfd);
		error("No such file");
	}
	fseek(fp, 0, SEEK_END);	//fp points to the end of file
	file_len = ftell(fp);	//the length of file
	file_content = malloc(sizeof(char)*file_len+1);
	fseek(fp, 0, SEEK_SET);
	fread(file_content, 1, file_len, fp);	//read all the content of fp into file_content
	file_content[file_len] = '\0';	//mark the end of the file


	//Pipeline Data Part
	int content_cur = 0;
	int last_pac = -1;
	int dup_ack = 0;
	seq_cur = rec_pac.ack_no;
	while(1){
		//only send when still has content and not dup ack received
		if(content_cur<file_len && dup_ack == 0){	//ignore the dup ack
			struct packet data_pac;
			if (content_cur == 0){
				data_pac = init_packet(seq_cur,(rec_pac.seq_no+1)%25601,1,0,0);
			}
			else{
				data_pac = init_packet(seq_cur,0,0,0,0);
			}
			int data_size;
			if (file_len - content_cur > 512){
				data_size = 512;
				seq_cur = (seq_cur+512)%25601;
			}
			else{
				data_size = file_len - content_cur;	//so that the EOF at content[len] is included.
				seq_cur = (seq_cur+data_size)%25601;	//for the next pac
				last_pac = seq_cur;
			}
			for(int i=0; i<data_size; i++){
				data_pac.data[i] = file_content[content_cur];
				content_cur++;
			}

			window[pac_num] = data_pac;	//As long as the window is not full, send packets
			timer[pac_num] = sysTime()+500;
			pac_size[pac_num] = 12+data_size;
			sendto(sockfd, &data_pac, 12+data_size, 0, (const struct sockaddr*)&servaddr, sizeof(servaddr));
			print_header(0, data_pac);
			pac_num+=1;
		}
		// if(last_pac != -1)
		// 	 pac_num+=1;

		//only wait and receive ack when window is full or last pac has sent
		if(pac_num >= 10 || last_pac != -1){
			bzero(&rec_pac, sizeof(rec_pac));
			while((recvlen = recvfrom(sockfd, &rec_pac, 13, MSG_DONTWAIT, (struct sockaddr*)&servaddr, &len))<0){
				if(sysTime()>timer[0]){
					break;
				}
			}
			if(recvlen > 0){
				print_header(1, rec_pac);
				if(rec_pac.ack_no == last_pac){
					break;
				}
				int num_acked=1;	//the ack correponds to #num_acked packet
				dup_ack = 1;	//dup_ack is 1 later on only when window size still == 10
				for(; num_acked < pac_num; num_acked++){
					if(window[num_acked].seq_no == rec_pac.ack_no){
						dup_ack = 0;
						break;
					}
				}
				
				if(dup_ack == 0){	//is ack for a pac in window
					pac_num-=num_acked;	//shorten the window size so that next turn can send out
					for(int i=0; i<10-num_acked; i++){
						window[i] = window[i+num_acked];
						timer[i] = timer[i+num_acked];
						pac_size[i] = pac_size[i+num_acked];
					}
				}	//above is only for received good ack
			}	
		}

		if(sysTime()>timer[0]){
			print_timeout(window[0]);
			//resend the packets in the window immediately
			for(int i=0; i<pac_num; i++){
				timer[i] = sysTime() + 500;
				sendto(sockfd, &window[i], pac_size[i], 0, (const struct sockaddr*)&servaddr, sizeof(servaddr));
				print_header(2, window[i]);
			}
		}
	}


	//Fin Part, at here all the data has been acked
	//keep initiate fin before receive the ack
	int first_fin = 1;
	unsigned long expire = sysTime()+100000;	//initialize with big enough value
	while(1){
		struct packet fin_pac = init_packet(seq_cur,0,0,0,1);
		sendto(sockfd, &fin_pac, 13, 0, (const struct sockaddr*)&servaddr, sizeof(servaddr));
		//first time is send and second time is resend
		if(first_fin){
			first_fin = 0;
			print_header(0, fin_pac);
		}
		else{
			print_timeout(fin_pac);
			print_header(2, fin_pac);	//when the fin request is lost
		}
		expire = sysTime()+500;
		bzero(&rec_pac, sizeof(rec_pac));
		while((recvlen=recvfrom(sockfd, &rec_pac, 13, MSG_DONTWAIT, (struct sockaddr*)&servaddr, &len))<0){
			if(sysTime()>expire){
				break;
			}
		}
		if(rec_pac.ack_flag){	//if expire, the ack_no is 0; also if receive the fin request from server, ack_no is 0
			print_header(1, rec_pac);
			break;
		}
	}
	
	//keep receive fin request and response, and close 2s after the first fin request
	seq_cur = (seq_cur+1)%25601;
	first_fin = 1;
	int close_ready = 0;
	expire = sysTime()+10000;
	while(1){
		//keep receive the remaining acks
		bzero(&rec_pac, sizeof(rec_pac));
		while(recvfrom(sockfd, &rec_pac, 13, MSG_DONTWAIT, (struct sockaddr*)&servaddr, &len)<0){
			if(sysTime()>expire){	//close socket immediately even when still waiting
				close_ready = 1;
				break;
			}
		}
		if(close_ready){
			break;
		}
		print_header(1, rec_pac);
		//process the fin request
		if(rec_pac.fin_flag){	//do nothing when receiving ack, but send ack when receiving fin
			struct packet fin_ack = init_packet(seq_cur, (rec_pac.seq_no+1)%25601,1,0,0);
			sendto(sockfd, &fin_ack, 13, 0, (const struct sockaddr*)&servaddr, sizeof(servaddr));
			if(first_fin){
				expire = sysTime()+2000;
				first_fin = 0;
				print_header(0, fin_ack);
			}
			else{
				print_header(3, fin_ack);	//DUP ACK
			}
		}
	}
	close(sockfd);
	fclose(fp);
	return 0;
}














