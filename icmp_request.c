
/*
 * ICMP Echo Request Client
 *
 * Compile: gcc icmp_request.c -o icmp_request
 * Run:     sudo ./icmp_request <SERVER_IP>
 *
 * Example: sudo ./icmp_request 192.168.1.10
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <stdint.h>

unsigned short checksum(void *data, int len)
{
    unsigned short *ptr = data;
    unsigned int sum = 0;

    while (len > 1)
    {
        sum += *ptr++;
        len -= 2;
    }

    if (len == 1)
        sum += *(unsigned char *)ptr;

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);

    return (unsigned short)(~sum);
}

int main(int argc, char *argv[])
{
    int sockfd;

    struct sockaddr_in dest_addr;
    struct icmphdr icmp_pkt;

    if (argc != 2)
    {
        printf("Usage: sudo %s <SERVER_IP>\n", argv[0]);
        return 1;
    }

    /* Create Raw Socket */

    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

    if (sockfd < 0)
    {
        perror("Socket creation failed");
        return 1;
    }

    printf("Raw ICMP Socket Created Successfully\n");

    /* Configure Destination IP */

    memset(&dest_addr, 0, sizeof(dest_addr));

    dest_addr.sin_family = AF_INET;

    if (inet_pton(AF_INET, argv[1],
                  &dest_addr.sin_addr) != 1)
    {
        printf("Invalid Server IP Address\n");
        close(sockfd);
        return 1;
    }

    /* Set Receive Timeout */

    struct timeval timeout;

    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
                   &timeout, sizeof(timeout)) < 0)
    {
        perror("setsockopt");
        close(sockfd);
        return 1;
    }

    /* Build ICMP Echo Request */

    memset(&icmp_pkt, 0, sizeof(icmp_pkt));

    unsigned short identifier = getpid() & 0xffff;
    unsigned short sequence = 1;

    icmp_pkt.type = ICMP_ECHO;
    icmp_pkt.code = 0;

    icmp_pkt.un.echo.id = htons(identifier);
    icmp_pkt.un.echo.sequence = htons(sequence);

    icmp_pkt.checksum = checksum(&icmp_pkt,
                                 sizeof(icmp_pkt));

    printf("ICMP Packet Created\n");
    printf("Type       : %d (Echo Request)\n",
           icmp_pkt.type);
    printf("Code       : %d\n",
           icmp_pkt.code);
    printf("Process ID : %u\n",
           identifier);

    /* Send Echo Request */

    if (sendto(sockfd,
               &icmp_pkt,
               sizeof(icmp_pkt),
               0,
               (struct sockaddr *)&dest_addr,
               sizeof(dest_addr)) < 0)
    {
        perror("sendto failed");
        close(sockfd);
        return 1;
    }

    printf("\nICMP Echo Request Sent Successfully\n");
    printf("Destination IP: %s\n", argv[1]);

    /* Receive Echo Reply */

    uint32_t buffer[16384];

    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);

    while (1)
    {
        int bytes = recvfrom(sockfd,
                             buffer,
                             sizeof(buffer),
                             0,
                             (struct sockaddr *)&from_addr,
                             &from_len);

        if (bytes < 0)
        {
            perror("No Echo Reply received");
            close(sockfd);
            return 1;
        }

        if (bytes < (int)sizeof(struct iphdr))
            continue;

        struct iphdr *ip = (struct iphdr *)buffer;

        int ip_header_len = ip->ihl * 4;

        if (ip->version != 4 ||
            ip->ihl < 5 ||
            ip_header_len > bytes)
            continue;

        int total_len = ntohs(ip->tot_len);

        if (total_len < ip_header_len +
                        (int)sizeof(struct icmphdr) ||
            total_len > bytes)
            continue;

        struct icmphdr *reply =
            (struct icmphdr *)
            ((unsigned char *)buffer + ip_header_len);

        if (from_addr.sin_addr.s_addr ==
                dest_addr.sin_addr.s_addr &&
            reply->type == ICMP_ECHOREPLY &&
            reply->code == 0 &&
            ntohs(reply->un.echo.id) == identifier &&
            ntohs(reply->un.echo.sequence) == sequence)
        {
            printf("\nEcho Reply Received Successfully!\n");

            printf("From IP : %s\n",
                   inet_ntoa(from_addr.sin_addr));

            printf("Type    : %d (Echo Reply)\n",
                   reply->type);

            printf("Code    : %d\n",
                   reply->code);

            printf("Sequence: %u\n",
                   ntohs(reply->un.echo.sequence));

            break;
        }
    }

    close(sockfd);

    return 0;
}
