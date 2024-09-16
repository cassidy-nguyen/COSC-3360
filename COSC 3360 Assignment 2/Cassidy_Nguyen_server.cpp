/*
Name: Cassidy Nguyen
PSID: 2042567
Course Number: COSC 3360
Professor: Paris
Title: Second Assignment
Notes: Code snippets (such as the establish function, get_connnection, etc.) were taken from Professor Paris' sockets notes page as well as the Linux How To link
provided in the Assignment description. Unlike the client, the server does require an additional argument for the port number and does not
prompt the user for that information.
*/

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <fstream>
#include <fcntl.h>

using namespace std;

// error message
void error(char *msg) {
    perror(msg);
    exit(0);
}

// code to establish a socket; originally from bzs@bu-cs.bu.edu
int establish(unsigned short portnum) {
    int MAXHOSTNAME = 12; // ???
    char myname[MAXHOSTNAME + 1];
    int s;
    struct sockaddr_in sa;
    struct hostent *hp;

    memset(&sa, 0, sizeof(struct sockaddr_in)); // clear our address
    gethostname(myname, MAXHOSTNAME);           // who are we? 
    hp = gethostbyname(myname);                 // get our address info 
    if (hp == NULL)                             // we don't exist !? 
        return (-1);
    sa.sin_family = hp->h_addrtype;                // this is our host address 
    sa.sin_port = htons(portnum);                  // this is our port number 
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) // create socket 
        return (-1);
    if (bind(s, (struct sockaddr *) &sa, sizeof(struct sockaddr_in)) < 0) {
        close(s);
        return (-1); // bind address to socket
    }
    listen(s, 3); // max # of queued connects
    return (s);
}

// wait for a connection to occur on a socket created with establish()
int get_connection(int s) {
    int t; /* socket of connection */

    if ((t = accept(s, NULL, NULL)) < 0) /* accept connection if there is one */
        return (-1);
    return (t);
}

int read_data(int s, char *buf, int n) { // s = connected socket, buf = pointer to the buffer, n = number of characters (bytes) we want
    int bcount; // counts bytes read
    int br; // bytes read this pass
    
    bcount = 0;
    br = 0;
    if ((br = read(s, buf, n - bcount)) > 0) {
        bcount += br; // increment byte counter
        buf += br; // move buffer ptr for next read
    }
    else if (br < 0) { // signal an error to the caller
        return(-1);
    }
    return(bcount);
}

int main(int argc, char *argv[]) {
    int sockd, newsockd, portnum;
    socklen_t clientLen;
    char buffer[512];
    struct sockaddr_in server_addr, client_addr;
    int nBytes; // returned by read()

    // check if a portnumber is provided
    if (argc != 2) {
        fprintf(stderr, "Usage %s portnumber\n", argv[0]);
        exit(1);
    }

    // create a socket
    sockd = socket(AF_INET, SOCK_STREAM, 0); // replace with establish function
    if (sockd < 0) {
        error("ERROR opening socket");
    }

    // bind an address to that socket
    bzero((char *) &server_addr, sizeof(server_addr));
    portnum = atoi(argv[1]);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(portnum);

    if (bind(sockd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        error("ERROR on binding");
    }

    // wait for a request
    listen(sockd, 5);
    //printf("Server is now 'listening.'\n");
    
    clientLen = sizeof(client_addr);

    while (true) {
        newsockd = accept(sockd, (struct sockaddr *) &client_addr, &clientLen);
        if (newsockd < 0) {
            error("ERROR on accept");
            continue;
        }

        bzero(buffer, 512);
        nBytes = read_data(newsockd, buffer, 512);
        if (nBytes < 0) {
            error("ERROR reading from socket");
        }

        buffer[nBytes] = '\0';
        
        string filename(buffer);

        // command is terminate
        if (filename == "terminate") {
            cout << "Goodbye!" << endl;
            close(newsockd);
            close(sockd);
            exit(0);
        }
        
        // command is exit
        else if (filename == "exit") {
            close(newsockd);
            close(sockd);
            return 0;
        }

        int fd = open(filename.c_str(), O_RDONLY);
        if (fd < 0) { // file does not exist
            cout << "A client requested the file " << filename << ". That file is missing!" << endl; 
            write(newsockd, "0", 1); // send a one-byte error message to the client
            close(newsockd);
        }
        else {
            cout << "A client requested the file " << filename << "." << endl;
            write(newsockd, "1", 1); // send a one-bye OK message to the client
            do {
                nBytes = read(fd, buffer, 512);
                if (nBytes > 0) {
                    write(newsockd, buffer, nBytes);
                }
            } while (nBytes == 512);
            close(fd);
            close(newsockd);
        }
    }

    close(sockd);

    return 0;
}