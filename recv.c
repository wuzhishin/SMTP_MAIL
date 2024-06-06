#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define MAX_SIZE 65535

char buf[MAX_SIZE+1];

void recv_mail()
{
    const char* host_name = "pop.qq.com"; // TODO: Specify the mail server domain name
    const unsigned short port = 110; // POP3 server port
    const char* user = "2145807962@qq.com"; // TODO: Specify the user
    const char* pass = "tltzmtjsbjapbbag"; // TODO: Specify the password
    char dest_ip[16];
    int s_fd; // socket file descriptor
    struct hostent *host;
    struct in_addr **addr_list;
    int i = 0;
    int r_size;

    // Get IP from domain name
    if ((host = gethostbyname(host_name)) == NULL)
    {
        herror("gethostbyname");
        exit(EXIT_FAILURE);
    }

    addr_list = (struct in_addr **) host->h_addr_list;
    while (addr_list[i] != NULL)
        ++i;
    strcpy(dest_ip, inet_ntoa(*addr_list[i-1]));

    // TODO: Create a socket,return the file descriptor to s_fd, and establish a TCP connection to the POP3 server
    // Create a socket
    if ((s_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    // Define the server address
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    inet_pton(AF_INET, dest_ip, &server.sin_addr);

    // Connect to the server
    if (connect(s_fd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    // Print welcome message
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0'; // Do not forget the null terminator
    printf("%s", buf);

    // TODO: Send user and password and print server response
    char user_command[256];
    sprintf(user_command, "USER %s\r\n", user);
    send(s_fd, user_command, strlen(user_command), 0);
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0';
    printf("%s", buf);

    // Send password and print server response
    char pass_command[256];
    sprintf(pass_command, "PASS %s\r\n", pass);
    send(s_fd, pass_command, strlen(pass_command), 0);
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0';
    printf("%s", buf);
    // TODO: Send STAT command and print server response
    const char *STAT = "STAT\r\n";
    send(s_fd, STAT, strlen(STAT), 0);
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0';
    printf("%s", buf);
    // TODO: Send LIST command and print server response
    const char *LIST = "LIST\r\n";
    send(s_fd, LIST, strlen(LIST), 0);
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0';
    printf("%s", buf);
    // TODO: Retrieve the first mail and print its content
    const char *RETR = "RETR 1\r\n";
    send(s_fd, RETR, strlen(RETR), 0);
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0';
    printf("%s", buf);
    // TODO: Send QUIT command and print server response
    const char *QUIT = "QUIT\r\n";
    send(s_fd, QUIT, strlen(QUIT), 0);
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0';
    printf("%s", buf);
    
    close(s_fd);
}

int main(int argc, char* argv[])
{
    recv_mail();
    exit(0);
}
