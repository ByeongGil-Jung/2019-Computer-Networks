/* 
    A simple server in the internet domain using TCP
    Usage:./server port (E.g. ./server 10000 )
*/
#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <stdbool.h>
#include <strings.h>
#include <unistd.h>

#define BUFFER_SIZE 2048

struct Header {
    char file_name[100];
    char status_code[50];
    char content_type[50];
    int content_length;
};

void error(char *msg)
{
    perror(msg);
    exit(1);
}

char* get_file_name(char *message)
{
    char *message_temp = malloc(sizeof(char) * BUFFER_SIZE);
    char *file_name;
    
    strcpy(message_temp, message);

    file_name = strtok(message_temp, "/");
    file_name = strtok(NULL, " ");
    
    return file_name;
}

char* get_content_type(char *file_name)
{
    char *file_name_temp = malloc(sizeof(char) * BUFFER_SIZE);
    char *file_format, *content_type;

    strcpy(file_name_temp, file_name);

    file_format = strtok(file_name_temp, ".");
    file_format = strtok(NULL, " ");

    // If query has not '.'
    if (file_format == NULL) {
        content_type = "text/plane";

        return content_type;
    }

    if (strcmp(file_format, "html") == 0) {
        content_type = "text/html";
    } else if (strcmp(file_format, "gif") == 0) {
        content_type = "image/gif";
    } else if (strcmp(file_format, "jpeg") == 0) {
        content_type = "image/jpeg";
    } else if (strcmp(file_format, "mp3") == 0) {
        content_type = "audio/mpeg3";
    } else if (strcmp(file_format, "pdf") == 0) {
        content_type = "application/pdf";
    } else if (strcmp(file_format, "mp4") == 0) {
        content_type = "video/mpeg";
    } else {
        content_type = "text/plane";
    }

    return content_type;
}

int check_file_name(struct Header *header)
{
    // 0 : normal | 1 : ignore | 2 : exit
    int check = 0;

    if (strcmp(header->file_name, "favicon.ico") == 0) {
        check = 1;
    } else if (strcmp(header->file_name, "exit") == 0) {
        check = 2;
    }
    
    return check;
}

void build_response(char *response_buffer, struct Header *header)
{
    int response_len = 0;
    char *alert_message;

    if (strcmp(header->status_code, "200 OK") == 0) {
        response_len += sprintf(response_buffer + response_len, "HTTP/1.1 %s\n", header->status_code);
        response_len += sprintf(response_buffer + response_len, "Content-Type: %s\n", header->content_type);
        response_len += sprintf(response_buffer + response_len, "Content-Length: %d\n", header->content_length);
        response_len += sprintf(response_buffer + response_len, "\n");
    } else if (strcmp(header->status_code, "404 Not Found") == 0) {
        alert_message = "404 Not Found !!!!!!";

        response_len += sprintf(response_buffer + response_len, "HTTP/1.1 %s\n", header->status_code);
        response_len += sprintf(response_buffer + response_len, "Content-Type: %s\n", header->content_type);
        response_len += sprintf(response_buffer + response_len, "Content-Length: %d\n", strlen(alert_message));
        response_len += sprintf(response_buffer + response_len, "\n");
        response_len += sprintf(response_buffer + response_len, alert_message);
    }
}

int main(int argc, char *argv[])
{
    socklen_t clilen;
    int server_sockfd, client_sockfd, portno;
    int file_name_trigger;
    char request_buffer[BUFFER_SIZE], response_buffer[BUFFER_SIZE];
    char *file_buffer;
    FILE *fp = NULL;
    
    struct sockaddr_in serv_addr, cli_addr;
    struct Header *header = malloc(sizeof(struct Header));
    
    int n;
    if (argc < 2) {
        fprintf(stderr,"[ERROR] There is no port provided\n");
        exit(1);
    }
    
    /*
    Create a new socket
        AF_INET: Address Domain is Internet 
        SOCK_STREAM: Socket Type is STREAM Socket 
    */
    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sockfd < 0) {
        error("[ERROR] Fail to open the socket\n");
    } else {
        printf("[INFO] Socket descriptor of server : %d\n", server_sockfd);
    }
    
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;  //for the server the IP address is always the address that the server is running on
    serv_addr.sin_port = htons(portno);  //convert from host to network byte order
    
    if (bind(server_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) //Bind the socket to the server address
        error("[ERROR] Fail to bind\n");
    
    listen(server_sockfd, 5);  // Listen for socket connections. Backlog queue (connections to wait) is 5
    
    clilen = sizeof(cli_addr);

    while (true) {
        /*
        Accept function: 
            1) Block until a new connection is established
            2) the new socket descriptor will be used for subsequent communication with the newly connected client.
        */
        client_sockfd = accept(server_sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (client_sockfd < 0) {
            error("[ERROR] Fail to accept\n");
        } else {
            printf("[INFO] Socket descriptor of client : %d\n", client_sockfd);
        }

        printf("[INFO] Client was connected ...\n");

        // After Accept
        bzero(request_buffer, BUFFER_SIZE);
        bzero(response_buffer, BUFFER_SIZE);
        
        // Read the HTTP request message
        if ((n = read(client_sockfd, request_buffer, BUFFER_SIZE)) < 0) {
            error("[ERROR] Fail to read from socket\n");
        }

        // Set the file name to header struct
        strcpy(header->file_name, get_file_name(request_buffer));

        // Checking file_name (0 : normal | 1 : ignore | 2 : exit)
        file_name_trigger = check_file_name(header);
        if (file_name_trigger == 1) {
            printf("[INFO] Ignore the 'favicon.ico'\n");
            
            close(client_sockfd);
            printf("[INFO] Client was disconnected ... (Socket descriptor : %d)\n", client_sockfd);
            continue;
        } else if (file_name_trigger == 2) {
            printf("[INFO] Let's finish ... (It's the 'exit' condition)\n");

            close(client_sockfd);
            printf("[INFO] Client was disconnected ... (Socket descriptor : %d)\n", client_sockfd);
            break;
        }

        printf("\n[ Request Header ]\n%s\n", request_buffer);

        // Set the content_type to header
        strcpy(header->content_type, get_content_type(header->file_name));

        // File handling
        fp = fopen(header->file_name, "rb");
        if (fp == NULL) {
            printf("[INFO] There is no file : \'%s\'\n", header->file_name);
            strcpy(header->status_code, "404 Not Found");

            // Build & send the error page
            build_response(response_buffer, header);

            if ((n = write(client_sockfd, response_buffer, strlen(response_buffer))) < 0)
                error("[ERROR] Writing header to socket");

            printf("[INFO] File doesn't exist\n");

            close(client_sockfd);
            printf("[INFO] Client was disconnected ... (Socket descriptor : %d)\n", client_sockfd);
            continue;
        } else {
            printf("[INFO] The file exists : \'%s\'\n", header->file_name);
            strcpy(header->status_code, "200 OK");
        }

        fseek(fp, 0, SEEK_END);
        header->content_length = ftell(fp);
        file_buffer = malloc(sizeof(char) * header->content_length);
        rewind(fp);
        
        fread(file_buffer, 1, header->content_length, fp);

        // Build the response buffer
        build_response(response_buffer, header);

        printf("\n[ Response Header ]\n");
        printf("%s\n", response_buffer);

        // Send the message with HTTP protocol to client using socket
        if ((n = write(client_sockfd, response_buffer, strlen(response_buffer))) < 0)
            error("[ERROR] Writing header to socket");
        if ((n = write(client_sockfd, file_buffer, header->content_length)) < 0)
            error("[ERROR] Writing body to socket");
        
        // Release memories
        fclose(fp);

        close(client_sockfd);
        printf("[INFO] Client was disconnected ... (Socket descriptor : %d)\n", client_sockfd);
    }

    free(file_buffer);
    free(header);

    // Disconnect server sockets
    close(server_sockfd);
    printf("[INFO] Server was disconnected ... (Socket descriptor : %d)\n", server_sockfd);
    
    return 0; 
}
