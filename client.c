#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>


int auction_finished(){
    //  agar yek dor tamam shod va kasi pishnahad balatari vared nakard 1 bargardan vagar na 0
    return 0;
}

void apply_coordinates(int* max_price, int suggested_price, int my_priority, int* winner){
    if(suggested_price > *max_price) {
        *max_price = suggested_price;
        *winner = my_priority;
    }
}

void inform_winner(int winner){
    int client_sock_fd;
    send(client_sock_fd, "WINNER", strlen("WINNER"), 0);
}

void sigalrm_handler(int signal){
    write(STDOUT_FILENO, "\n\nNO MORE TIME!\n\n", strlen("\n\nNO MORE TIME!\n\n"));
}

int start_auction(int auction_number, int port, int my_priority){
    char* seperator_message = "<-------------------------->\n";
    int udp_socket;
    int broadcast = 1;
    int reuseport = 1;
    struct sockaddr_in address;
    int len, other_len;
    char str_turn[4] = {0};
    char buffer[64] = {0};
    int max_price = 0;
    int winner = -1;

    int turn = 0;
    int input;


    // Create UDP socket
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0){
        write(STDOUT_FILENO, "ERROR: On udp socket creation.\n", strlen("ERROR: On udp socket creation.\n"));
        return -1;
    }

    if (setsockopt(udp_socket, SOL_SOCKET, SO_REUSEPORT, (char *)&reuseport, sizeof(reuseport)) < 0){
        perror("setsockopt SO_REUSEPORT");
        exit(-1);
    }

    if (setsockopt(udp_socket, SOL_SOCKET, SO_BROADCAST, (char *)&broadcast, sizeof(broadcast)) < 0){
        perror("setsockopt SO_BROADCAST");
        exit(-1);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("255.255.255.255");
    address.sin_port = htons(port);
    
    if (bind(udp_socket, (const struct sockaddr *)&address, sizeof(address)) < 0){ 
        perror("bind failed"); 
        exit(EXIT_FAILURE);
    } 

    len = sizeof(address);

    siginterrupt(SIGALRM, 1);
    signal(SIGALRM, sigalrm_handler);

    while(!auction_finished()){
        write(STDOUT_FILENO, "Turn ", strlen("Turn "));
        str_turn[0] = turn + '0';
        write(STDOUT_FILENO, str_turn, strlen(str_turn));
        write(STDOUT_FILENO, ":\n", strlen(":\n"));
        if(turn == my_priority){
            while (1){
                write(STDOUT_FILENO, "It's your turn.\n", strlen("It's your turn.\n"));
                //memset(input, 0, 8);
                alarm(20);
                write(STDOUT_FILENO, "Enter your suggested price: ", strlen("Enter your suggested price: "));
                read(0, &input, sizeof(input));
                alarm(0);

                // Send price
                sendto(udp_socket, (const void*)input, sizeof(input), 0, (const struct sockaddr*)&address, len);
                recvfrom(udp_socket, (char *)buffer, 64, 0, (struct sockaddr*)&address, &len);  //  get acknowledge

                apply_coordinates(&max_price, input, my_priority, &winner);

                write(STDOUT_FILENO, seperator_message, strlen(seperator_message));

            }
        }
        else{
            while(1){
                write(STDOUT_FILENO, "It's not your turn. please wait\n", strlen("It's not your turn. please wait\n"));
                // Receive price
                memset(buffer, 0, 64);
                recvfrom(udp_socket, (char *)buffer, 64, 0, (struct sockaddr*)&address, &len);
                input = atoi(buffer);
                memset(buffer, 0, 64);

                apply_coordinates(&max_price, input, turn, &winner);

                write(STDOUT_FILENO, seperator_message, strlen(seperator_message));

            }
        }
        turn += 1;
        if (turn == 5){
            turn = 0;
        }
    }
    if(winner == my_priority)
        inform_winner(winner);
}

int main(int argc, char const *argv[]){  
    int client_sock_fd; 
    struct sockaddr_in server_address;
    int server_port = atoi(argv[1]);
    int connection_result;
    char buffer[128] = {0};
    char auction_number[15];
    int auction_port, auction_priority;
    char str_priority[4] = {0};
    char str_port[8] = {0};
    char* waiting_message = "Waiting for other people...\n";
    char* seperator_message = "<-------------------------->\n";

    client_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock_fd < 0){
        write(STDOUT_FILENO, "ERROR: On client socket creation.\n", strlen("ERROR: On client socket creation.\n"));
        return -1;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0){
        write(STDOUT_FILENO, "ERROR: Invalid address.\n", strlen("ERROR: Invalid address.\n"));
        return -1;
    }

    connection_result = connect(client_sock_fd, (struct sockaddr*)&server_address, sizeof(server_address));
    if (connection_result < 0){
        write(STDOUT_FILENO, "ERROR: On connection failed.\n", strlen("ERROR: On connection failed.\n"));
        return -1;
    }
    
    // Get welcome message
    recv(client_sock_fd, buffer, sizeof(buffer), 0);
    write(STDOUT_FILENO, buffer, strlen(buffer));
    write(STDOUT_FILENO, "\n", strlen("\n"));
    memset(buffer, 0, 128);

    // Send auction number
    read(0, auction_number, 15);
    send(client_sock_fd, auction_number, strlen(auction_number), 0);

    // Recieve auction port
    write(STDOUT_FILENO, waiting_message, strlen(waiting_message));
    recv(client_sock_fd, &auction_port, sizeof(int), 0);
    write(STDOUT_FILENO, seperator_message, strlen(seperator_message));
    write(STDOUT_FILENO, "auction started\auction port: ", strlen("auction started\auction port: "));
    sprintf(str_port, "%d\n", auction_port);
    write(STDOUT_FILENO, str_port, strlen(str_port));

    // Recieve auction priority
    recv(client_sock_fd, &auction_priority, sizeof(int), 0);
    str_priority[0] = auction_priority + '0';
    write(STDOUT_FILENO, "auction priority: ", strlen("auction priority: "));
    write(STDOUT_FILENO, str_priority, strlen(str_priority));
    write(STDOUT_FILENO, "\n", strlen("\n"));
    write(STDOUT_FILENO, seperator_message, strlen(seperator_message));

    // start auction
    start_auction(atoi(auction_number), auction_port, auction_priority);

    return 0;
}
