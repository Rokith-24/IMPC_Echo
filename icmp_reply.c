
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

/* Write timestamped messages to the log file */
void log_event(FILE *log, const char *message)
{
    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    char timestamp[30];

    strftime(timestamp, sizeof(timestamp),
             "%Y-%m-%d %H:%M:%S", local);

    fprintf(log, "[%s] %s\n", timestamp, message);
    fflush(log);
}

/* Calculate ICMP checksum */
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
    char message[256];

    /* Open server log file */
    FILE *log = fopen("server.log", "a");

    if (log == NULL)
    {
        perror("Unable to open server.log");
        return 1;
    }

    log_event(log, "Server Started");

    /* Create raw ICMP socket */
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

    if (sockfd < 0)
    {
        perror("Socket creation failed");
        log_event(log, "ERROR: Socket creation failed");
        fclose(log);
        return 1;
    }

    printf("ICMP Server Started\n");
    printf("Waiting for Echo Requests...\n");

    log_event(log, "Raw ICMP Socket Created Successfully");

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
            log_event(log, "ERROR: Failed to receive packet");
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

        /* Check whether packet is an Echo Request */
        if (icmp->type == ICMP_ECHO && icmp->code == 0)
        {
            const char *source_ip = inet_ntoa(src_addr.sin_addr);

            printf("\nEcho Request Received from %s\n",
                   source_ip);

            snprintf(message, sizeof(message),
                     "Echo Request Received from %s",
                     source_ip);
            log_event(log, message);

            snprintf(message, sizeof(message),
                     "ICMP Type: %d, Code: %d",
                     icmp->type, icmp->code);
            log_event(log, message);

            /* Convert Echo Request into Echo Reply */
            icmp->type = ICMP_ECHOREPLY;
            icmp->checksum = 0;

            icmp->checksum = checksum(icmp, icmp_len);

            /* Send Echo Reply */
            if (sendto(sockfd,
                       icmp,
                       icmp_len,
                       0,
                       (struct sockaddr *)&src_addr,
                       addr_len) < 0)
            {
                perror("sendto");

                snprintf(message, sizeof(message),
                         "ERROR: Failed to send Echo Reply to %s",
                         source_ip);
                log_event(log, message);
            }
            else
            {
                printf("Echo Reply Sent Successfully\n");

                snprintf(message, sizeof(message),
                         "Echo Reply Sent Successfully to %s",
                         source_ip);
                log_event(log, message);
            }
        }
    }

    close(sockfd);
    fclose(log);

    return 0;
}
