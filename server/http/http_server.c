#include "http_server.h"
#include "router.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <errno.h>

static int http_server_fd = -1;
static pthread_t http_thread;
static volatile bool http_running = false;

typedef struct {
    int client_fd;
} HttpClientData;

void* handle_http_client(void* arg) {
    HttpClientData* data = (HttpClientData*)arg;
    int client_fd = data->client_fd;
    free(data);
    
    char request[8192];
    int bytes_read = recv(client_fd, request, sizeof(request) - 1, 0);
    
    if (bytes_read > 0) {
        request[bytes_read] = '\0';
        
        char* response = NULL;
        int response_len = 0;
        
        int status = route_request(request, &response, &response_len);
        
        if (response) {
            send(client_fd, response, response_len, 0);
            free(response);
        }
        
        printf("[HTTP] %d - %s\n", status, request);
    }
    
    close(client_fd);
    return NULL;
}

void* http_server_thread(void* arg) {
    (void)arg;
    
    while (http_running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(http_server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (http_running) {
                perror("HTTP accept failed");
            }
            continue;
        }
        
        // Create thread for each HTTP request
        HttpClientData* data = malloc(sizeof(HttpClientData));
        data->client_fd = client_fd;
        
        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_http_client, data) != 0) {
            perror("Failed to create HTTP client thread");
            close(client_fd);
            free(data);
        } else {
            pthread_detach(thread);
        }
    }
    
    return NULL;
}

int http_server_start(int port) {
    http_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (http_server_fd < 0) {
        perror("Failed to create HTTP socket");
        return -1;
    }
    
    int opt = 1;
    setsockopt(http_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(http_server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Failed to bind HTTP socket");
        close(http_server_fd);
        return -1;
    }
    
    if (listen(http_server_fd, 10) < 0) {
        perror("Failed to listen on HTTP socket");
        close(http_server_fd);
        return -1;
    }
    
    http_running = true;
    
    if (pthread_create(&http_thread, NULL, http_server_thread, NULL) != 0) {
        perror("Failed to create HTTP server thread");
        close(http_server_fd);
        return -1;
    }
    
    printf("[HTTP] Server started on port %d\n", port);
    return 0;
}

void http_server_stop() {
    http_running = false;
    if (http_server_fd >= 0) {
        close(http_server_fd);
        http_server_fd = -1;
    }
    pthread_join(http_thread, NULL);
    printf("[HTTP] Server stopped\n");
}
