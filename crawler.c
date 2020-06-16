/* Program written by Jiayi Lu (962535)*/

/*
 * This program writes to crawl a web site.
 * Use socket programming to build connection
 * with server, then used regular expression
 * to get the url from the HTTP response,
 * and then recursively fetch URLs.
 *
 * Some parts of code have referenced some source
 * codes from the web, which all indicated the
 * source in the comments.
 * */

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
#include "functions.h"

#define PORTNO 80
#define URL_SIZE 1100
#define REQUEST_SIZE 600
#define BUFFER_SIZE 100000
#define LIST_SIZE 100
#define URL_LENGTH 1000
#define URL_PREFIX_LEN 7


int main (int argc, char *argv[]) {

    struct hostent *server;

    char origin_host[URL_SIZE];
    char host[URL_SIZE];
    char post[URL_SIZE];
    char request[REQUEST_SIZE];
    char buffer[BUFFER_SIZE];

    // create a list to store the urls which need to fetch,
    // because we only need to fetch 100 distinct pages,
    // so set the size of list as 100
    char *list[LIST_SIZE];
    memset(list, 0, sizeof(list));
    list[0] = argv[1];

    // set two int variables,
    // one to count the fetched pages,
    // another to count the parsed urls
    int current = 0, count_url = 1;

    // get the original host from the stdin url,
    // will use this to check the validity of the url
    get_host(list[0], origin_host, post);

    // go through and fetch every url in the list
    while (list[current]!= NULL) {

        // print the fetched url
        if ( current == LIST_SIZE) {
            break;
        } else {

            printf("%s\n", list[current]);
            memset(host, 0, URL_SIZE);
            memset(post, 0, URL_SIZE);
            memset(request, 0, REQUEST_SIZE);
            memset(buffer, 0, BUFFER_SIZE);
            get_host(list[current],host,post);
            form_request(request, host, post, 200);
        
            server = gethostbyname(host);
            
            // connect the socket first to check the status
            int status = connect_socket(request, buffer, host,server);

            // deal with different status number first
            if (status == 200) {
                // 200 no need to do any more actions
            } else if (status == 404 || status == 410 || status == 414) {
                // 404, 410, 414 just ignore and move to the next url to fetch
                    current++;
                    continue;
            } else if (status == 503 || status == 504) {
                // 503 and 504 reconnect the socket
                    memset(buffer, 0, BUFFER_SIZE);
                    status = connect_socket(request, buffer, host,server);
            } else if (status == 401) {
                // 401 needs to apply a new request to add authorization
                memset(request, 0, REQUEST_SIZE);
                memset(buffer, 0, BUFFER_SIZE);
                form_request(request, host, post, 401);
                status = connect_socket(request, buffer, host,server);
            } else {
                // other situations will contain 301 or 0;
                // 0 indicates that content type is not text/html;
                // 301 needs to redirect to another url

                // first get the status number
                char ch[3];
                strncpy(ch, buffer + 9, 3);
                // only fetch when it's 301
                if (atoi(ch)==301) {
                    char buf[BUFFER_SIZE];
                    char tag[10];
                    // first find the given url in the response
                    strcpy(tag,"Location:");
                    strcpy(buf, strstr(buffer, tag) + 10);
                    //printf("buf: %s\n", buf);
                    int i = 0;
                    while (buf[i] != '\n') {
                        i++;
                    }

                    // change the current one to the redirected one
                    strncpy(list[current], buf, i-1);

                    memset(host, 0, URL_SIZE);
                    memset(post, 0, URL_SIZE);
                    memset(request, 0, REQUEST_SIZE);
                    memset(buffer, 0, BUFFER_SIZE);

                    get_host(list[current],host,post);
                    server = gethostbyname(host);
                    int head = check_head(buffer);
                    form_request(request, host, post, head);
                    status = connect_socket(request, buffer, host,server);
                        
                } else {
                    // if it's 0, just ignore
                    // and move to the next url to fetch
                    current++;
                    continue;
                }
                /*
                 * PS. I have to admit that there are some better way
                 * to deal with different status numbers,
                 * however, I completed these tasks following the order of
                 * diiferent level of extensions, and they have
                 * different requirements.
                 * I did 301 at last, and it's difficult to change the
                 * whole logic, so I used this "silly" way
                 * to achieve the advanced extension.
                 */

            }

            // after dealing the status, let's start fetch!
            count_url = get_url(list[current], buffer,list,origin_host, count_url);
            current++;

        }
    }

/* free the list */
for (int i = 1; i < LIST_SIZE; i++) {
    free(list[i]);
}

return 0;
}



