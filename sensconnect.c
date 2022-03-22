#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <alloca.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVER_ADDRESS (INADDR_ANY)
#define SERVER_PORT 34756
#define MAX_PACKET_LEN 65536

#define PROTO_DEVREG 0
#define PROTO_SENSDATA 1
#define PROTO_GETSUM 2

#define RESP_SUCCESS 0
#define RESP_BAD_PARAM 1
#define RESP_BAD_DEV 2

__attribute__((noreturn)) void internal_error(int errc)
{
    fprintf(stderr, "Internal error: %i", errc);
    exit(errc);
}

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

struct resp_status
{
    uint8_t code; // 0 means success
    char extra_str[64]; // Extra, null terminated text message
} __attribute__((packed));

void resp_status(int s, struct sockaddr_in *c, struct resp_status msg)
{
    memset(&msg, 0, sizeof(msg));

    if (sendto(s, &msg, sizeof(msg), 0, (struct sockaddr *) c, sizeof(struct sockaddr_in)) != sizeof(msg)) {
        fprintf(stderr, "Warning: Failed to send response to client at %s:%d\n",
                inet_ntoa(c->sin_addr), ntohs(c->sin_port));
    }
}

struct entry_dev
{
    char *name;
    size_t name_len;
    char *field_names[8];
    size_t field_names_len[8];
    int32_t field_values[8];
};

struct list_head
{
    struct list_head *next;
};

struct node_device
{
    struct list_head head;
    struct entry_dev e_dev;
} g_devs;

void* alloc_new_str(uint8_t *data, size_t data_len, size_t *real_data_len)
{
    char * ret = NULL;

    if (data[data_len] != 0)
        data_len += 1;

    ret = malloc(data_len);
    memcpy(ret, data, data_len);

    *real_data_len = data_len;

    return ret;
}

struct node_device *search_device(uint8_t *name_data, size_t data_len)
{
    size_t real_name_sz = data_len;
    if (name_data[data_len-1] != '\0')
        real_name_sz += 1;

    char name[real_name_sz];
    memcpy(name, name_data, data_len);
    name[real_name_sz - 1] = '\0';

    for (struct node_device *head = &g_devs; head; head = (struct node_device *) head->head.next) {
        if (strcmp(head->e_dev.name, name) == 0)
            return head;
    }

    return NULL;
}

// Returns field id which matches the name. -1 otherwise.
int search_field(struct node_device *dev, uint8_t *name_data, size_t data_len)
{
    size_t real_name_sz = data_len;
    if (name_data[data_len-1] != '\0')
        real_name_sz += 1;

    char name[real_name_sz];
    memcpy(name, name_data, data_len);
    name[real_name_sz - 1] = '\0';

    for (int i = 0; i < 8; ++i) {
        if (strcmp(dev->e_dev.field_names[i], name) == 0)
            return i;
    }

    return -1;
}

void serialize_field(char *outbuf, struct node_device *dev, int field_id)
{
    memcpy(outbuf, dev->e_dev.field_names[field_id], dev->e_dev.field_names_len[field_id]);
    outbuf += dev->e_dev.field_names_len[field_id];

    *outbuf = '\0';
    outbuf += 1;

    memcpy(outbuf, &dev->e_dev.field_values[field_id], sizeof(dev->e_dev.field_values[field_id]));
    outbuf += sizeof(dev->e_dev.field_values[field_id]);
}

int handle_packet(int s, struct sockaddr_in *client, uint8_t *packet_buff, ssize_t packet_len)
{
    struct resp_status msg;

    if (packet_len > MAX_PACKET_LEN)
        internal_error(0x0A);

    switch (packet_buff[0])
    {

        /*
         * @packet DEVREG - Register new device
         *
         * uint8_t cmd = PROTO_DEVREG;
         * uint16_t namelen;
         * char name[namelen]; // VLA
         * uint8_t field_count;
         *
         * struct { uint8_t len; char data[len] }
         * field_names [field_count]; // VLA of structures
         */
        case PROTO_DEVREG:
        {
            struct node_device *ndev = malloc(sizeof(struct node_device));
            uint16_t namelen = (((uint16_t) packet_buff[2]) << 8) | ((uint16_t) packet_buff[1]);
            uint8_t *name = &packet_buff[3];

            if (namelen > packet_len - 3)
            {
                msg.code = RESP_BAD_PARAM;
                strncpy(msg.extra_str, "Reported name is too long!", 64);
                resp_status(s, client, msg);
                break;
            }

            {
                size_t real_data_len = 0;
                ndev->e_dev.name = alloc_new_str(name, namelen, &real_data_len);
                ndev->e_dev.name_len = real_data_len;
            }

            uint8_t field_count = packet_buff[1 + 2 + namelen];
            if (field_count > 8)
            {
                msg.code = RESP_BAD_PARAM;
                strncpy(msg.extra_str, "We only support up to 8 fields! Sorry ;(", 64);
                resp_status(s, client, msg);
                break;
            }

            uint8_t *fields = &packet_buff[1 + 2 + namelen + 1];
            for (int i = 0; i < field_count && fields < (packet_buff + packet_len); ++i, fields += fields[0])
            {
                size_t real_data_len = 0;
                ndev->e_dev.field_names[i] = alloc_new_str(&fields[1], fields[0], &real_data_len);
                ndev->e_dev.field_names_len[i] = real_data_len;
            }

            // TODO handle failures in the middle (free malloced node_device).

            msg.code = RESP_SUCCESS;
            resp_status(s, client, msg);
            puts("DEVREG success");

            break;
        }

        /*
         * @packet SENSDATA - Update sensor data
         *
         * uint8_t cmd = PROTO_SENSDATA;
         * uint16_t namelen;
         * char name[namelen]; // VLA
         * uint8_t field_name_len;
         * char field_name[field_name_len];
         * int32_t value;
         */
        case PROTO_SENSDATA:
        {
            uint16_t namelen = (((uint16_t) packet_buff[2]) << 8) | ((uint16_t) packet_buff[1]);
            uint8_t *name = &packet_buff[3];

            if (namelen > packet_len - 3)
            {
                msg.code = RESP_BAD_PARAM;
                strncpy(msg.extra_str, "Reported name is too long!", 64);
                resp_status(s, client, msg);
                break;
            }

            struct node_device *dev = search_device(name, namelen);

            if (!dev)
            {
                msg.code = RESP_BAD_DEV;
                strncpy(msg.extra_str, "Unknown device", 64);
                resp_status(s, client, msg);
                break;
            }

            uint8_t field_name_len = packet_buff[1 + 2 + namelen];
            uint8_t *field_name = &packet_buff[1 + 2 + namelen + 1];

            if (field_name + field_name_len >= packet_buff + packet_len)
            {
                msg.code = RESP_BAD_PARAM;
                strncpy(msg.extra_str, "Reported field name is too long!", 64);
                resp_status(s, client, msg);
                break;
            }

            int field_id = search_field(dev, field_name, field_name_len);

            if (field_id == -1)
            {
                msg.code = RESP_BAD_DEV;
                strncpy(msg.extra_str, "Unknown field", 64);
                resp_status(s, client, msg);
                break;
            }

            int32_t value;
            memcpy(&value, field_name + field_name_len, 4);

            dev->e_dev.field_values[field_id] = value;

            msg.code = RESP_SUCCESS;
            resp_status(s, client, msg);

            puts("SENSDATA success");

            break;
        }

        /*
         * @packet GETSUM - Get data summary
         *
         * uint8_t cmd = PROTO_SENSDATA;
         * uint16_t namelen;
         * char name[namelen]; // VLA
         * uint8_t field_name_len;
         * char field_name[field_name_len];
         */
        case PROTO_GETSUM:
        {
            uint16_t namelen = (((uint16_t) packet_buff[2]) << 8) | ((uint16_t) packet_buff[1]);
            uint8_t *name = &packet_buff[3];

            if (namelen > packet_len - 3)
            {
                msg.code = RESP_BAD_PARAM;
                strncpy(msg.extra_str, "Reported name is too long!", 64);
                resp_status(s, client, msg);
                break;
            }

            struct node_device *dev = search_device(name, namelen);

            if (!dev)
            {
                msg.code = RESP_BAD_DEV;
                strncpy(msg.extra_str, "Unknown device", 64);
                resp_status(s, client, msg);
                break;
            }

            uint8_t field_name_len = packet_buff[1 + 2 + namelen];
            uint8_t *field_name = &packet_buff[1 + 2 + namelen + 1];

            if (field_name + field_name_len >= packet_buff + packet_len)
            {
                msg.code = RESP_BAD_PARAM;
                strncpy(msg.extra_str, "Reported field name is too long!", 64);
                resp_status(s, client, msg);
                break;
            }

            int field_id = -1;

            for (int i = 0; i < 8; ++i) {
                if (strcmp((const char *) field_name, dev->e_dev.field_names[i]) == 0) {
                    field_id = i;
                    break;
                }
            }

            if (field_id == -1) {
                msg.code = RESP_BAD_PARAM;
                strncpy(msg.extra_str, "No such field", 64);
                resp_status(s, client, msg);
                break;
            }

            size_t resp_sz = strlen((const char *) field_name);
            resp_sz += 2; // For ": "
            resp_sz += 10; // Max length of formated int32
            resp_sz += 1; // nullbyte

            if (resp_sz > sizeof(msg.extra_str))
            {
                msg.code = RESP_BAD_PARAM;
                strncpy(msg.extra_str, "TODO: Implement longer field names lmao", 64);
                resp_status(s, client, msg);
                break;
            }

            char resp_buffer[resp_sz];
            serialize_field(resp_buffer, dev, field_id);

            msg.code = RESP_SUCCESS;
            strncpy(msg.extra_str, resp_buffer, 64);
            resp_status(s, client, msg);

            puts("GETSUM success");

            break;
        }

        default:
            msg.code = RESP_BAD_PARAM;
            strncpy(msg.extra_str, "Unknown packet.", 64);
            resp_status(s, client, msg);
            break;
    }
    return 0;
}

int main(void)
{
    int serv_sock;
    uint8_t *packet_buff;
    ssize_t recv_len;

    struct sockaddr_in client;
    socklen_t client_addr_len;

    serv_sock = create_server_socket(INADDR_ANY, SERVER_PORT);

    if (serv_sock == -1) {
        perror("Failed to create server socket");
    }

    packet_buff = malloc(MAX_PACKET_LEN);

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

        handle_packet(serv_sock, &client, packet_buff, recv_len);
    }

    return 0;
}
