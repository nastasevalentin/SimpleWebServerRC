#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <dirent.h>

#define MAX_BUFFER 1024
#define PORT 80

void *connection_handler(void *socket_desc) {
    int sock = *(int*)socket_desc;
    char buffer[MAX_BUFFER];
    char filename[MAX_BUFFER] = "index.html";

    ssize_t len = read(sock, buffer, MAX_BUFFER-1);
    if(len < 0) {
        perror("Failed to read from socket");
        return 0;
    }
    buffer[len] = '\0';

    char req_file[MAX_BUFFER];
    sscanf(buffer, "GET /%s ", req_file);

    if(strlen(req_file) > 0 && strcmp(req_file, "favicon.ico") != 0) {
        strncpy(filename, req_file, MAX_BUFFER);
    }

    if(access(filename, F_OK) != 0) {
        printf("File not found: %s\n", filename);
        char *response = "HTTP/1.0 404 Not Found\n"
                         "Content-Type: text/html\n\n"
                         "<html><body><h1>404 Not Found</h1><p>The requested file was not found on this server.</p></body></html>";
        write(sock, response, strlen(response));
    } else {
        char *content_type;
        if(strstr(filename, ".html") != NULL) {
            content_type = "text/html";
        } else if(strstr(filename, ".jpg") != NULL) {
            content_type = "image/jpeg";
        } else if(strstr(filename, ".txt") != NULL) {
            content_type = "text/plain";
        } else {
            content_type = "application/octet-stream";
        }

        FILE *fp = fopen(filename, "rb");
        if(fp == NULL) {
            perror("Failed to open file");
            close(sock);
            return 0;
        }

        char header[MAX_BUFFER];
        sprintf(header, "HTTP/1.0 200 OK\nContent-Type: %s\n\n", content_type);
        write(sock, header, strlen(header));

        char buffer[MAX_BUFFER];
        size_t bytes;
        while((bytes = fread(buffer, sizeof(char), MAX_BUFFER, fp)) > 0) {
            write(sock, buffer, bytes);
        }

        fclose(fp);
    }

    close(sock);

    return 0;
}

int main(int argc, char *argv[]) {
    int socket_desc, client_sock, c;
    struct sockaddr_in server, client;
    pthread_t thread_id;

    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        printf("Could not create socket");
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    if(bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("bind failed. Error");
        return 1;
    }

    listen(socket_desc, 3);

    while((client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c))) {
        if(pthread_create(&thread_id, NULL, connection_handler, (void*) &client_sock) < 0) {
            perror("could not create thread");
            return 1;
        }
    }

    if (client_sock < 0) {
        perror("accept failed");
        return 1;
    }

    return 0;
}