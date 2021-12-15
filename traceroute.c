#include <unistd.h>
#include <stdio.h> 
#include <string.h>
#include <sys/socket.h>  
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/time.h>
unsigned short checksum (unsigned short *data, int len)
{
    unsigned short *w = data;
    int nleft = len;
    int sum = 0;
    unsigned short chksum=0;

    while (nleft > 1)  {
        sum += *w++;
        nleft -= 2;
    }

    if (nleft == 1) {
        *(unsigned char *)(&chksum) = *(unsigned char *)w;
        sum += chksum;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    chksum = (unsigned short)(~sum);

    return chksum;
}
//main takes website you wish to trace the route of
void main(int argc, char* argv[])
{
    if(argc < 2){ //if no website is provided
        printf("please provide a host name\n");
        return;
    }
    char *hostname = argv[1];

    struct hostent *host;
    host = gethostbyname(hostname);
    if(host == NULL){ //check if website is found
        printf("host name not found\n");
        return;
    }
    char ip[NI_MAXHOST];
    strcpy(ip, inet_ntoa(*(struct in_addr*)host->h_addr));

    printf("Host IP = %s\n", ip);

    int sock; // setting up raw socket
    if((sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0){
        perror("Socket error\n");
        return;
    }
    //make recvfrom stop listening for messages after 2 seconds have passed
    struct timeval tv;
    tv.tv_usec = 0;
    tv.tv_sec = 2;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    int ttl = 1; //time to live starts at 1 hop will increase everytime loop runs so it can reach the next destination
    while (1) {
        printf("Hop #: %d\n", ttl);
        struct sockaddr_in dest_addr;
        memset(&dest_addr, 0x00, sizeof(dest_addr));
        dest_addr.sin_family = AF_INET; // indicates address family is ip4
        dest_addr.sin_addr.s_addr = inet_addr(ip); //set up ip address for the socket
        dest_addr.sin_port = htons(0); //operating system assigns port

        char packet[64 * sizeof(char)];
        memset(packet, 0x00, sizeof(packet));
        struct icmp* hdr = (struct icmp*)packet; //setup icmp header
        hdr->icmp_type = ICMP_ECHO; //ECHO makes router send data back to us as an ECHO reply
        hdr->icmp_code = 0;
        //hdr->icmp_cksum = 0;
        hdr->icmp_seq = 0;
        hdr->icmp_id = getpid();// sets header id to the proccess id

        hdr->icmp_cksum = checksum((unsigned short *) packet, sizeof(packet));
        //printf("pid: %d\n", getpid());
        //starting a timer to see how long it takes to get echo back to us
        struct timeval start;
        gettimeofday(&start, NULL);
        setsockopt(sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));

        //send our packet
        sendto(sock, packet, sizeof(packet), 0, (struct sockaddr*) &dest_addr, sizeof(dest_addr));
        struct sockaddr_in resp_addr;
        memset(&resp_addr, 0x00, sizeof(resp_addr));
        socklen_t addr_len = sizeof(resp_addr);

        char recv[4096];

        int n;
        //get echo response
        n = recvfrom(sock, recv, sizeof(recv), 0, (struct sockaddr*) &resp_addr, &addr_len);

        if (n < 0) { // make sure response is valid
            printf("recvfrom error\n");
        } else {
            //since we got a valid response we can stop the timer
            struct timeval end;
            gettimeofday(&end, NULL);
            printf("%02x %02x %02x %02x\n", recv[0], recv[1], recv[2], recv[3]);
            //get the data from the response
            struct ip *ip4 = (struct ip*)recv;
            //printf("hl: %d\n", ip4->ip_hl);
            //printf("v: %d\n", ip4->ip_v);
            struct icmp *recv_hdr = (struct icmp *) (recv + (ip4->ip_hl << 2));
            printf("type: %d\n", recv_hdr->icmp_type); //if this is zero we know we've reached the end
            //printf("id: %d\n", recv_hdr->icmp_id);
            printf("ip: %s\n", inet_ntoa(resp_addr.sin_addr));
            //calculate how long it took
            struct timeval diff;
            timersub(&end, &start, &diff);
            printf("it took %ld ms\n", diff.tv_usec / 1000);

            //get info about the router we reached
            char source_host[NI_MAXHOST];
            int s = getnameinfo((struct sockaddr*) &resp_addr, addr_len, source_host, sizeof(source_host), NULL, 0, NI_NOFQDN);
            if(s == 0){
                printf("Host Name %s\n", source_host);
            }
            printf("=================================\n");
            if(recv_hdr->icmp_type==0){ //check if we reached the end of the route
                printf("destination reached it took %d hops\n", ttl);
                break;
            }
            ttl++; //increase time to live so we can reach the next furthest router
        }
        if(ttl == 30){
            break;
        }


    }


    close(sock);


}
