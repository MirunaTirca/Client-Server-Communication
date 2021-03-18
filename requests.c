#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson.h"

char *compute_delete_request (char *host, char *url, char *query_params,
                            char **cookies, int cookies_count, char *auth_token, int size_token)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // write the method name, URL, request params (if any) and protocol type
    if (query_params != NULL) {
        sprintf(line, "DELETE %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "DELETE %s HTTP/1.1", url);
    }
    compute_message(message, line);

    // add the host
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    

    // add headers and/or cookies, according to the protocol format
    if (cookies != NULL) {
       memset(line, 0, LINELEN);
       strcat(line, "Cookie: ");
       for(int i = 0; i < cookies_count - 1;i++) {
            strcat(line, cookies[i]);
            strcat(line, "; ");
       }
       strcat(line, cookies[cookies_count - 1]);
       compute_message(message, line);
    }

    if(size_token != 0) {
        sprintf(line, "Authorization: Bearer %s", auth_token);
        compute_message(message, line);
    }


    //add final new line
    compute_message(message, "");
    return message;
}

char *compute_get_request(char *host, char *url, char *query_params,
                            char **cookies, int cookies_count, char *auth_token, int size_token)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    //write the method name, URL, request params (if any) and protocol type
    if (query_params != NULL) {
        sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "GET %s HTTP/1.1", url);
    }
    compute_message(message, line);

    // add the host
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    

    // add headers and/or cookies, according to the protocol format
    if (cookies != NULL) {
       memset(line, 0, LINELEN);
       strcat(line, "Cookie: ");
       for(int i = 0; i < cookies_count - 1;i++) {
            strcat(line, cookies[i]);
            strcat(line, "; ");
       }
       strcat(line, cookies[cookies_count - 1]);
       compute_message(message, line);
    }

    if(size_token != 0) {
        sprintf(line, "Authorization: Bearer %s", auth_token);
        compute_message(message, line);
    }


    // add final new line
    compute_message(message, "");
    return message;
}
char *compute_post_request(char *host, char *url, char* content_type, JSON_Value *json,
                            int body_data_fields_count, char **cookies, int cookies_count, char* auth_token, int size_token)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));
    char *body_data_buffer = calloc(LINELEN, sizeof(char));

    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);

    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);

    if(size_token != 0) {
        sprintf(line, "Authorization: Bearer %s", auth_token);
        compute_message(message, line);
    }

    if (json != NULL)
        strcat(body_data_buffer, json_serialize_to_string(json) );

    sprintf(line, "Content-Length: %ld", strlen(body_data_buffer));
    compute_message(message, line);
    if (cookies != NULL) {
    }

    compute_message(message, "");

    memset(line, 0, LINELEN);
    compute_message(message, body_data_buffer);

    free(line);
    return message;
}
