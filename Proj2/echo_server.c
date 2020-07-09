#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#define MAXLINE 4096 /*max text line length*/
#define SERV_PORT 9000 /*port*/
#define LISTENQ 20 /*maximum number of client connections */

int main (int argc, char **argv)
{
 int listenfd, connfd, n;
 pid_t childpid;
 socklen_t clilen;
 char buf[MAXLINE];
 struct sockaddr_in cliaddr, servaddr;
 char user_command[MAXLINE];
	
 //creation of the socket
 listenfd = socket (AF_INET, SOCK_STREAM, 0);
	
 //preparation of the socket address 
 servaddr.sin_family = AF_INET;
 servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY allows client to connect to ANY one of the host's IP address.
 servaddr.sin_port = htons(SERV_PORT);
 memset(servaddr.sin_zero, '\0', sizeof(servaddr.sin_zero));
	
 if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr))==-1) {
  perror("bind");
  exit(1);
 }
	
 if (listen(listenfd, LISTENQ)==-1) {
  perror("listen");
  exit(1);
 }
	
 printf("%s\n","Server running...waiting for connections.");
	
 for ( ; ; ) {
  clilen = sizeof(cliaddr);
  connfd = accept (listenfd, (struct sockaddr *) &cliaddr, &clilen);
  printf("%s\n","Received request...");
				
  while ( (n = recv(connfd, buf, MAXLINE,0)) > 0)  {
   printf("%s","String received from and resent to the client:");
   puts(buf);
   send(connfd, buf, n, 0);
	 if (strcmp(buf,"q\n")==0) {
	  printf("Quit...\n");
    break;
   }
	 memset(buf, '\0', MAXLINE);
  }
			
  if (n < 0) {
   perror("Read error"); 
   exit(1);
  }
	printf("Close connection fd...\n");
  close(connfd);

  //if (fgets(user_command, MAXLINE, stdin) != NULL) {
  // if (strcmp(user_command,"q\n")==0)
  //  break;
  // memset(user_command, '\0', MAXLINE);
  //}
 }
 //close listening socket
 printf("Close listening socket...\n");
 close (listenfd); 
}
