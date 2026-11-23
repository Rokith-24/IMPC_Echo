
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
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

int main(int argc, char *argv[])
{
    int sockfd;
    char message[256];

    struct sockaddr_in dest_addr;
    struct icmphdr icmp_pkt;

    /* Open client log file */
    FILE *log = fopen("client.log", "a");

    if (log == NULL)
    {
        perror("Unable to open client.log");
        return 1;
    }

    log_event(log, "Client Started");

    /* Validate command-line argument */
    if (argc != 2)
    {
        printf("Usage: sudo %s <SERVER_IP>\n", argv[0]);

        log_event(log, "ERROR: Server IP not provided");
        fclose(log);
        return 1;
    }

    /* Create raw ICMP socket */
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

    if (sockfd < 0)
    {
        perror("Socket creation failed");
        log_event(log, "ERROR: Socket creation failed");
        fclose(log);
        return 1;
    }

    printf("Raw ICMP Socket Created Successfully\n");
    log_event(log, "Raw ICMP Socket Created Successfully");

    /* Configure destination address */
    memset(&dest_addr, 0, sizeof(dest_addr));

    dest_addr.sin_family = AF_INET;

    if (inet_pton(AF_INET, argv[1],
                  &dest_addr.sin_addr) != 1)
    {
        printf("Invalid Server IP Address\n");

        log_event(log, "ERROR: Invalid Server IP Address");

        close(sockfd);
        fclose(log);
        return 1;
    }

    snprintf(message, sizeof(message),
             "Destination IP: %s", argv[1]);
    log_event(log, message);

    /* Set receive timeout */
    struct timeval timeout;

    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
                   &timeout, sizeof(timeout)) < 0)
    {
        perror("setsockopt");

        log_event(log, "ERROR: Failed to set receive timeout");

        close(sockfd);
        fclose(log);
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

    log_event(log, "ICMP Echo Request Created");

    /* Send Echo Request */
    if (sendto(sockfd,
               &icmp_pkt,
               sizeof(icmp_pkt),
               0,
               (struct sockaddr *)&dest_addr,
               sizeof(dest_addr)) < 0)
    {
        perror("sendto");

        log_event(log, "ERROR: Failed to send Echo Request");

        close(sockfd);
        fclose(log);
        return 1;
    }

    printf("ICMP Echo Request Sent Successfully\n");

    snprintf(message, sizeof(message),
             "Echo Request Sent Successfully to %s",
             argv[1]);
    log_event(log, message);

    /* Receive Echo Reply */
    uint32_t buffer[16384];

    struct sockaddr_in from_addr;

    while (1)
    {
        socklen_t from_len = sizeof(from_addr);

        int bytes = recvfrom(sockfd,
                             buffer,
                             sizeof(buffer),
                             0,
                             (struct sockaddr *)&from_addr,
                             &from_len);

        if (bytes < 0)
        {
            perror("No Echo Reply received");

            log_event(log, "ERROR: No matching Echo Reply received (timeout or receive error)");

            close(sockfd);
            fclose(log);
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

        /* Validate matching Echo Reply */
        if (from_addr.sin_addr.s_addr ==
                dest_addr.sin_addr.s_addr &&
            reply->type == ICMP_ECHOREPLY &&
            reply->code == 0 &&
            ntohs(reply->un.echo.id) == identifier &&
            ntohs(reply->un.echo.sequence) == sequence)
        {
            printf("\nEcho Reply Received Successfully!\n");
            printf("From IP: %s\n",
                   inet_ntoa(from_addr.sin_addr));

            snprintf(message, sizeof(message),
                     "Echo Reply Received from %s",
                     inet_ntoa(from_addr.sin_addr));
            log_event(log, message);

            log_event(log, "Communication Successful");

            break;
        }
    }

    close(sockfd);
    fclose(log);

    return 0;
}
