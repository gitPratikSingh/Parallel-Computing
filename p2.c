#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#define PORT 2025

int MPI_Init(int in_proc_count)
{
 /*
    PART 1:
    Set up the server in this machine on the port PORT
 */

 // sock_fd is the file descriptor of server socket 
 int sock_fd = InitServerSocket();
 
  /*
    PART 2: 
    After setting up the server, update the syncFile so that everyone knows that this process has completed
        initiation
 */
 
  char* file = "syncFile";
  int fd_read, fd_write;
  struct flock lock;

  printf ("opening %s\n", file);
  /* Open a file descriptor to the file.  */
  fd_write = open (file, O_WRONLY);
  fd_read = open (file, O_RDONLY);
  printf ("locking\n");
  
  /* Initialize the flock structure.  */
  memset (&lock, 0, sizeof(lock));
  lock.l_type = F_WRLCK;
  
  /* Place a write lock on the file blocks until the request is granted*/
  fcntl (fd_write, F_SETLKW, &lock);
  char readCount[2];

  //read the file to get the existing count
  int count = read(fd_read, readCount, 1);

  printf("read %c %d\n", readCount[0], count);
  readCount[1]='\0';

  int val = 1 + atoi(readCount);
  sprintf (readCount, "%d", val);
  
  printf("write: %s\n", readCount);
  write(fd_write, readCount, 1);
  
  printf ("unlocking\n");
  
  /* Release the lock.  */
  lock.l_type = F_UNLCK;
  fcntl (fd_write, F_SETLKW, &lock);
  close (fd_write);
  close (fd_read);
  
  printf("File unlocked");
  
  // now wait till the count in the syncFile equals the proc count
  int proc_count= atoi(in_proc_count);
  
  int cur_count = 0;
  printf("Going to while: proccount %d", proc_count);

  while(cur_count<proc_count){
    int fd = open (file, O_RDONLY);
    read(fd, readCount, 1);
    close (fd);
    readCount[1]='\0';
    cur_count = atoi(readCount);
    
    printf("Going to sleep for 1sec");
    
    sleep(1);
    printf("Woke up");
  }

  // all processes have initiated!
  printf("Initiation Complete");

  return sock_fd;
}

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int InitServerSocket()
{
     int sockfd;
     struct sockaddr_in serv_addr;
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     
     if (*sockfd < 0) 
        error("ERROR opening socket");
     
     bzero((char *) &serv_addr, sizeof(serv_addr));
     
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;  //INADDR_ANY is the symbolic constant that contains the IP addr of the machine the code is runnin on
     serv_addr.sin_port = htons(PORTNO);
     
     if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        error("ERROR on binding");
        
     listen(sockfd,5);
     return sockfd; 
}


int sendMessageClient(char *hostname, char *message)
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (sockfd < 0) 
        error("ERROR opening socket");
    
    server = gethostbyname(hostname);
    
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    
    serv_addr.sin_family = AF_INET;
    
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(PORT);
    
    // blocks until accepted by the server
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");
    
    n = write(sockfd,message,strlen(message));
    
    if (n < 0) 
         error("ERROR writing to socket");
    
    close(sockfd);
    return 0;    
}

int main(int argc, char **argv)
{
    FILE *f = fopen("hostfile", "r");
    char hostfile[100];
    char *token, *myhostname = argv[1], *proc_count_str = *argv[2];
    char *delim="|";
    
    int proc_count = atoi(proc_count_str);
    
    // blocks untill all the processess have set up the following
    // bind the server ports and starts to listen to them
    
    int ssock_fd = MPI_Init(proc_count);
    
    int i=0;
    while(strcmp(hostfile, myhostname) !=0 ){
        fgets(hostfile, 100, f);
        hostfile[strlen(hostfile)-1]='\0';
        token=strtok(hostfile, delim);
        printf("String read:%s %s", token, myhostname);
        i++;
        if(i==8){break;}
    }
    
    char *recr=strtok(NULL, delim);
    char *senr=strtok(NULL, delim);
    printf("Recr,sendr:%s %s", recr, senr);
    int is_root = strcmp(recr, "----")==0?1:0;
    
    if(is_root == 1){
        // root
        // send the data to the next rank proc and then wait for the return of data at server socket
        
        // connect a socket to the server at senr!
        sendMessageClient(senr, "This is a test message");
        
        // start waiting for the message to comeback to server
        int newsockfd;
        socklen_t clilen;
        char buffer[256];
        struct sockaddr_in cli_addr;
        clilen = sizeof(cli_addr);
        
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        
        if (newsockfd < 0) 
              error("ERROR on accept");
              
        bzero(buffer,256);
         
        int n = read(newsockfd,buffer,255);
         
        if (n < 0) error("ERROR reading from socket");
        
        printf("Here is the message: %s\n",buffer);
         
        close(newsockfd);
        close(ssock_fd);
         
    }else{
        //other process
        
        int newsockfd;
        socklen_t clilen;
        char buffer[256];
        struct sockaddr_in cli_addr;
        clilen = sizeof(cli_addr);
        
        newsockfd = accept(ssock_fd, (struct sockaddr *) &cli_addr, &clilen);
        
        if (newsockfd < 0) 
              error("ERROR on accept");
              
        bzero(buffer,256);
         
        int n = read(newsockfd,buffer,255);
         
        if (n < 0) error("ERROR reading from socket");
        
        printf("Here is the message: %s\n",buffer);
         
        close(newsockfd);
        close(ssock_fd);
        
        sendMessageClient(senr, buffer);
    }
    
    fclose(f);
    return 0;
}
