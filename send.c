#include <stdio.h>
#include "base64_utils.h"
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <getopt.h>


#define MAX_SIZE 4095

char buf[MAX_SIZE+1];


// receiver: mail address of the recipient
// subject: mail subject
// msg: content of mail body or path to the file containing mail body
// att_path: path to the attachment
void send_mail(const char* receiver, const char* subject, const char* msg, const char* att_path)
{
    const char* end_msg = "\r\n.\r\n";
    const char* host_name = "smtp.qq.com"; // TODO: Specify the mail server domain name
    const unsigned short port = 25; // SMTP server port
    const char* user = ""; // TODO: Specify the user
    const char* pass = ""; // TODO: Specify the password
    const char* from = ""; // TODO: Specify the mail address of the sender
    char dest_ip[16]; // Mail server IP address
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

    // TODO: Create a socket, return the file descriptor to s_fd, and establish a TCP connection to the mail server
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

    // Send EHLO command and print server response
    const char* EHLO = "EHLO qq.com\r\n"; // TODO: Enter EHLO command here
    send(s_fd, EHLO, strlen(EHLO), 0);
    // TODO: Print server response to EHLO command
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0';
    printf("%s", buf);
    // TODO: Authentication. Server response should be printed out.
    // Send AUTH LOGIN command and print server response
    const char* AUTH_LOGIN = "AUTH LOGIN\r\n";
    send(s_fd, AUTH_LOGIN, strlen(AUTH_LOGIN), 0);
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0';
    printf("%s", buf);

    // Send username and print server response
    char* encoded_user = encode_str(user);
    send(s_fd, encoded_user, strlen(encoded_user), 0);
    send(s_fd, "\r\n", 2, 0);
    free(encoded_user);
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0';
    printf("%s", buf);

    // Send password and print server response
    char* encoded_pass = encode_str(pass);
    send(s_fd, encoded_pass, strlen(encoded_pass), 0);
    send(s_fd, "\r\n", 2, 0);
    free(encoded_pass);
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0';
    printf("%s", buf);
    // TODO: Send MAIL FROM command and print server response
    char* mail_from = malloc(strlen(from) + 14);
    sprintf(mail_from, "MAIL FROM: <%s>\r\n", from);
    send(s_fd, mail_from, strlen(mail_from), 0);
    free(mail_from);
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0';
    printf("%s", buf);
    // TODO: Send RCPT TO command and print server response
    char* rcpt_to = malloc(strlen(receiver) + 12);
    sprintf(rcpt_to, "RCPT TO: <%s>\r\n", receiver);
    send(s_fd, rcpt_to, strlen(rcpt_to), 0);
    free(rcpt_to);
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0';
    printf("%s", buf);
    // TODO: Send DATA command and print server response
    const char* DATA = "DATA\r\n";
    send(s_fd, DATA, strlen(DATA), 0);
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0';
    printf("%s", buf);
    // TODO: Send message data
    

    char email_content[MAX_SIZE] = "";
    snprintf(email_content, MAX_SIZE,
            "From: %s\r\n"
            "To: %s\r\n"
            "Subject: %s\r\n"
            "Content-Transfer-Encoding: base64\r\n"
            "Content-Type: multipart/mixed;boundary=@boundary@\r\n\r\n",
            from, receiver, subject);

    // Send the email content
    send(s_fd, email_content, strlen(email_content), 0);

    

    if (msg != NULL)
    {
        FILE* msg_file = fopen(msg, "r");
        if (msg_file == NULL)
        {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        fseek(msg_file, 0, SEEK_END);
        long file_size = ftell(msg_file);
        fseek(msg_file, 0, SEEK_SET);

        char* content = malloc(file_size + 1); 
        if (content == NULL) {
            perror("malloc");
            fclose(msg_file);
            return NULL;
        }

        fread(content, 1, file_size, msg_file);
        content[file_size] = '\0'; // null terminator

        fclose(msg_file);

        // char* encoded_content = encode_str(content);
        char message[MAX_SIZE] = "";
        snprintf(message, MAX_SIZE,
                "--@boundary@\r\nContent-Type: text/plain;charset=\"utf-8\"\r\n\r\n"
                "%s\r\n\r\n",
                content);
        send(s_fd, message, strlen(message), 0);
        send(s_fd, "\r\n", 2, 0);
        // free(encoded_content);
    }

    

    // Send attachment data
    if (att_path != NULL)
    {
        FILE* attachmentFile = fopen(att_path, "rb");
        if (attachmentFile == NULL) {
            printf("Error: Unable to open attachment file.\n");
            return;
        }

        // Create a temporary file for the encoded attachment
        FILE* encodedAttachmentFile = tmpfile();
        if (encodedAttachmentFile == NULL) {
            printf("Error: Unable to create temporary file for encoded attachment.\n");
            fclose(attachmentFile);
            return;
        }
        encode_file(attachmentFile, encodedAttachmentFile);
        fclose(attachmentFile);

        fseek(encodedAttachmentFile, 0, SEEK_END);
        long file_size = ftell(encodedAttachmentFile);
        fseek(encodedAttachmentFile, 0, SEEK_SET);

        char * buffer = (char *)malloc(file_size);
        if (buffer == NULL) {
            perror("Memory allocation failed");
            fclose(encodedAttachmentFile);
            exit(EXIT_FAILURE);
        }

        // 读取文件内容到缓冲区
        size_t bytes_read = fread(buffer, 1, file_size, encodedAttachmentFile);
        if (bytes_read != file_size) {
            perror("Reading file failed");
            fclose(encodedAttachmentFile);
            free(buffer);
            exit(EXIT_FAILURE);
        }

        char file_content[MAX_SIZE] = "";
        snprintf(file_content, MAX_SIZE,
                "--@boundary@\r\nContent-Type: application/octet-stream; name=\"%s\"\r\n"
                "Content-Disposition: attachment; filename=\"%s\"\r\n"
                "Content-Transfer-Encoding: base64\r\n\r\n"
                "%s\r\n",
                att_path, att_path, buffer);

        // Send the email content
        send(s_fd, file_content, strlen(file_content), 0);

        free(encodedAttachmentFile);
    }
    
    // TODO: Message ends with a single period
    // Send end of message
    send(s_fd, end_msg, strlen(end_msg), 0);
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0';
    printf("%s", buf);
    // TODO: Send QUIT command and print server response
    const char* QUIT = "QUIT\r\n";
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
    int opt;
    // char* s_arg = NULL;
    // char* m_arg = NULL;
    // char* a_arg = NULL;
    // char* recipient = NULL;
    char* s_arg = NULL;
    char* m_arg = NULL;
    char* a_arg = NULL;
    char* recipient = NULL;
    const char* optstring = ":s:m:a:";
    while ((opt = getopt(argc, argv, optstring)) != -1)
    {
        switch (opt)
        {
        case 's':
            s_arg = optarg;
            break;
        case 'm':
            m_arg = optarg;
            break;
        case 'a':
            a_arg = optarg;
            break;
        case ':':
            fprintf(stderr, "Option %c needs an argument.\n", optopt);
            exit(EXIT_FAILURE);
        case '?':
            fprintf(stderr, "Unknown option: %c.\n", optopt);
            exit(EXIT_FAILURE);
        default:
            fprintf(stderr, "Unknown error.\n");
            exit(EXIT_FAILURE);
        }
    }

    if (optind == argc)
    {
        fprintf(stderr, "Recipient not specified.\n");
        exit(EXIT_FAILURE);
    }
    else if (optind < argc - 1)
    {
        fprintf(stderr, "Too many arguments.\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        recipient = argv[optind];
        send_mail(recipient, s_arg, m_arg, a_arg);
        exit(0);
    }

}

