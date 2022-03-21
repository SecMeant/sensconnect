#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVER_ADDRESS (INADDR_ANY)
#define SERVER_PORT 34756
#define MAX_PACKET_LEN 65536

int create_server_socket(uint32_t address, int port)
{
    struct sockaddr_in addr_us;
    int sock;

    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        return sock;
    }

    memset((void*) &addr_us, 0, sizeof(addr_us));
    addr_us.sin_family = AF_INET;
    addr_us.sin_port = htons(port);
    addr_us.sin_addr.s_addr = htonl(address);

    if (bind(sock, (struct sockaddr*)&addr_us, sizeof(addr_us)) == -1)
    {
        close(sock);
        return -1;
    }

    return sock;
}

int main(void)
{
    int serv_sock;
    char *packet_buff;
    ssize_t recv_len;

    struct sockaddr_in client;
    socklen_t client_addr_len;

    serv_sock = create_server_socket(INADDR_ANY, SERVER_PORT);

    if (serv_sock == -1) {
        perror("Failed to create server socket");
    }

    packet_buff = (char*) malloc(MAX_PACKET_LEN);

    while (1) {
        client_addr_len = sizeof(client);
        if ((recv_len = recvfrom(serv_sock, packet_buff, MAX_PACKET_LEN-1,
                                 0, (struct sockaddr *) &client,
                                 &client_addr_len)) == -1)
        {
            continue;
        }

        packet_buff[recv_len] = '\0';

        printf("Received packet from %s:%d\n",
               inet_ntoa(client.sin_addr), ntohs(client.sin_port));
        printf("Data: %s\n", packet_buff);
    }

    return 0;
}
