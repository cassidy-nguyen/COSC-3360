/*
Name: Cassidy Nguyen
PSID: 2042567
Course Number: COSC 3360
Professor: Paris
Title: Second Assignment
Notes: Code snippets (such as the call_socket function) were taken from Professor Paris' sockets notes page as well as the Linux How To link
provided in the Assignment description. The client does not require additional arguments, instead it will prompt the user for inputs accordingly.
For the server hostname, the client will only accept the input "localhost" to eliminate possible errors with the domain.
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
#include <fcntl.h>

using namespace std;

// error message
void error(char *msg) {
    perror(msg);
    exit(0);
}

// dialing/how to call a socket
int call_socket(const char *hostname, unsigned short portnum) {
    struct sockaddr_in sa; // sa stands for socket address
    struct hostent *hp;
    int a, s; // s stands for socket

    if ((hp = gethostbyname(hostname)) == NULL) { // do we know the host's
        errno = ECONNREFUSED; // address?
        return (-1);          // no
    }

    memset(&sa, 0, sizeof(sa));
    memcpy((char *)&sa.sin_addr, hp->h_addr, hp->h_length); // set address
    sa.sin_family = hp->h_addrtype;
    sa.sin_port = htons((u_short)portnum);

    if ((s = socket(hp->h_addrtype, SOCK_STREAM, 0)) < 0) // get socket
        return (-1);
    if (connect(s, (struct sockaddr*)&sa, sizeof sa) < 0) { // connect
        close(s);
        return (-1);
    }
    return (s);
}

int main() {
    int sockd; // socket
    string hostname;
    int portnum;

    char buffer[512];
    size_t bufferSize = 512;

    // prompt the user for the server hostname and port number
    cout << "Enter server hostname:" << endl;
    cin >> hostname;
    while (hostname != "localhost") {
        cout << "ERROR: Incorrect server host name. Please enter 'localhost':" << endl;
        cin >> hostname;
    }

    cout << "Enter port number:" << endl;
    cin >> portnum;
    while (portnum <= 1024) {
        cout << "ERROR: Invalid port number. Port number must be greater than 1024. Enter port number again: " << endl;
        cin >> portnum;
    }

    // create a socket and connect to server
    if ((sockd = call_socket(hostname.c_str(), portnum)) < 0) {
        error("Error: Cannot call socket");
        exit(1);
    }

    // prompt the user for a filename
    string filename;
    cout << "Enter filename/command:" << endl;
    cin >> filename;

    // act on the command
    if (filename == "terminate" || filename == "exit") { // terminate or exit command
        send(sockd, filename.c_str(), filename.size(), 0);
        close(sockd);
        exit(0);
    }
    else { // get filename command
        send(sockd, filename.c_str(), filename.size(), 0);
    }

    // receive the server's reply
    int nRead, byteCount = 0;
    nRead = read(sockd, buffer, 1); // reading the one byte response from the server
    if (nRead < 0) {
        error("ERROR reading from socket");
    }
    
    if (buffer[0] == '0') { // error message byte response
        close(sockd);
        exit(0);
    }
    int fd = open(filename.c_str(), O_WRONLY | O_CREAT, 0600);
    do {
        nRead = read(sockd, buffer, 512);
        if (nRead > 0) {
            byteCount += nRead;
            write(fd, buffer, nRead);
        }
    } while (nRead == 512);

    cout << "Received file " << filename << " (" << byteCount << " bytes)" << endl;

    // close the socket
    close(fd);
    close(sockd);

    return 0;
}