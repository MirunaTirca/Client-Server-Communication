#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson.h"

int main(int argc, char *argv[])
{
	char *session_cookie = NULL;
	char buffer[BUFLEN];

	char *message;
	char *response;
	int sockfd;
	char *cookies[1];
	sockfd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0); //deschid conexiunea tcp

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

	FD_SET(sockfd, &read_fds);
	FD_SET(0, &read_fds);
	fdmax = sockfd;

	//valori de control - pentru login si pentru biblioteci
	int okSesiune = 0;
	int okToken = 0;

	char *token = NULL;
	char *FIN = NULL;

	while (1) {		
		sockfd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0); //deschid conexiunea tcp?
		tmp_fds = read_fds;
		select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		if(FD_ISSET(0, &tmp_fds)) {
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN - 1, stdin);

			if (strncmp(buffer, "register", 8) == 0) {

				//promturi pentru username si parola
				printf("username = ");
				memset(buffer, 0, BUFLEN);
				fgets(buffer, BUFLEN - 1, stdin);

				char user[100];
				strcpy(user,strtok(buffer, "\n"));
				char aux[1500];
			    strcpy(aux,"{\"username\": \"");
			    strcat(aux, user);

				printf("password = ");
				memset(buffer, 0, BUFLEN);
				fgets(buffer, BUFLEN - 1, stdin);
				char *passw = strtok(buffer, "\n");

				strcat(aux, "\", \"password\": \"");
			    strcat(aux, passw);
			    strcat(aux, "\"}");

			    //in aux este mesajul cu user si parola in format json
			    //in functiile de request trimit un JSON_Value format cu
			    //functiile din parson.h

			    JSON_Value *send = NULL;
			    send = json_parse_string(aux);
			    int ok  = json_serialization_size_pretty(send);

			    //formarea si trimiterea mesajului
			    message = compute_post_request("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com", "/api/v1/tema/auth/register", "application/json", send, 1, NULL, 0, NULL, 0); 
			    send_to_server(sockfd, message);
			    //raspunsul serverului
			    response = receive_from_server(sockfd);
			    if(strstr(response, "20") != NULL ) {
			    	puts("\nRegister completed\n\n");
			    }
			    else {
			    	puts("\nError with register\n\n");
			    }
			    close_connection(sockfd);
			    continue;
			}
			else if (strncmp(buffer, "exit", 4) == 0){
				close_connection(sockfd);
				break;
			}
			else if (strncmp(buffer, "login", 5) == 0 && okSesiune == 0) {

				//prompt username si parola
				printf("username = ");
				memset(buffer, 0, BUFLEN);
				fgets(buffer, BUFLEN - 1, stdin);
				char user[100];
				strcpy(user,strtok(buffer, "\n"));

				char aux[1500];
			    strcpy(aux,"{\"username\": \"");
			    strcat(aux, user);

				printf("password = ");
				memset(buffer, 0, BUFLEN);
				fgets(buffer, BUFLEN - 1, stdin);
				char *passw = strtok(buffer, "\n");

				strcat(aux, "\", \"password\": \"");
			    strcat(aux, passw);
			    strcat(aux, "\"}");

			    //string-ul aux este transformat intr-un JSON_Value
			    JSON_Value *send = NULL;
			    send = json_parse_string(aux);
			    int ok  = json_serialization_size_pretty(send);

			    message = compute_post_request("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com", "/api/v1/tema/auth/login", "application/json", send, 1, NULL, 0, NULL, 0);	    
			    send_to_server(sockfd, message);
			    response = receive_from_server(sockfd);
			    //daca a login-ul fost ok
			    if (strstr(response, "OK") != NULL) {
			    	//inseamna ca a inceput o sesiune
			    	//extrag cookie-ul
			    	okSesiune = 1;
				    char* setCookie = strstr(response, "Set-Cookie: connect.sid=");
				    char *f = strtok(setCookie, " ");
				    session_cookie = strtok(NULL, "; ");
				    puts("\nLogin completed\n\n");
				}
				else {
					puts("\nLogin failed\n\n");
				}
			    close_connection(sockfd);
			    continue;
			}
			else if (strncmp (buffer, "enter_library", 13) == 0) {

				if (okSesiune == 0) {
					puts("\nYou are not logged in\n\n");
			    	close_connection(sockfd);
			    	continue;
				}
				okToken = 1;
				cookies[0] =  session_cookie;
			    message = compute_get_request("3.8.116.10", "/api/v1/tema/library/access", NULL, cookies, 1, NULL, 0);
			    send_to_server(sockfd, message);
			    response = receive_from_server(sockfd);
				if (strstr(response, "OK") != NULL) {
				    puts("\nYou entered the library\n\n");
				}
				else {
					puts("\nEnter library failed\n\n");
				}
			    close_connection(sockfd);
			    //extrag token din raspunsul de la server
			    token = strstr(response, "\"token\":\"");
			    char *auth_token = strtok(token, ":");
			    FIN = strtok(NULL, "\"");
			    continue;
			}
			else if (strncmp(buffer, "get_books", 9) == 0) {
				if (okSesiune == 0) {
					printf("\nYou are not logged in\n\n");
			    	continue;
				}
				if (okToken == 0) {
					printf("\nYou don't have library access\n\n");
				    continue;
				}
				//trimit cererea de get, cu tot cu cookie si cu token
				cookies[0] = session_cookie;
			    message = compute_get_request("3.8.116.10", "/api/v1/tema/library/books", NULL, cookies, 1, FIN, 1);
			    send_to_server(sockfd, message);
			    response = receive_from_server(sockfd);
			    char *list_books = strstr(response, "[");
			    if(strstr(response, "OK") != NULL){
			    	puts("\n");
			    	puts(list_books);
			    	puts("\n\n");
			    }
			    else {
			    	puts("\nGet books failed\n\n");
			    }
			    close_connection(sockfd);
			    continue;
			}
			else if (strncmp (buffer, "add_book", 8) == 0) {
				if (okSesiune == 0) {
			    	puts("\nYou are not logged in \n\n");
			    	close_connection(sockfd);
			    	continue;
				}
				if (okToken == 0) {
					printf("\nYou don't have library access\n\n");
			    	close_connection(sockfd);
				    continue;
				}
				char title[1000], author[1000], genre[1000], publisher[1000], page_count[1000];

				//prompturi pentru utilizator
				printf("title=");
				memset(buffer, 0, BUFLEN);
				fgets(buffer, BUFLEN - 1, stdin);
				strcpy(title, strtok(buffer, "\n"));

				char aux[1500];
				memset (aux, 0, sizeof(aux));
			    strcpy(aux,"{\n\"title\": \"");
			    strcat(aux, title);

			    printf("author=");
				memset(buffer, 0, BUFLEN);
				fgets(buffer, BUFLEN - 1, stdin);
				strcpy(author, strtok(buffer, "\n"));

			    strcat(aux,"\",\n\"author\": \"");
			    strcat(aux, author);

			    printf("genre=");
				memset(buffer, 0, BUFLEN);
				fgets(buffer, BUFLEN - 1, stdin);
				strcpy(genre, strtok(buffer, "\n"));

			    strcat(aux,"\",\n\"genre\": \"");
			    strcat(aux, genre);

			    printf("publisher=");
				memset(buffer, 0, BUFLEN);
				fgets(buffer, BUFLEN - 1, stdin);
				strcpy(publisher, strtok(buffer, "\n"));

			    strcat(aux,"\",\n\"publisher\": \"");
			    strcat(aux, publisher);

			    printf("page_count=");
				memset(buffer, 0, BUFLEN);
				fgets(buffer, BUFLEN - 1, stdin);
				strcpy(page_count, strtok(buffer, "\n"));

				//in aux este string-ul in format JSON cu toate datele
			    strcat(aux,"\",\n\"page_count\": \"");
			    strcat(aux, page_count);

			    //verificare daca page_count-ul este valid
			    int pg = atoi(page_count);
			    if(pg <= 0) {
			    	puts("\nPage count invalid\n\n");
			    	close_connection(sockfd);
			    	continue;
			    }

			    strcat(aux, "\"\n}");
			    //datele sunt valide, trimit cererea catre server
			    JSON_Value *send = NULL;
			    send = json_parse_string(aux);
			    int ok  = json_serialization_size_pretty(send);
				cookies[0] =  session_cookie;
			    message = compute_post_request("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com", "/api/v1/tema/library/books", "application/json", send, 1, cookies, 1, FIN, 1);		    
			    send_to_server(sockfd, message);
			    response = receive_from_server(sockfd);
			    if (strstr(response, "OK") != NULL){
			    	puts("\nBook added successfully\n\n");
			    }
			    else {
			    	puts("\nError with add book\n\n");
			    }
			    close_connection(sockfd);
			    continue;
			}
			else if (strncmp (buffer, "get_book", 8) == 0) {
				if (okSesiune == 0) {
					puts("\nYou are not logged in\n\n");
			    	close_connection(sockfd);
			    	continue;
				}
				if (okToken == 0) {
				    puts("\nYou don't have library access\n\n");
			    	close_connection(sockfd);
				    continue;
				}
				//prompt pt id
				printf("id=");
				memset(buffer, 0, BUFLEN);
				fgets(buffer, BUFLEN - 1, stdin);
				char *id = strtok(buffer, "\n");

				//verificare id (sa fie nr > 0)
			    int ID = atoi(id);
			    if(ID <= 0) {
				    puts("\nInvalid id\n\n");
			    	close_connection(sockfd);
			    	continue;
			    }
			    cookies[0] = session_cookie;
				char req[1000];
				memset(req, 0, 1000);
				strcpy(req, "/api/v1/tema/library/books/");
				strcat(req, id);
			    message = compute_get_request("3.8.116.10", req, NULL, cookies, 1, FIN, 1);
			    send_to_server(sockfd, message);
			    response = receive_from_server(sockfd);
			    if(strstr(response, "OK") != NULL) {
			    	char *ret_book = strstr(response, "[");
			    	puts("\n");
			    	puts(ret_book);
			    	puts("\n\n");
			    }
			    else {
			    	puts("\nCould not get book\n\n");
			    }
			    close_connection(sockfd);
			    continue;
			}
			else if (strncmp (buffer, "delete_book", 11) == 0) {
				if (okSesiune == 0) {
			    	puts("\nYou are not logged in\n\n");
			    	close_connection(sockfd);
			    	continue;
				}
				if (okToken == 0) {
				    puts("\nYou don't have library access\n\n");
			    	close_connection(sockfd);
				    continue;
				}
				//promt pt utilizator
				printf("id=");
				memset(buffer, 0, BUFLEN);
				fgets(buffer, BUFLEN - 1, stdin);
				char *id = strtok(buffer, "\n");

				//verificare id sa fie nr
			    int ID = atoi(id);
			    if(ID <= 0) {
				    puts("\nInvalid id\n\n");
			    	close_connection(sockfd);
			    	continue;
			    }
			    //utilizatorul a introdus un id care poate fi valid
			    //trimit cererea de delete la server
			    cookies[0] = session_cookie;
				char req[1000];
				memset(req, 0, 1000);
				strcpy(req, "/api/v1/tema/library/books/");
				strcat(req, id);
			    message = compute_delete_request("3.8.116.10", req, NULL, cookies, 1, FIN, 1);
			    send_to_server(sockfd, message);
			    response = receive_from_server(sockfd);
			    if(strstr(response, "OK") != NULL) {
			    	puts("\nBook deleted successfully\n\n");
			    }
			    else if (strstr(response, "Not Found") != NULL) {
			    	puts("\nBook not found\n\n");
			    }
			    else {
			    	puts("\nError, could not delete book\n\n");
			    }
			    close_connection(sockfd);
			    continue;
			}
			else if (strncmp(buffer, "logout", 6) == 0) {
				if (okSesiune == 0) {
			    	puts("\nYou are not logged in\n\n");
			    	close_connection(sockfd);
			    	continue;
				}

				okSesiune = 0; //sa pot sa mai dau login dupa
				okToken = 0;

				cookies[0] =  session_cookie;
				message = compute_get_request("3.8.116.10","/api/v1/tema/auth/logout", NULL, cookies, 1, FIN, 1);
			    send_to_server(sockfd, message);
			    response = receive_from_server(sockfd);
			    if (strstr(response, "OK") != NULL) {
			    	puts("\nYou are now logged out\n\n");
			    }
			    else {
			    	puts("\nError with logout\n\n");
			    }
			    close_connection(sockfd);
			    continue;
			}
			else {
				puts("\nInvalid command\n\n");
			}
		}
	}
	return 0;
}
