//NAME: Wenxuan Liu
//EMAIL: allenliux01@163.com
//ID: 805152602

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#include <errno.h>


int sockfd, newsockfd;
socklen_t clilen;
int port_no=7000; //default to be 7000, but can change by '--port=#'
struct sockaddr_in serv_addr, cli_addr;
char file_name[1024];
int file_valid = 0;
int file_len = 0;
char* file_content = NULL;

char* type_header = "Content-Type: text/html\r\n\0";
char* found_header = "HTTP/1.1 200 OK\r\n\0";
char* notfound_header = "HTTP/1.1 404 Not Found\r\n\0";
char* notfound_file = "<html><h1 style='text-align: center; font-size: 4em;'> Error 404! File Not Found. </h1></html>";

void get_filename(char* request);
void response_sender();
void file_handler();

void error(char* msg){
	fprintf(stderr, "Error: failed to %s.\n", msg);
}

void init_socket(){
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		error("open socket");
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);	//INADDR_ANY allows to connect to any one of the host’s IP address.
	serv_addr.sin_port = htons(port_no);
	if (bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
		error("binding");
	}
	listen(sockfd, 1);	// 1 is the longest length of request queue
}

void accept_socket(){
	while (1) {
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr*) &cli_addr, &clilen);
		if (newsockfd < 0) {
			error("accept");
		}

		char request[4096];
		bzero(request, 4096);
		if (read(newsockfd, request, 4096) < 0) {
			error("read socket");
		}
		int pid = fork();
		if (pid == -1){	//error for forking
			error("fork");
			exit(1);
		}
    else if (pid == 0) //child, Own copy of parent’s descriptors, e.g., Socket 
    {
    	printf("%s", request);
    	get_filename(request);
    	file_handler();
    	response_sender();
    	close(newsockfd);
    	exit(0);
    }
    else if (pid > 0)	//parent
    {
    	close(newsockfd);
    	waitpid(-1, NULL, WNOHANG); //wait for any child process. If no any finished cild, return immediately
    }
  }
}

void get_filename(char* request){
	bzero(file_name, 1024);
	char* s = "\r\n";
	char* first_line = strtok(request, s);	//get the first line of the request
	s = " ";
	char* word = strtok(first_line, s);	//parse the count seperate by ' '
	word = strtok(NULL, s);		//iterate to the second word

	char* rough_filename;
	rough_filename = word;	//the filename is the second word in the first line

	int i = 1, j = 0;	//i points to char in roughfilename, j points to char in file_name
	int filename_len = strlen(rough_filename);

	while (i < filename_len)
	{
		//process the space character "%20" in html header
		if (i+2<filename_len && rough_filename[i] == '%' && rough_filename[i+1] == '2' && rough_filename[i+2] == '0'){
			file_name[j] = ' ';
			i=i+3;
		}
		else{
			file_name[j] = rough_filename[i];
			i++;
		}
		j++;
	}
	file_name[j] = '\0';
}

void extension_parser(char* file){
	char* s = ".";
	char* pos = strtok(file, s);	//str before '.''
	pos = strtok(NULL, s); //get the str after '.' in the filename
	if (pos == NULL){
		type_header = "Content-Type: application/octet-stream\r\n\0";
		return;
	}
	if (strcmp("html", pos) == 0){
		type_header = "Content-Type: text/html\r\n\0";
	}
	else if (strcmp("txt", pos) == 0){
		type_header = "Content-Type: text/plain\r\n\0";
	}
	else if (strcmp("jpg", pos) == 0){
		type_header = "Content-Type: image/jpeg\r\n\0";
	}
	else if (strcmp("png", pos) == 0){
		type_header = "Content-Type: image/png\r\n\0";
	}
	else{
		type_header = "Content-Type: text/plain\r\n\0";
	}
}

void file_handler(){
	DIR *dir;
	struct dirent *np;
	dir = opendir("./");
	FILE* fp;
	if (dir != NULL)
	{
		while ((np = readdir(dir)))	//traverse the filenames in dir
		{
			if (strcasecmp(np->d_name, file_name) == 0) {
				fp = fopen(np->d_name, "r");
				extension_parser(np->d_name);
				if (fp == NULL)
					error("open file");
				else
					file_valid = 1;
			}
		}      
		closedir(dir);
	}

	if (file_valid)
	{
		fseek(fp, 0, SEEK_END);	//fp points to the end of file
		file_len = ftell(fp);	//the length of file
		file_content = malloc(sizeof(char)*file_len+1);
		fseek(fp, 0, SEEK_SET);

		fread(file_content, 1, file_len, fp);	//read all the content of fp into file_content
		file_content[file_len] = '\0';	//mark the end of the file
	}
}

void response_sender()
{
	if(file_valid){
		if (write(newsockfd, found_header, strlen(found_header)) < 0)
			error("write to new socket");
	}
	else{
		if (write(newsockfd, notfound_header, strlen(notfound_header)) < 0)
			error("write to new socket");
	}

	//write the header line into new socket
	if (write(newsockfd, type_header, strlen(type_header)) < 0)
		error("write to new socket");
	char* CRLF = "\r\n\0";
	if (write(newsockfd, CRLF, strlen(CRLF)) < 0)	//to mark the end of header line and beginning of content
		error("write to new socket");

	//Write the file content into new socket
	if (file_valid){
		if (write(newsockfd, file_content, file_len) < 0)
			error("write to new socket");
	} else{
		if (write(newsockfd, notfound_file, strlen(notfound_file)) < 0)
			error("write to new socket");
	}
	free(file_content);
}


int main(int argc, char* argv[]){
	struct option opts[] = {
		{"port", 1, NULL, 'p'},
	}; 

	int c; //parse options
	while ((c = getopt_long(argc, argv, "", opts, NULL)) != -1) {
		switch (c) {
			case 'p':
			port_no = atoi(optarg);
			if (port_no <= 0) {
				fprintf(stderr, "Error: invalid port number.\n");
				exit(1);
			}
			break;
			default:
			fprintf(stderr, "Error. Invald option.\n");
			exit(1);
		}
	}

	init_socket();

	accept_socket();
}




