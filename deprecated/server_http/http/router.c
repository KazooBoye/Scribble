#include "router.h"
#include "mime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

#define WEBUI_DIR "./webui"

// Sanitize path to prevent directory traversal
int sanitize_path(const char* request_path, char* safe_path, int safe_path_size) {
    if (!request_path || !safe_path) return -1;
    
    // Remove query string
    char clean_path[PATH_MAX];
    const char* query = strchr(request_path, '?');
    if (query) {
        int len = query - request_path;
        if (len >= PATH_MAX) len = PATH_MAX - 1;
        strncpy(clean_path, request_path, len);
        clean_path[len] = '\0';
    } else {
        strncpy(clean_path, request_path, PATH_MAX - 1);
        clean_path[PATH_MAX - 1] = '\0';
    }
    
    // Default to index.html if path is /
    if (strcmp(clean_path, "/") == 0) {
        snprintf(safe_path, safe_path_size, "%s/index.html", WEBUI_DIR);
    } else {
        snprintf(safe_path, safe_path_size, "%s%s", WEBUI_DIR, clean_path);
    }
    
    // Check for directory traversal attempts
    if (strstr(safe_path, "..") != NULL) {
        return -1;
    }
    
    return 0;
}

int route_request(const char* request, char** response, int* response_len) {
    // Parse HTTP method and path
    char method[16], path[512], version[16];
    if (sscanf(request, "%15s %511s %15s", method, path, version) != 3) {
        *response = strdup("HTTP/1.1 400 Bad Request\r\n\r\n");
        *response_len = strlen(*response);
        return 400;
    }
    
    // Only support GET
    if (strcmp(method, "GET") != 0) {
        *response = strdup("HTTP/1.1 405 Method Not Allowed\r\n\r\n");
        *response_len = strlen(*response);
        return 405;
    }
    
    // Sanitize and get safe path
    char file_path[PATH_MAX];
    if (sanitize_path(path, file_path, sizeof(file_path)) < 0) {
        *response = strdup("HTTP/1.1 403 Forbidden\r\n\r\n");
        *response_len = strlen(*response);
        return 403;
    }
    
    // Check if file exists
    struct stat st;
    if (stat(file_path, &st) < 0 || !S_ISREG(st.st_mode)) {
        *response = strdup("HTTP/1.1 404 Not Found\r\n\r\n");
        *response_len = strlen(*response);
        return 404;
    }
    
    // Read file
    FILE* f = fopen(file_path, "rb");
    if (!f) {
        *response = strdup("HTTP/1.1 500 Internal Server Error\r\n\r\n");
        *response_len = strlen(*response);
        return 500;
    }
    
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char* file_content = malloc(file_size);
    if (!file_content) {
        fclose(f);
        *response = strdup("HTTP/1.1 500 Internal Server Error\r\n\r\n");
        *response_len = strlen(*response);
        return 500;
    }
    
    fread(file_content, 1, file_size, f);
    fclose(f);
    
    // Get MIME type
    const char* mime_type = get_mime_type(file_path);
    
    // Build response
    char header[512];
    int header_len = snprintf(header, sizeof(header),
                              "HTTP/1.1 200 OK\r\n"
                              "Content-Type: %s\r\n"
                              "Content-Length: %ld\r\n"
                              "Connection: keep-alive\r\n"
                              "\r\n",
                              mime_type, file_size);
    
    *response_len = header_len + file_size;
    *response = malloc(*response_len);
    if (!*response) {
        free(file_content);
        return 500;
    }
    
    memcpy(*response, header, header_len);
    memcpy(*response + header_len, file_content, file_size);
    
    free(file_content);
    return 200;
}
