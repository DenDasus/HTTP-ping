#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include "string.h"
#include <arpa/inet.h>
#include <time.h>
#include <poll.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include "utils.h"

#define ERR_OK 0
#define ERR_OTHER 1
#define ERR_NO_DNS_RECORDS_FOUND 2

typedef struct element_t {
    struct element_t* nextElem;
    char* site;
    struct addrinfo* addrs;
    unsigned short* measArray;
    int err;
} element;

element* elemList = NULL;
int elemCount = 0;

static const char* optString = "c:d:i:h?";
const char* inputFileName = NULL;
unsigned short measuresCount = 5;
unsigned short measDelay_ms = 1000;

void PrintUsage() {
    printf("HTTP-ping utility. Usage:\n\r");
    printf("http-ping -i <file name> [-d <delay between measurements in ms (def = 1000ms)>] [-c <meas count (def = 5)>]");
}

void PrintResults() {
    element* item = elemList;

    while (item != NULL) {
        switch (item->err) {
            case ERR_NO_DNS_RECORDS_FOUND:
                printf("%s - Site not found ", item->site);
                break;

            case ERR_OK:
                {
                    unsigned short avg = AvgCalc(item->measArray, measuresCount);
                    if(avg == USHRT_MAX) {
                        printf("%s (avg: -) ", item->site);
                    } else {
                        printf("%s (avg: %dms) ", item->site, avg);
                    }

                    for (int n = 0; n < measuresCount; n++) {
                        if (item->measArray[n] == USHRT_MAX) {
                            printf("%d -; ", (n + 1));
                        } else {
                            printf("%d %dms; ", (n + 1), item->measArray[n]);
                        }
                    }
                    printf("\n\r");
                }
                break;

            case ERR_OTHER:
            default:
                printf("%s - Error in measurement ", item->site);
                break;
        }

        item = item->nextElem;
    }
}

void* measurement(void* param) {
    element* elem = param;

    //Exit if no IPs found
    if(elem->addrs == NULL) {
        return NULL;
    }
    if(elem->measArray == NULL) {
        return NULL;
    }

    //Create HTTP request
    char request[128];
    char buf[1024];
    int len, bytesRecv;
    len = sprintf(request, "HEAD / HTTP/1.1\r\nHost: %s\r\nConnection: Close\r\n\r\n", elem->site);

    for(int n = 0; n < measuresCount; n++) {
        //Create socket. Only first IP addr used
        int sock = socket(elem->addrs->ai_family, elem->addrs->ai_socktype, elem->addrs->ai_protocol);
        if(sock == -1) {
            printf("Error in creating socket\n\r");
            return NULL;
        }

        //Create struct for poll()
        struct pollfd fd;
        fd.fd = sock;
        fd.events = POLLOUT;

        //Set socket to work in non-block mode
        long arg;
        if( (arg = fcntl(sock, F_GETFL, NULL)) < 0) {
            printf("Critical error in fcntl()\n\r");
            return NULL;
        }
        arg |= O_NONBLOCK;
        if( fcntl(sock, F_SETFL, arg) < 0) {
            printf("Critical error in fcntl()\n\r");
            return NULL;
        }

        //Create variables for storing timestamps and get START timestamp
        struct timespec start, stop;
        clock_gettime(CLOCK_REALTIME, &start);

        //Trying to connect
        int ret = connect(sock, elem->addrs->ai_addr, elem->addrs->ai_addrlen);

        if(ret < 0) {
            if (errno == EINPROGRESS) {
                while(1) {
                    ret = poll(&fd, 1, 5000 );
                    if ( ret <= 0 ) {
                        break;
                    } else {
                        if (fd.revents & POLLIN) {
                            //Received answer! Response content is not being processed
                            fd.revents = 0;
                            bytesRecv = recv(sock, buf, sizeof(buf), 0);
                            if(bytesRecv > 0) {
                                //If something received - get STOP time
                                clock_gettime(CLOCK_REALTIME, &stop);
                                ret = 1;
                            } else {
                                ret = -1;
                            }
                            break;
                        }

                        if (fd.revents & POLLOUT) {
                            //Connected! Send HTTP request
                            fd.revents = 0;
                            fd.events = POLLIN;
                            send(sock, request, len, 0);
                        }
                    }
                }
            }
        }

        close(sock);

        //Calculate time
        double t1 = (double)start.tv_sec + (double)start.tv_nsec / 1000000000.0;
        double t2 = (double)stop.tv_sec + (double)stop.tv_nsec / 1000000000.0;
        double time = (t2-t1) * 1000;
        elem->measArray[n] = FastRound(time);
        //And sleep for measurement delay
        usleep(measDelay_ms * 1000);
    }

    return NULL;
}

void cleanup() {
    element* item = elemList;
    while(item != NULL) {
        if(item->site != NULL) {
            free(item->site);
        }
        if(item->measArray != NULL) {
            free(item->measArray);
        }

        struct addrinfo* addrsItem = item->addrs;
        while(addrsItem != NULL) {
            struct addrinfo* addr = addrsItem;
            addrsItem = addrsItem->ai_next;
            free(addr);
        }
        element* el = item;
        item = item->nextElem;
        free(el);
    }
}

int main(int argc, char *argv[]) {
    //Parse input arguments
    int opt = 0;
    opt = getopt(argc, argv, optString);
    unsigned short val;
    while(opt != -1) {
        switch (opt) {
            case 'i':
                inputFileName = optarg;
                break;
            case 'c':
                val = atoi(optarg);
                if(val > 0 && val <= 100) {
                    measuresCount = val;
                } else {
                    printf("Meas count >100 or =0, using default %d\n\r", measuresCount);
                }
                break;
            case 'd':
                val = atoi(optarg);
                if(val > 100 && val <= 10000) {
                    measDelay_ms = val;
                } else {
                    printf("Delay <100 or >10000, using default %dms\n\r", measDelay_ms);
                }
                break;
            case 'h':
            case '?':
                PrintUsage();
                break;
            default:
                break;
        }
        opt = getopt( argc, argv, optString );
    }

    if(inputFileName == NULL) {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    //Read site list from input file
    FILE* fl = fopen(inputFileName, "r");

    if(fl != NULL) {
        element* lastItem = NULL;
        while(1) {
            char* lineBuf = NULL;
            size_t bufLen = 0;
            ssize_t lineLen = 0;

            lineLen = getline(&lineBuf, &bufLen, fl);
            if(lineLen == -1) {
                free(lineBuf);
                break;
            }

            while((lineBuf[lineLen - 1] == '\n' || lineBuf[lineLen - 1] == '\r') && lineLen > 0) {
                lineBuf[lineLen - 1] = 0;
                lineLen--;
            }

            element* elem = (element*) malloc(sizeof(element));
            if(elem == NULL) {
                printf("Critical error in reading file.\n\r");
                free(lineBuf);
                break;
            }
            memset(elem, 0, sizeof(element));

            elem->site = lineBuf;

            if(elemList == NULL) {
                elemList = elem;
            } else {
                lastItem->nextElem = elem;
            }
            lastItem = elem;
            elemCount++;
        }
        fclose(fl);
    } else {
        printf("File not found\n\r");
        exit(EXIT_FAILURE);
    }

    if(elemCount < 1) {
        printf("No site list... Exit.\n\r");
        exit(EXIT_FAILURE);
    }

    //Find IP addresses for all sites in list
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    element* item = elemList;
    while (item != NULL) {
        getaddrinfo(item->site, "http", &hints, &(item->addrs));
        if(item->addrs == NULL) {
            item->err = ERR_NO_DNS_RECORDS_FOUND;
            item->measArray = NULL;
        } else {
            item->measArray = malloc(measuresCount * sizeof(unsigned short));
            if(item->measArray != NULL) {
                for(int i = 0; i < measuresCount; i++) {
                    item->measArray[i] = USHRT_MAX;
                }
                item->err = ERR_OK;
            } else {
                item->err = ERR_OTHER;
            }
        }
        item = item->nextElem;
    }

    //Create threads for measurements (each site in own thread)
    pthread_t* threadId = (pthread_t*) malloc(elemCount * sizeof(pthread_t));
    if(threadId == NULL) {
        printf("Error in creating threads... Exit.");
        exit(EXIT_FAILURE);
    }
    pthread_attr_t threadAttr;
    pthread_attr_init(&threadAttr);

    item = elemList;
    int err;
    int threadsCount = 0;
    while (item != NULL) {
        if(item->err == ERR_OK) {
            err = pthread_create(&threadId[threadsCount], &threadAttr, measurement, item);
            if(err != 0) {
                printf("Error in creating threads...\n\r");
            } else {
                threadsCount++;
            }
        }
        item = item->nextElem;
    }

    //Waiting for all measurements to complete
    for(int i = 0; i < threadsCount; i++) {
        pthread_join(threadId[i], NULL);
    }

    //Print measurements results
    PrintResults();

    //All done! Cleanup and exit.
    cleanup();

    return EXIT_SUCCESS;
}