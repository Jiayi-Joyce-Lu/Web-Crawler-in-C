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
#include "functions.h"

#define PORTNO 80
#define URL_SIZE 1100
#define REQUEST_SIZE 600
#define BUFFER_SIZE 100000
#define LIST_SIZE 100
#define URL_LENGTH 1000
#define URL_PREFIX_LEN 7

/* split the url into host part and post part */
void get_host(char* url, char *host, char *post) {
    
    int start = URL_PREFIX_LEN, temp = 0;

    // find the nearest '/' after "http://"
    for (int i = strlen(url)-1; i > start; i--) {
        if (url[i] == '/') {
            temp = i;
        }
    }

    if (temp == 0) {
        // there is no '/' in url, which means there is no post part
        strncpy(host, url+start, strlen(url)-start);
        host[strlen(url) - start] = '\0';
        strcpy(post,"/");

    } else {
        // stuff between "http://" and '/' will be the host
        strncpy(host, url + start, temp - start);
        // stuff after '/' will be the post
        strncpy(post, url + temp, strlen(url) - temp);
    }

}

/*
 * check if the content type is text/html,
 * if so get the status number from the response,
 * otherwise return 0
 */
int check_head(char *buffer) {
    regmatch_t pmatch;
    regex_t reg;
    const char *pattern = "text/html";
    char code[3];

    regcomp(&reg, pattern, REG_EXTENDED);
    if (regexec(&reg, buffer, 1, &pmatch, 0) != REG_NOMATCH && pmatch.rm_so != -1) {
        strncpy(code, buffer + 9, 3);
        regfree(&reg);
        return atoi(code);
    }
    regfree(&reg);
    return 0;
}

/* check whether the url already exists in the list */
int check_exist(char *url, char *http_list[], int count) {
    for (int i = 0; i < count; i++) {
        if (strcmp(url, http_list[i]) == 0) {
            return 0;
        }
    }
    return 1;
}

/*
 * check whether the given url is valid,
 * firstly, has the same host with the original one
 * sencondly, has the length shorter than 1000
 */
int check_validity(char *origin_host, char *url) {
    /*
     * for example,
     * web1.comp30023, web2.comp30023, abcd.comp30023
     * has the same host,
     * so only need to get the part after '.'
     */
    char target[URL_SIZE];
    const char ch = '.';
    strcpy(target, strrchr(origin_host, ch)); //eg. ".comp30023"
    int len_host = strlen(origin_host);
    int len_target = strlen(target);
    // find the index of '.' in url
    int index = URL_PREFIX_LEN + len_host - len_target;

    if (memcmp(target, url+index, len_target) == 0 && strlen(url) < URL_LENGTH) {
        return 1;
    }
    return 0;
}

/* make up the relative urls */
int makeup_url(char *origin_url, char *url, char *origin_host) {
    char temp[URL_SIZE];
    memset(temp, 0, sizeof(temp));

    // urls contain '.', '/', '?', '#' and '%' should be ignored
    for (int i = 0; i < strlen(url) - 1; i++) {
        if ((url[i] == '.' && url[i+1] == '/') || url[i] == '?' \
            || url[i] == '#' || url[i] == '%') {
            return 0;
        }
    }

    // url appear like '/' should be ignored
    if (strlen(url) == 1 && url[0] == '/') {
        return 0;
    }
    
    if (url[0] == '/' && url[1] == '/') {
        // url starts with '//' should be added 'http:' at the front
        sprintf(temp, "http:");
        strcat(temp, url);
        strcpy(url, temp);
    } else if (url[0] == '/' && url[1] != '/') {
        // url starts with only one '/',
        // should be added original host at the front
        strcat(origin_host, url);
    } else if (url[0] == 'h' && url[1] == 't' && url[2] == 't' \
        && url[3] == 'p' && url[4] == ':' && url[5] == '/' && url[6] == '/') {
        // url starts with 'http://' is absolute, no more action needed

    } else {
        // the rest will be like "a.html",
        // should add this at the end of the original url
        int index = strlen(origin_url) - 1;
        while (origin_url[index] != '/') {
            index--;
        }
        strncpy(temp, origin_url, index+1);
        strcat(temp, url);
        strcpy(url, temp);

    }

    // after make up the url, check whether it is valid
    return check_validity(origin_host, url);

}

/*
 * get the url from response using regular expression
 * this function referenced the regular expression usage from:
 * https://ask.csdn.net/questions/185883;
 *
 * the regular expression pattern referenced from:
 * https://stackoverflow.com/questions/1960461/convert-plain-text-urls-into-html-hyperlinks
 */
int get_url(char *current_url, char *buffer, char *http_list[], char *origin_host, int count) {
    // split the response into each line
    char *one_line;
    one_line = strtok(buffer, "\n");

    regmatch_t pm1, pm2;
    regex_t reg1, reg2;
    const char *pattern1 = "(href|HREF) *= *\"[-A-Za-z0-9+&@/=~_|!:,.;]*\"";
    const char *pattern2 = "\"[-A-Za-z0-9+&@/=~_|!:,.;]*\"";

    char href[URL_SIZE], http[URL_SIZE];

    if (regcomp(&reg1, pattern1, REG_EXTENDED)!=0) {
        printf("error!");
    }

    if (regcomp(&reg2, pattern2, REG_EXTENDED)!=0) {
        printf("error!");
    }

    // use while loop to go through every line to find url
    while (one_line != NULL) {
        // use index to record the place of the target url
        int index = 0;
        while (index < strlen(one_line)) {
            // to find <a ... href = "..."> first
            if (regexec(&reg1, one_line + index, 1, &pm1, 0) != REG_NOMATCH && pm1.rm_so != -1 ) {
                memset(href, 0, URL_SIZE);
                strncpy(href, one_line + index + pm1.rm_so, pm1.rm_eo - pm1.rm_so);

                // to find the url inside double quotes
                if (regexec(&reg2, href, 1, &pm2, 0) != REG_NOMATCH && pm2.rm_so != -1) {
                    memset(http, 0, URL_SIZE);
                    strncpy(http, href + pm2.rm_so+1, pm2.rm_eo - pm2.rm_so-2);

                    // after getting the url, make it up and check its validity
                    // add it to the list only if it doesn't exist already
                    if (makeup_url(current_url, http, origin_host)) {
                        // check if url already exists
                        if (check_exist(http, http_list, count)) {

                            http_list[count] = (char *)malloc(URL_SIZE * sizeof(char));
                            assert(http_list[count]);
                            strcpy(http_list[count], http);
                            count++;

                            if (count == LIST_SIZE + 1) {
                                return count;
                            }

                        }
                    }
                }
            }
            // get to the next place of the url
            index += pm1.rm_eo;
        }
        // get to the next line of response
        one_line = strtok(NULL, "\n");
    }

    regfree(&reg1);
    regfree(&reg2);
    return count;

}

/*
 * form the request,
 * if the status number of page is 401,
 * add the authorization at the end of request
 */
void form_request(char *request, char *host, char *post, int head) {
    sprintf(request, "GET ");
    strcat(request, post);
    strcat(request, " HTTP/1.1\r\n");
    strcat(request, "Host: ");
    strcat(request, host);
    strcat(request, "\r\n");
    strcat(request, "Accept: */*\r\n");
    strcat(request, "User-Agent: jllu1\r\n");
    strcat(request, "Connection: close\r\n");
    if (head == 401) {
        strcat(request, "Authorization: Basic amxsdTE6cGFzc3dvcmQ=\r\n\r\n");
    } else {
        strcat(request, "\r\n\r\n");
    }
}

/*
 * build the socket connection,
 * this function referenced from COMP30023 lab4 source code
 */
int connect_socket(char *request, char *buffer, char *host, struct hostent *server) {
    int sockfd;
    struct sockaddr_in serv_addr;

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy(server->h_addr_list[0], (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(PORTNO);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    send(sockfd, request, strlen(request), 0);
    recv(sockfd, buffer, BUFFER_SIZE, 0);

    close(sockfd);

    // return the status number from the response
    return check_head(buffer);
}
