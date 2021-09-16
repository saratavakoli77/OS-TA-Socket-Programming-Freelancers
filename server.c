#include <stdio.h>  
#include <string.h>
#include <stdlib.h>  
#include <errno.h>  
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <sys/time.h> 

int main(int argc, char const *argv[]){
    int server_sock_fd, result, i, j;
    int on = 1;
    int server_port = atoi(argv[1]);    //  give sever port
    int last_auction_port = server_port;
    struct sockaddr_in server_address, client_address;
    fd_set read_fds, copy_fds;
    int max_fd, activity, ready_sockets;
    int new_socket, address_len, close_connection;
    char* welcome_message = "Welcome! you are connected.\nWhich auction do you wanna attend? ";
    char* finish_event_message = "<-------------------One event finished------------------->\n";
    char buffer[64] = {0};

    int auctions[100][100];
    int auctions_size[100];

    for(int i = 0; i < 100; i++) {
        for(int j = 0; j < 100; j++) {
            auctions[i][j] = 0;
        }
        auctions_size[i] = 0;
    }

    server_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock_fd < 0){ 
        write(STDOUT_FILENO, "ERROR: On server socket creation.\n", strlen("ERROR: On server socket creation.\n"));
        return -1;
    }

    if (setsockopt(server_sock_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0)
    {
        perror("setsockopt SO_REUSEADDR");
        close(server_sock_fd);
        exit(-1);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(server_port);

    result = bind(server_sock_fd, (struct sockaddr*)&server_address, sizeof(server_address));
    if (result < 0){
        write(STDOUT_FILENO, "ERROR: On binding.\n", strlen("ERROR: On binding.\n"));
        return -1;
    }
    
    result = listen(server_sock_fd, 20);
    if (result < 0){
        write(STDOUT_FILENO, "ERROR: On listening.\n", strlen("ERROR: On listening.\n"));
        return -1;
    }

    address_len = sizeof(client_address);
    max_fd = server_sock_fd;
    FD_ZERO(&copy_fds);
    FD_SET(server_sock_fd, &copy_fds);
    write(STDOUT_FILENO, "Starting server ...\n", strlen("Starting server ...\n"));
    while (1){
        // Add sockets to set
        memcpy(&read_fds, &copy_fds, sizeof(copy_fds));

        // Select
        activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        if ((activity < 0) && (errno!=EINTR)){
            write(STDOUT_FILENO, "ERROR: select\n", strlen("ERROR: select\n"));
            return -1;
        }

        ready_sockets = activity;
        for (i = 0; i <= max_fd  &&  ready_sockets > 0; ++i){
            if (FD_ISSET(i, &read_fds)){
                ready_sockets -= 1;

                if (i == server_sock_fd){
                    new_socket = accept(server_sock_fd, (struct sockaddr *)&client_address, (socklen_t*)&address_len);
                    if(new_socket < 0){
                        perror("ERROR: On accepting new client.\n");
                        return -1;
                    }
                    write(STDOUT_FILENO, "New connection has come.\n", strlen("New connection has come.\n"));
                    send(new_socket, welcome_message, strlen(welcome_message), 0);
                    FD_SET(new_socket, &copy_fds);
                    if (new_socket > max_fd){
                        max_fd = new_socket;
                    }
                }
                else{
                    close_connection = 0;

                    result = recv(i, buffer, sizeof(buffer), 0);

                    if (result < 0){
                        if (errno != EWOULDBLOCK){
                            perror("recv() failed");
                            close_connection = 1;
                        }
                        
                    }



                    if (result == 0){
                        write(STDOUT_FILENO, "Connection closed\n", strlen("Connection closed\n"));
                        close_connection = 1;
                    }

                    //  --------------------------------------

                    write(STDOUT_FILENO, "Suggested number for the auction: ", strlen("Suggested number for the auction: "));
                    write(STDOUT_FILENO, buffer, strlen(buffer));
                    int num_of_auction = atoi(buffer);
                    // Handle new person to jon auction

                    auctions[num_of_auction][auctions_size[num_of_auction]] = i;
                    auctions_size[num_of_auction] += 1;
                    if(auctions_size[num_of_auction] == 5){
                        // Send port and auction priorities
                        write(STDOUT_FILENO, "new auction starts.\n", strlen("new auction starts.\n"));
                        last_auction_port += 1;
                        for(j = 0; j < 5; j++){
                            send(auctions[num_of_auction][j], &last_auction_port, sizeof(int), 0);
                            send(auctions[num_of_auction][j], &j, sizeof(int), 0);
                        }
                    }
                    memset(buffer, 0, 64);

                    //  --------------------------------------

                    if (close_connection){
                        close(i);
                        FD_CLR(i, &copy_fds);
                        if (i == max_fd)
                        {
                            while (FD_ISSET(max_fd, &copy_fds) == 0)
                                max_fd -= 1;
                        }
                    }
                }
            }
        }

        write(STDOUT_FILENO, finish_event_message, strlen(finish_event_message));
    }

    close(server_sock_fd);
    return 0;
}