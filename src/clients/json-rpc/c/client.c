/*
 * http://www.linuxhowtos.org/C_C++/socket.htm
 * https://www.ibm.com/support/knowledgecenter/en/SSB23S_1.1.0.14/gtpc1/unixsock.html
 * 
 * 
 * gcc -g -Wall -o client client.c
 * 
 * */
 
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <unistd.h>
#include <errno.h>

//#define SERVER_PATH "tpf_unix_sock.server"

#define DATA "Hello from client\n"

#define SERVER_PATH "/tmp/test_unix_socket.server"
 
int main(int c,char *argv[])
{    
    int client_socket, rc;
    struct sockaddr_un remote; 
    char buf[256];
    memset(&remote, 0, sizeof(struct sockaddr_un));
    
    /****************************************/
    /* Create a UNIX domain datagram socket */
    /****************************************/
    client_socket = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (client_socket == -1) {
        exit(1);
    }

    /***************************************/
    /* Set up the UNIX sockaddr structure  */
    /* by using AF_UNIX for the family and */
    /* giving it a filepath to send to.    */
    /***************************************/
    remote.sun_family = AF_UNIX;        
    strcpy(remote.sun_path, SERVER_PATH); 
    
    /***************************************/
    /* Copy the data to be sent to the     */
    /* buffer and send it to the server.   */
    /***************************************/
    strcpy(buf, DATA);
    printf("Sending data...\n");
    rc = sendto(client_socket, buf, strlen(buf), 0, (struct sockaddr *) &remote, sizeof(remote));
    if (rc == -1) {
        close(client_socket);
        exit(1);
    }   
    else {
        printf("Data sent!\n");
    }
    
    /*****************************/
    /* Close the socket and exit */
    /*****************************/
    rc = close(client_socket);
    
    return 0;
}
