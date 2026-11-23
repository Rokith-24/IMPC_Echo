
/*
 * ICMP Echo Reply Server
 *
 * Compile: gcc icmp_reply.c -o icmp_reply
 * Run:     sudo ./icmp_reply
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
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

int main()
{
    int sockfd;

    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

    if (sockfd < 0)
    {
        perror("Socket creation failed");
        return 1;
    }

    printf("ICMP Reply Server Started\n");
    printf("Waiting for Echo Requests...\n");

    while (1)
    {
        uint32_t buffer[16384];

        struct sockaddr_in src_addr;
        socklen_t addr_len = sizeof(src_addr);

        int bytes = recvfrom(sockfd,
                             buffer,
                             sizeof(buffer),
                             0,
                             (struct sockaddr *)&src_addr,
                             &addr_len);

        if (bytes < 0)
        {
            perror("recvfrom");
            continue;
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

        int icmp_len = total_len - ip_header_len;

        struct icmphdr *icmp =
            (struct icmphdr *)
            ((unsigned char *)buffer + ip_header_len);

        if (icmp->type == ICMP_ECHO &&
            icmp->code == 0)
        {
            printf("\nEcho Request Received\n");

            printf("From: %s\n",
                   inet_ntoa(src_addr.sin_addr));

            /* Build Echo Reply */

            icmp->type = ICMP_ECHOREPLY;
            icmp->checksum = 0;

            icmp->checksum =
                checksum(icmp, icmp_len);

            /* Send Echo Reply */

            if (sendto(sockfd,
                       icmp,
                       icmp_len,
                       0,
                       (struct sockaddr *)&src_addr,
                       addr_len) < 0)
            {
                perror("sendto");
            }
            else
            {
                printf("Echo Reply Sent Successfully\n");
            }
        }
    }

    close(sockfd);

    return 0;
}
