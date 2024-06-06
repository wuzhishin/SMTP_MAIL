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

const int CHARS_PER_LINE = 72;

#define SIZE 4096

typedef enum
{
	step_A, step_B, step_C
} base64_encodestep;

typedef struct
{
	base64_encodestep step;
	char result;
	int stepcount;
} base64_encodestate;

void base64_init_encodestate(base64_encodestate* state_in)
{
	state_in->step = step_A;
	state_in->result = 0;
	state_in->stepcount = 0;
}

char base64_encode_value(char value_in)
{
	static const char* encoding = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	if (value_in > 63) return '=';
	return encoding[(int)value_in];
}

int base64_encode_block(const char* plaintext_in, int length_in, char* code_out, base64_encodestate* state_in)
{
	const char* plainchar = plaintext_in;
	const char* const plaintextend = plaintext_in + length_in;
	char* codechar = code_out;
	char result;
	char fragment;
	
	result = state_in->result;
	
	switch (state_in->step)
	{
		while (1)
		{
	case step_A:
			if (plainchar == plaintextend)
			{
				state_in->result = result;
				state_in->step = step_A;
				return codechar - code_out;
			}
			fragment = *plainchar++;
			result = (fragment & 0x0fc) >> 2;
			*codechar++ = base64_encode_value(result);
			result = (fragment & 0x003) << 4;
	case step_B:
			if (plainchar == plaintextend)
			{
				state_in->result = result;
				state_in->step = step_B;
				return codechar - code_out;
			}
			fragment = *plainchar++;
			result |= (fragment & 0x0f0) >> 4;
			*codechar++ = base64_encode_value(result);
			result = (fragment & 0x00f) << 2;
	case step_C:
			if (plainchar == plaintextend)
			{
				state_in->result = result;
				state_in->step = step_C;
				return codechar - code_out;
			}
			fragment = *plainchar++;
			result |= (fragment & 0x0c0) >> 6;
			*codechar++ = base64_encode_value(result);
			result  = (fragment & 0x03f) >> 0;
			*codechar++ = base64_encode_value(result);
			
			++(state_in->stepcount);
			if (state_in->stepcount == CHARS_PER_LINE/4)
			{
				*codechar++ = '\n';
				state_in->stepcount = 0;
			}
		}
	}
	/* control should not reach here */
	return codechar - code_out;
}

int base64_encode_blockend(char* code_out, base64_encodestate* state_in)
{
	char* codechar = code_out;
	
	switch (state_in->step)
	{
	case step_B:
		*codechar++ = base64_encode_value(state_in->result);
		*codechar++ = '=';
		*codechar++ = '=';
		break;
	case step_C:
		*codechar++ = base64_encode_value(state_in->result);
		*codechar++ = '=';
		break;
	case step_A:
		break;
	}
	*codechar++ = '\n';
	
	return codechar - code_out;
}





void base64_init_encodestate(base64_encodestate* state_in);

char base64_encode_value(char value_in);

int base64_encode_block(const char* plaintext_in, int length_in, char* code_out, base64_encodestate* state_in);

int base64_encode_blockend(char* code_out, base64_encodestate* state_in);

char* encode_str(const char* input)
{
    /* length of input string */
    int len = strlen(input);
    /* set up a destination buffer large enough to hold the encoded data */
    char* output = (char*) malloc(len*2);
    /* keep track of our encoded position */
    char* c = output;
    /* store the number of bytes encoded by a single call */
    int cnt = 0;
    /* we need an encoder state */
    base64_encodestate s;

    /* String length should not be greater than 3,000 */
    if (len > 3000)
    {
        fprintf(stderr, "Input too long!\n");
        return NULL;
    }
    
    /*---------- START ENCODING ----------*/
    /* initialize the encoder state */
    base64_init_encodestate(&s);
    /* gather data from the input and send it to the output */
    cnt = base64_encode_block(input, len, c, &s);
    c += cnt;
    /* since we have encoded the entire input string, we know that 
        there is no more input data; finalise the encoding */
    cnt = base64_encode_blockend(c, &s);
    c += cnt;
    /*---------- STOP ENCODING  ----------*/
    
    /* we want to print the encoded data, so null-terminate it: */
    *c = 0;
    
    return output;
}

void encode_file(FILE* inputFile, FILE* outputFile)
{
    /* set up a destination buffer large enough to hold the encoded data */
    int size = SIZE;
    char* input = (char*)malloc(size);
    char* encoded = (char*)malloc(2*size); /* ~4/3 x input */
    /* we need an encoder and decoder state */
    base64_encodestate es;
    /* store the number of bytes encoded by a single call */
    int cnt = 0;
    
    /*---------- START ENCODING ----------*/
    /* initialise the encoder state */
    base64_init_encodestate(&es);
    /* gather data from the input and send it to the output */
    while (1)
    {
        cnt = fread(input, sizeof(char), size, inputFile);
        if (cnt == 0)
            break;
        cnt = base64_encode_block(input, cnt, encoded, &es);
        /* output the encoded bytes to the output file */
        fwrite(encoded, sizeof(char), cnt, outputFile);
    }
    /* since we have reached the end of the input file, we know that 
       there is no more input data; finalise the encoding */
    cnt = base64_encode_blockend(encoded, &es);
    /* write the last bytes to the output file */
    fwrite(encoded, sizeof(char), cnt, outputFile);
    /*---------- STOP ENCODING  ----------*/
	
    free(encoded);
    free(input);
}


// receiver: mail address of the recipient
// subject: mail subject
// msg: content of mail body or path to the file containing mail body
// att_path: path to the attachment
void send_mail(const char* receiver, const char* subject, const char* msg, const char* att_path)
{
    const char* end_msg = "\r\n.\r\n";
    const char* host_name = "smtp.qq.com"; // TODO: Specify the mail server domain name
    const unsigned short port = 25; // SMTP server port
    const char* user = "392744757@qq.com"; // TODO: Specify the user
    const char* pass = "sjlglhyuqddibibd"; // TODO: Specify the password
    const char* from = "392744757@qq.com"; // TODO: Specify the mail address of the sender
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
            "Content-Transfer-Encoding: base64\r\n\r\n",
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

        char* content = malloc(file_size + 1); // +1 for null terminator
        if (content == NULL) {
            perror("malloc");
            fclose(msg_file);
            return NULL;
        }

        fread(content, 1, file_size, msg_file);
        content[file_size] = '\0'; // null terminator

        fclose(msg_file);

        // Send attachment
        char* encoded_content = encode_str(content);
        send(s_fd, encoded_content, strlen(encoded_content), 0);
        send(s_fd, "\r\n", 2, 0);
        free(encoded_content);
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
                "begin 644 %s\r\n"
                "%s\r\n"
                "`\r\n"
                "end\r\n",
                att_path, buffer);

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
    char* s_arg = "Mail subject";
    char* m_arg = NULL;
    char* a_arg = "attachment.zip";
    char* recipient = "2145807962@qq.com";
    // char* s_arg = NULL;
    // char* m_arg = NULL;
    // char* a_arg = NULL;
    // char* recipient = NULL;
    // const char* optstring = ":s:m:a:";
    // while ((opt = getopt(argc, argv, optstring)) != -1)
    // {
    //     switch (opt)
    //     {
    //     case 's':
    //         s_arg = optarg;
    //         break;
    //     case 'm':
    //         m_arg = optarg;
    //         break;
    //     case 'a':
    //         a_arg = optarg;
    //         break;
    //     case ':':
    //         fprintf(stderr, "Option %c needs an argument.\n", optopt);
    //         exit(EXIT_FAILURE);
    //     case '?':
    //         fprintf(stderr, "Unknown option: %c.\n", optopt);
    //         exit(EXIT_FAILURE);
    //     default:
    //         fprintf(stderr, "Unknown error.\n");
    //         exit(EXIT_FAILURE);
    //     }
    // }

    // if (optind == argc)
    // {
    //     fprintf(stderr, "Recipient not specified.\n");
    //     exit(EXIT_FAILURE);
    // }
    // else if (optind < argc - 1)
    // {
    //     fprintf(stderr, "Too many arguments.\n");
    //     exit(EXIT_FAILURE);
    // }
    // else
    // {
    //     recipient = argv[optind];
        send_mail(recipient, s_arg, m_arg, a_arg);
        exit(0);
    // }

}

