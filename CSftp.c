#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "dir.h"
#include "usage.h"
#include "login.h"
#include "constant.h"


#define BACKLOG 10
#define MAXDATASIZE 256

enum TYPE {
    ASCII,
    IMAGE
};

enum MODE {
    STREAM
};

enum STRU {
    STRU_FILE
};

enum FTP_CMD {
    INVALID = -1,
    USER,
    QUIT,
    CWD,
    CDUP,
    TYPE,
    MODE,
    STRU,
    RETR,
    PASV,
    NLST
};

struct ftp_cmd {
    enum FTP_CMD cmd;
    int argc;
    char** argv;
};

enum FTP_CMD charbufToCmd(char* command) {
    // printf("%s\n", command);
    if (command == NULL) {
        return INVALID;
    }

    if (strcmp(command, "USER") == 0) {
        return USER;
    } else if (strcmp(command, "QUIT") == 0) {
        return QUIT;
    } else if (strcmp(command, "CWD") == 0) {
        return CWD;
    } else if (strcmp(command, "CDUP") == 0) {
        return CDUP;
    } else if (strcmp(command, "TYPE") == 0) {
        return TYPE;
    } else if (strcmp(command, "MODE") == 0) {
        return MODE;
    } else if (strcmp(command, "STRU") == 0) {
        return STRU;
    } else if (strcmp(command, "RETR") == 0) {
        return RETR;
    } else if (strcmp(command, "PASV") == 0) {
        return PASV;
    } else if (strcmp(command, "NLST") == 0) {
        return NLST;
    } else {
        return INVALID;
    }
}

void to_upper(char* str) {
    int i;
    for (i = 0; i < strlen(str); i++) {
        str[i] = toupper(str[i]);
    }
}

// parse command buffer into ftp_cmd
struct ftp_cmd* parse_cmd(char* buf) {
    struct ftp_cmd* ftpcmd = malloc(sizeof(struct ftp_cmd));
    ftpcmd->argv = malloc(sizeof(char*) * MAXDATASIZE);
    ftpcmd->argc = 0;

    char* match = malloc(sizeof(char) * 4);
    match[0] = ' ';
    match[1] = (char) 10;
    match[2] = (char) 13;
    match[3] = '\0';

    char* command = strtok(buf, match);
    
    if (command != NULL) {
        to_upper(command);
    }

    ftpcmd->cmd = charbufToCmd(command);

    char* token = command;

    if (token != NULL) {
        token = strtok(NULL, match);
    }

    while (token != NULL && ftpcmd->argc < MAXDATASIZE-1) {
        ftpcmd->argv[ftpcmd->argc++] = token;
        token = strtok(NULL, match);
    }
    ftpcmd->argv[ftpcmd->argc] = 0;

    free(match);

    return ftpcmd;
}

// Replace char with another char
char* replace_char(char* str, char find, char replace){
    char *current_pos = strchr(str,find);
    while (current_pos){
        *current_pos = replace;
        current_pos = strchr(current_pos,find);
    }
    return str;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// Check if path is a file. Return false if directory, or file does not exist
int is_file(const char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

// Here is an example of how to use the above function. It also shows
// one how to get the arguments passed on the command line.

int main(int argc, char **argv) {

    // This is some sample code feel free to delete it
    // This is the main program for the thread version of nc

    int i;
    
    // Check the command line arguments
    if (argc != 2) {
      usage(argv[0]);
      return -1;
    }

    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;
    int yes = 1;
    int sockfd, new_fd;
    int rv;
    socklen_t sin_size;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    //PASV stuff
    struct addrinfo hintsPasv, *servinfoPasv, *pPasv;
    int new_sockfd;
    int addr_info;
    int yesPasv = 1;
    int inPassiveMode = 0;
    int datafd;
    struct sockaddr_storage new_their_addr;
    socklen_t new_sin_size;


    if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    
    // Set and bind main socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    while (1) {
        // Accept new connection
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *) &their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        } 

        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *) &their_addr), s, sizeof s);
        printf("server: got connection from %s\n", s);

        send(new_fd, SERVER_CONNECTED_MESSAGE, strlen(SERVER_CONNECTED_MESSAGE), 0);
        
        int isLoggedIn = 0;
        int keepRunning = 1;
        
        // Retrieve parentDirectory
        char parentDirectory[5000];
        getcwd(parentDirectory, sizeof(parentDirectory));

        while (keepRunning) {
            char buf[MAXDATASIZE];
            memset(buf, '\0', sizeof(char) * MAXDATASIZE);
            int bytesReceived = recv(new_fd, buf, MAXDATASIZE, 0);
            
            if (bytesReceived == -1) {
                return 0;
            } else if (bytesReceived == 0) {
                continue;
            }

            enum TYPE type = ASCII;
            enum MODE mode = STREAM;
            enum STRU stru = STRU_FILE;

            struct ftp_cmd *ftpcmd = parse_cmd(buf);
//            printf("%s\n", buf);
//            printf("%d\n", ftpcmd->argc);

            switch (ftpcmd->cmd) {
                case USER:
                    if (ftpcmd->argc != 1) {
                        send(new_fd, INCORRECT_ARGUEMENTS, strlen(INCORRECT_ARGUEMENTS), 0);
                    } else {
                        if (login(ftpcmd->argv[0])) {
                            isLoggedIn = 1;
                            send(new_fd, SUCCESS_LOGIN, strlen(SUCCESS_LOGIN), 0);
                        } else {
                            isLoggedIn = 0;
                            send(new_fd, UNABLE_LOGIN, strlen(UNABLE_LOGIN), 0);
                        }
                    }
                    break;
                case QUIT:
                    if (ftpcmd->argc != 0) {
                        send(new_fd, INCORRECT_ARGUEMENTS, strlen(INCORRECT_ARGUEMENTS), 0);
                    } else {
                        isLoggedIn = 0;
                        keepRunning = 0;
                        send(new_fd, SUCCESS_QUIT, strlen(SUCCESS_QUIT), 0);
                    }
                    break;
                case CWD:
                    if (!isLoggedIn) {
                        send(new_fd, PLEASE_LOGIN, strlen(PLEASE_LOGIN), 0);
                    } else {
                        if (ftpcmd->argc != 1) {
                            send(new_fd, INCORRECT_ARGUEMENTS, strlen(INCORRECT_ARGUEMENTS), 0);
                        } else {
                            char* path = ftpcmd->argv[0];
                        
                            if (strncmp(DOT_SLASH, path, strlen(DOT_SLASH)) == 0 || 
                                strncmp(DOT_DOT_SLASH, path, strlen(DOT_DOT_SLASH)) == 0 || 
                                strstr(path, DOT_DOT_SLASH) != 0 || chdir(path) != 0) {
                                send(new_fd, INVALID_DIRECTORY, strlen(INVALID_DIRECTORY), 0);
                            } else {
                                send(new_fd, SUCCESS_DIRECTORY, strlen(SUCCESS_DIRECTORY), 0);
                            }
                        }
                    }
                    break;
                case CDUP:
                    if (!isLoggedIn) {
                        send(new_fd, PLEASE_LOGIN, strlen(PLEASE_LOGIN), 0);
                    } else {
                        if (ftpcmd->argc != 0) {
                            send(new_fd, INCORRECT_ARGUEMENTS, strlen(INCORRECT_ARGUEMENTS), 0);
                        } else {
                            char currentDirectory[5000];
                            getcwd(currentDirectory, sizeof(currentDirectory));
                            if (strlen(currentDirectory) == strlen(parentDirectory)) {
                                send(new_fd, INVALID_DIRECTORY, strlen(INVALID_DIRECTORY), 0);
                            } else {
                                int p = 0;
                                for (p = strlen(currentDirectory)-1; p >= 0; p--) {
                                    if (currentDirectory[p] == '/') {
                                        currentDirectory[p] = 0;
                                        break;
                                    }
                                    currentDirectory[p] = 0;
                                }
                                if (chdir(currentDirectory) != 0) {
                                    send(new_fd, INVALID_DIRECTORY, strlen(INVALID_DIRECTORY), 0);
                                }  else {
                                    send(new_fd, SUCCESS_DIRECTORY, strlen(SUCCESS_DIRECTORY), 0);
                                }
                            }
                        }
                    }
                    break;
                case TYPE:
                    if (!isLoggedIn) {
                        send(new_fd, PLEASE_LOGIN, strlen(PLEASE_LOGIN), 0);
                    } else {
                        if (ftpcmd->argc != 1 && ftpcmd->argc != 2) {
                            send(new_fd, INCORRECT_ARGUEMENTS, strlen(INCORRECT_ARGUEMENTS), 0);
                        } else if (ftpcmd->argc == 2) {
                            to_upper(ftpcmd->argv[0]);
                            to_upper(ftpcmd->argv[1]);
                            if (strcmp(ftpcmd->argv[0], "A") == 0 || strcmp(ftpcmd->argv[0], "E") == 0) {
                                if (strcmp(ftpcmd->argv[1], "N") == 0 || strcmp(ftpcmd->argv[1], "T") == 0 
                                    || strcmp(ftpcmd->argv[1], "C") == 0) {
                                    send(new_fd, UNIMPLEMENTED_COMMAND, strlen(UNIMPLEMENTED_COMMAND), 0);
                                } else {
                                    send(new_fd, INCORRECT_ARGUEMENTS, strlen(INCORRECT_ARGUEMENTS), 0);
                                }
                            } else if (strcmp(ftpcmd->argv[0], "L") == 0) {
                                send(new_fd, UNIMPLEMENTED_COMMAND, strlen(UNIMPLEMENTED_COMMAND), 0);
                            } else {
                                send(new_fd, INCORRECT_ARGUEMENTS, strlen(INCORRECT_ARGUEMENTS), 0);
                            }
                        } else {
                            to_upper(ftpcmd->argv[0]);
                            if (strcmp(ftpcmd->argv[0], "A") == 0) {
                                type = ASCII;
                                send(new_fd, ASCII_COMMAND, strlen(ASCII_COMMAND), 0);
                            } else if (strcmp(ftpcmd->argv[0], "I") == 0) {
                                type = IMAGE;
                                send(new_fd, IMAGE_COMMAND, strlen(IMAGE_COMMAND), 0);
                            } else {
                                send(new_fd, INCORRECT_ARGUEMENTS, strlen(INCORRECT_ARGUEMENTS), 0);
                            }
                        }
                    }
                    break;
                case MODE:
                    if (!isLoggedIn) {
                        send(new_fd, PLEASE_LOGIN, strlen(PLEASE_LOGIN), 0);
                    } else {
                        if (ftpcmd->argc != 1) {
                            send(new_fd, INCORRECT_ARGUEMENTS, strlen(INCORRECT_ARGUEMENTS), 0);
                        } else {
                            to_upper(ftpcmd->argv[0]);
                            if (strcmp(ftpcmd->argv[0], "S") == 0) {
                                mode = STREAM;
                                send(new_fd, STREAM_COMMAND, strlen(STREAM_COMMAND), 0);
                            } else if (strcmp(ftpcmd->argv[0], "B") == 0 || strcmp(ftpcmd->argv[0], "C") == 0) {
                                send(new_fd, UNIMPLEMENTED_COMMAND, strlen(UNIMPLEMENTED_COMMAND), 0);
                            } else {
                                send(new_fd, INCORRECT_ARGUEMENTS, strlen(INCORRECT_ARGUEMENTS), 0);
                            }
                        }
                    }
                    break;
                case STRU:
                    if (!isLoggedIn) {
                        send(new_fd, PLEASE_LOGIN, strlen(PLEASE_LOGIN), 0);
                    } else {
                        if (ftpcmd->argc != 1) {
                            send(new_fd, INCORRECT_ARGUEMENTS, strlen(INCORRECT_ARGUEMENTS), 0);
                        } else {
                            to_upper(ftpcmd->argv[0]);
                            if (strcmp(ftpcmd->argv[0], "F") == 0) {
                                stru = STRU_FILE;
                                send(new_fd, FILE_COMMAND, strlen(FILE_COMMAND), 0);
                            } else if (strcmp(ftpcmd->argv[0], "R") == 0 || strcmp(ftpcmd->argv[0], "P") == 0) {
                                send(new_fd, UNIMPLEMENTED_COMMAND, strlen(UNIMPLEMENTED_COMMAND), 0); 
                            } else {
                                send(new_fd, INCORRECT_ARGUEMENTS, strlen(INCORRECT_ARGUEMENTS), 0);
                            }
                        }
                    }
                    break;
                case RETR:
                    if (!isLoggedIn) {
                        send(new_fd, PLEASE_LOGIN, strlen(PLEASE_LOGIN), 0);
                    } else {
                        if (ftpcmd->argc != 1) {
                            send(new_fd, INCORRECT_ARGUEMENTS, strlen(INCORRECT_ARGUEMENTS), 0);
                        } else {
                            if(inPassiveMode) {
                                const char *fileData;
                                unsigned char* source = NULL;
                                long bufsize;

                                // Read binary into fileData
                                FILE *fp = fopen(ftpcmd->argv[0], "rb");
                                if (fp != NULL && is_file(ftpcmd->argv[0])) {
                                    if (fseek(fp, 0L, SEEK_END) == 0) {
                                        bufsize = ftell(fp);
                                        if (bufsize == -1) {
                                            perror("file");
                                        }

                                        char res[1024];
                                        sprintf(res, INCOMING_FILE, ftpcmd->argv[0], bufsize);
                                        send(new_fd, res, strlen(res), 0);

                                        source = malloc(sizeof(unsigned char) * (bufsize));

                                        if (fseek(fp, 0L, SEEK_SET) != 0) {
                                            perror("file");
                                        }

                                        size_t newLen = fread(source, sizeof(unsigned char), bufsize, fp);
                                        if(ferror(fp) != 0) {
                                            perror("file");
                                        }
                                    }
                                    fclose(fp);

                                    if (send(datafd, source, bufsize, 0) != -1) {
                                        send(new_fd, TRANSFER_COMPLETE, strlen(TRANSFER_COMPLETE), 0);
                                    }
                                    free(source);
                                } else {
                                    send(new_fd, RETR_FAIL, strlen(RETR_FAIL), 0);
                                }
                                
                                close(datafd);
                                close(new_sockfd);
                                inPassiveMode = 0;
                            } else {
                                send(new_fd, REQUIRE_PASSIVE, strlen(REQUIRE_PASSIVE), 0);
                            }
                        }
                    }
                    break;
                case PASV:
                    if (!isLoggedIn) {
                        send(new_fd, PLEASE_LOGIN, strlen(PLEASE_LOGIN), 0);
                    } else {
                        if (ftpcmd->argc != 0) {
                            send(new_fd, INCORRECT_ARGUEMENTS, strlen(INCORRECT_ARGUEMENTS), 0);
                        } else {
                            //PASV stuff

                            if (inPassiveMode) {
                                close(datafd);
                                close(new_sockfd);
                                datafd = -1;
                                new_sockfd = -1;

                                inPassiveMode = 0;
                            }


                            memset(&hintsPasv, 0, sizeof hintsPasv);
                            hintsPasv.ai_family = AF_UNSPEC;
                            hintsPasv.ai_socktype = SOCK_STREAM;
                            hintsPasv.ai_flags = AI_PASSIVE; // use my IP

                            
                            if ((addr_info = getaddrinfo(NULL, "0", &hintsPasv, &servinfoPasv)) != 0) {
                                fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(addr_info));
                                continue;
                            }

                            for(pPasv = servinfoPasv; pPasv != NULL; pPasv = pPasv->ai_next) {
                                if ((new_sockfd = socket(pPasv->ai_family, pPasv->ai_socktype,
                                        pPasv->ai_protocol)) == -1) {
                                    perror("server: socket");
                                    continue;
                                }

                                if (setsockopt(new_sockfd, SOL_SOCKET, SO_REUSEADDR, &yesPasv,
                                        sizeof(int)) == -1) {
                                    perror("setsockopt");
                                    continue;
                                }

                                if (bind(new_sockfd, pPasv->ai_addr, pPasv->ai_addrlen) == -1) {
                                    close(new_sockfd);
                                    new_sockfd = -1;
                                    perror("server: bind");
                                    continue;
                                }

                                break;
                            }
 
                            struct sockaddr_in sock_addrPasv; 
                            socklen_t addr_lenPasv = sizeof(sock_addrPasv);   

                            if (getsockname(new_sockfd, (struct sockaddr *) &sock_addrPasv, &addr_lenPasv) == -1) {
                                perror("cannot find sockname");
                                close(new_sockfd);
                                new_sockfd = -1;
                                continue;
                            }  

                            // use sock_addrPasv.sin_port for port
                            int port = (int) ntohs(sock_addrPasv.sin_port);

                            char portStrHalf1[8];
                            sprintf(portStrHalf1, "%d", port / 256);
                            char portStrHalf2[8];
                            sprintf(portStrHalf2, "%d", port % 256);
                            
                            // Get hostname
                            char hostname[INET6_ADDRSTRLEN];
                            if (gethostname(hostname, INET6_ADDRSTRLEN) == -1) {
                                perror("cannot gethostname");
                                close(new_sockfd);
                                new_sockfd = -1;
                                continue;
                            }

                            struct hostent *he;
                            struct in_addr ** addr_list;
                            
                            // Get host by name

                            if ((he = gethostbyname(hostname)) == NULL || he->h_addr_list[0] == NULL) {
                                perror("gethostbyname");
                                close(new_sockfd);
                                new_sockfd = -1;
                                continue;
                            }

                            addr_list = (struct in_addr **) he->h_addr_list;

                            char * ipString = replace_char(inet_ntoa(*addr_list[0]), '.', ',');
                            char res[1024];

                            sprintf(res, SUCCESS_PASSIVE, ipString, portStrHalf1, portStrHalf2);

                            send(new_fd,res,strlen(res),0);

                            freeaddrinfo(servinfoPasv);

                            if (listen(new_sockfd, BACKLOG) == -1) {
                                perror("listen");
                                close(new_sockfd);
                                new_sockfd = -1;
                                continue;
                            }
                            
                            struct timeval tv;
                            tv.tv_sec = 20;
                            tv.tv_usec = 0;
                            
                            fd_set fdset;
                            FD_ZERO(&fdset);
                            FD_SET(new_sockfd,&fdset);
                            
                            // Socket timeout in 20 seconds
                            if (select(new_sockfd+1, &fdset, NULL, NULL, &tv) <= 0) {
                                close(new_sockfd);
                                inPassiveMode = 0;
                                new_sockfd = -1;
                                send(new_fd, TIMEOUT_PASSIVE, strlen(TIMEOUT_PASSIVE), 0);
                                continue;
                            }
                            if (FD_ISSET(new_sockfd, &fdset)) {
                                new_sin_size = sizeof new_their_addr;
                                datafd = accept(new_sockfd, (struct sockaddr *) &new_their_addr, &new_sin_size);
                                if(datafd == -1) {
                                    perror("accept\n");
                                    close(new_sockfd);
                                    new_sockfd = -1;
                                    continue;
                                }
                                inPassiveMode = 1;
                            }
                        }
                    }
                    break;
                case NLST:
                    if (!isLoggedIn) {
                        send(new_fd, PLEASE_LOGIN, strlen(PLEASE_LOGIN), 0);
                    } else {
                        if (ftpcmd->argc != 0) {
                            send(new_fd, INCORRECT_ARGUEMENTS, strlen(INCORRECT_ARGUEMENTS), 0);
                        } else {
                            if(inPassiveMode) {
                                send(new_fd, INCOMING_DIRECTORY_LISTING, strlen(INCOMING_DIRECTORY_LISTING), 0);

                                int filesListed = listFiles(datafd, ".");

                                // -1 the named directory does not exist or you don't have permission to read it.
                                // -2 insufficient resources to perform request

                                if(filesListed >= 0) {
                                    send(new_fd, DIRECTORY_LISTING_SUCCESS, strlen(DIRECTORY_LISTING_SUCCESS), 0);
                                } else if(filesListed == -1) {
                                    send(new_fd, DIRECTORY_DOES_NOT_EXIT, strlen(DIRECTORY_DOES_NOT_EXIT), 0);
                                } else if(filesListed == -2) {
                                    send(new_fd, NOT_ENOUGH_RESOURCES, strlen(NOT_ENOUGH_RESOURCES), 0);
                                }

                                close(datafd);
                                close(new_sockfd);
                                inPassiveMode = 0;
                            } else {
                                send(new_fd, REQUIRE_PASSIVE, strlen(REQUIRE_PASSIVE), 0);
                            }
                        }
                    }
                    break;
                default:
                    send(new_fd, UNSUPPORTED_COMMAND, strlen(UNSUPPORTED_COMMAND), 0);
                    break;
            }
            int j;

            // for (j = 0; j < ftpcmd->argc; j++) {
            //     free(ftpcmd->argv[j]);
            // }

            free (ftpcmd->argv);
            free (ftpcmd);
            
        }
            
        close(new_fd);
    }

    close(sockfd);

    // This is how to call the function in dir.c to get a listing of a directory.
    // It requires a file descriptor, so in your code you would pass in the file descriptor 
    // returned for the ftp server's data connection
    
    // printf("Printed %d directory entries\n", listFiles(1, "."));
    return 0;

}
