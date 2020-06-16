/* Program written by Jiayi Lu (962535)*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <assert.h>
#include <regex.h>

#define PORTNO 80
#define URL_SIZE 1100
#define REQUEST_SIZE 600
#define BUFFER_SIZE 100000
#define LIST_SIZE 100
#define URL_LENGTH 1000
#define URL_PREFIX_LEN 7

void get_host(char* url, char *host, char *post);
int check_head(char *buffer);
int check_exist(char *url, char *http_list[], int count);
int check_validity(char *origin_host, char *url);
int makeup_url(char *origin_url, char *url, char *origin_host);
int get_url(char *current_url, char *buffer, char *http_list[], char *origin_host, int count);
void form_request(char *request, char *host, char *post, int head);
int connect_socket(char *request, char *buffer, char *host, struct hostent *server);
