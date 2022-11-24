//
// Created by Calvin Ren on 2022/11/23.
//

#ifndef COMPUTER_NETWORK_PROJ_PORT_CHAT_H
#define COMPUTER_NETWORK_PROJ_PORT_CHAT_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>           // close()
#include <string.h>           // strcpy, memset(), and memcpy()
#include <netdb.h>            // struct addrinfo
#include <sys/types.h>        // needed for socket(), uint8_t, uint16_t, uint32_t
#include <sys/socket.h>       // needed for socket()
#include <netinet/in.h>       // IPPROTO_UDP, INET_ADDRSTRLEN
#include <netinet/ip.h>       // struct ip and IP_MAXPACKET (which is 65535)
#include <netinet/udp.h>      // struct udphdr
#include <arpa/inet.h>        // inet_pton() and inet_ntop()
#include <sys/ioctl.h>        // macro ioctl is defined
#include <bits/ioctls.h>      // defines values for argument "request" of ioctl.
#include <net/if.h>           // struct ifreq
#include <linux/if_ether.h>   // ETH_P_IP = 0x0800, ETH_P_IPV6 = 0x86DD
#include <linux/if_packet.h>  // struct sockaddr_ll (see man 7 packet)
#include <net/ethernet.h>
#include <errno.h>            // errno, perror()

/* Constants Define */
#define ETH_HDRLEN 14         // Ethernet header length
#define IP4_HDRLEN 20         // IPv4 header length
#define UDP_HDRLEN  8         // UDP header length, excludes data
#define MTU_SIZE 1            // MTU size

/* Function Prototypes*/
uint16_t checksum (uint16_t *, int);
uint16_t udp4_checksum (struct ip, struct udphdr, uint8_t *, int);
char *allocate_strmem (int);
uint8_t *allocate_ustrmem (int);
uint8_t out_buf[1024];
int *allocate_intmem (int);
short seq_num;

/* Function Definitions */
// Sender Main Function
int sender(short src_port, short dst_port) {
    int i, status, frame_length, sd, nsd, bytes, datalen, *ip_flags;
    char *interface, *target, *src_ip, *dst_ip, *send_data;
    char input_buf[1024];
    struct ip iphdr;
    struct udphdr udphdr;
    uint8_t *data, *src_mac, *dst_mac, *ether_frame;
    struct addrinfo hints, *res;
    struct sockaddr_in *ipv4;
    struct sockaddr_ll device;
    struct ifreq ifr;
    void *tmp;

    // Allocate memory for various arrays.
    src_mac = allocate_ustrmem(6);
    dst_mac = allocate_ustrmem(6);
    data = allocate_ustrmem(IP_MAXPACKET);
    ether_frame = allocate_ustrmem(IP_MAXPACKET);
    interface = allocate_strmem(40);
    target = allocate_strmem(40);
    src_ip = allocate_strmem(INET_ADDRSTRLEN);
    dst_ip = allocate_strmem(INET_ADDRSTRLEN);
    ip_flags = allocate_intmem(4);

    // Interface to send packet through.
    strcpy(interface, "lo");

    // Submit request for a socket descriptor to look up interface.
    if ((nsd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
        perror ("socket() failed to get socket descriptor for using ioctl() ");
        exit (EXIT_FAILURE);
    }

    // Use ioctl() to look up interface name and get its MAC address.
    memset (&ifr, 0, sizeof (ifr));
    snprintf (ifr.ifr_name, sizeof (ifr.ifr_name), "%s", interface);
    if (ioctl (nsd, SIOCGIFHWADDR, &ifr) < 0) {
        perror ("ioctl() failed to get source MAC address ");
        return (EXIT_FAILURE);
    }

    // Copy source MAC address.
    memcpy (src_mac, ifr.ifr_hwaddr.sa_data, 6 * sizeof (uint8_t));

    // Report source MAC address to stdout.
    printf("InterFace Name: %s\n", interface);
    printf("Host MAC Address:");
    for (i=0; i<5; i++) {
        printf ("%02x:", src_mac[i]);
    }
    printf ("%02x\n", src_mac[5]);

    if (ioctl(nsd, SIOCGIFADDR, &ifr) < 0)
    {
        perror("can't get ip address");
        exit(1);
    }
    strcpy(src_ip, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
    // Copy source IPv4 address
    printf("Host IP Address: %s\n", src_ip);

    close(nsd);

    // Find interface index from interface name and store index in
    // struct sockaddr_ll device, which will be used as an argument of sendto().
    memset(&device, 0, sizeof(device));
    if ((device.sll_ifindex = if_nametoindex(interface)) == 0)
    {
        perror("if_nametoindex() failed to obtain interface index ");
        exit(EXIT_FAILURE);
    }
    printf("Index For InterFace %s: %i\n", interface, device.sll_ifindex);
    printf("=========================================================\n");
    printf("ATTENTION: Type Message or Press Ctrl+C to Exit\n\n");

    // Set destination MAC address: you need to fill these out
    dst_mac[0] = 0xff;
    dst_mac[1] = 0xff;
    dst_mac[2] = 0xff;
    dst_mac[3] = 0xff;
    dst_mac[4] = 0xff;
    dst_mac[5] = 0xff;
    // memcpy(dst_mac, dst_mac, 6 * sizeof(uint8_t));
    // Destination URL or IPv4 address: you need to fill this out
    strcpy(target, src_ip);

    // Fill out hints for getaddrinfo().
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = hints.ai_flags | AI_CANONNAME;

    // Resolve target using getaddrinfo().
    if ((status = getaddrinfo(target, NULL, &hints, &res)) != 0)
    {
        fprintf(stderr, "getaddrinfo() failed: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }
    ipv4 = (struct sockaddr_in *)res->ai_addr;
    tmp = &(ipv4->sin_addr);
    if (inet_ntop(AF_INET, tmp, dst_ip, INET_ADDRSTRLEN) == NULL)
    {
        status = errno;
        fprintf(stderr, "inet_ntop() failed.\nError message: %s", strerror(status));
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(res);

    // Fill out sockaddr_ll.
    device.sll_family = AF_PACKET;
    memcpy(device.sll_addr, src_mac, 6 * sizeof(uint8_t));
    device.sll_halen = 6;

    while (1)
    {
        memset(input_buf, 0, 1024);
        fgets(input_buf, 1024, stdin);
        int msg_len = strlen(input_buf);
        for (int i = 0; i < msg_len; i = i + MTU_SIZE)
        {
            // Copy UDP data
            int idx;
            send_data = input_buf + i;
            datalen = MTU_SIZE;
            for (idx = 0; idx < datalen; idx++)
            {
                data[idx] = send_data[idx];
            }
            /* =========================== Network Layer =========================== */

            // IPv4 header

            // IPv4 header length (4 bits): Number of 32-bit words in header = 5
            iphdr.ip_hl = IP4_HDRLEN / sizeof(uint32_t);

            // Internet Protocol version (4 bits): IPv4
            iphdr.ip_v = 4;

            // Type of service (8 bits)
            iphdr.ip_tos = 0;

            // Total length of datagram (16 bits): IP header + UDP header + datalen
            iphdr.ip_len = htons(IP4_HDRLEN + UDP_HDRLEN + datalen);

            // ID sequence number (16 bits): unused, since single datagram
            iphdr.ip_id = htons(0);

            // Flags, and Fragmentation offset (3, 13 bits): 0 since single datagram

            // Zero (1 bit)
            ip_flags[0] = 0;

            // Do not fragment flag (1 bit)
            ip_flags[1] = 0;

            // More fragments following flag (1 bit)
            ip_flags[2] = (i + MTU_SIZE < msg_len);

            // Fragmentation offset (13 bits)
            ip_flags[3] = i / MTU_SIZE;

            iphdr.ip_off = htons((ip_flags[0] << 15) + (ip_flags[1] << 14) + (ip_flags[2] << 13) + ip_flags[3]);

            // Time-to-Live (8 bits): default to maximum value
            iphdr.ip_ttl = 255;

            // Transport layer protocol (8 bits): 17 for UDP
            iphdr.ip_p = IPPROTO_UDP;

            // Source IPv4 address (32 bits)
            if ((status = inet_pton(AF_INET, src_ip, &(iphdr.ip_src))) != 1)
            {
                fprintf(stderr, "inet_pton() failed.\nError message: %s", strerror(status));
                exit(EXIT_FAILURE);
            }

            // Destination IPv4 address (32 bits)
            if ((status = inet_pton(AF_INET, dst_ip, &(iphdr.ip_dst))) != 1)
            {
                fprintf(stderr, "inet_pton() failed.\nError message: %s", strerror(status));
                exit(EXIT_FAILURE);
            }

            // IPv4 header checksum (16 bits): set to 0 when calculating checksum
            iphdr.ip_sum = 0;
            iphdr.ip_sum = checksum((uint16_t *)&iphdr, IP4_HDRLEN);

            /* =========================== Transport Layer =========================== */

            // UDP header

            // Source port number (16 bits): pick a number
            udphdr.source = htons(src_port);

            // Destination port number (16 bits): pick a number
            udphdr.dest = htons(dst_port);

            // Length of UDP datagram (16 bits): UDP header + UDP data
            udphdr.len = htons(UDP_HDRLEN + datalen);

            // UDP checksum (16 bits)
            udphdr.check = udp4_checksum(iphdr, udphdr, data, datalen);

            // Fill out ethernet frame header.

            // Ethernet frame length = ethernet header (MAC + MAC + ethernet type) + ethernet data (IP header + UDP header + UDP data)
            frame_length = 6 + 6 + 2 + IP4_HDRLEN + UDP_HDRLEN + datalen;

            // Destination and Source MAC addresses
            memcpy(ether_frame, dst_mac, 6 * sizeof(uint8_t));
            memcpy(ether_frame + 6, src_mac, 6 * sizeof(uint8_t));

            // Next is ethernet type code (ETH_P_IP for IPv4).
            // http://www.iana.org/assignments/ethernet-numbers
            ether_frame[12] = ETH_P_IP / 256;
            ether_frame[13] = ETH_P_IP % 256;


            // Next is ethernet frame data (IPv4 header + UDP header + UDP data).

            // IPv4 header
            memcpy(ether_frame + ETH_HDRLEN, &iphdr, IP4_HDRLEN * sizeof(uint8_t));

            // UDP header
            memcpy(ether_frame + ETH_HDRLEN + IP4_HDRLEN, &udphdr, UDP_HDRLEN * sizeof(uint8_t));

            // UDP data
            memcpy(ether_frame + ETH_HDRLEN + IP4_HDRLEN + UDP_HDRLEN, data, datalen * sizeof(uint8_t));

            // Submit request for a raw socket descriptor.
            if ((sd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0)
            {
                perror("socket() failed ");
                exit(EXIT_FAILURE);
            }

            // Send ethernet frame to socket.
            if ((bytes = sendto(sd, ether_frame, frame_length, 0, (struct sockaddr *)&device, sizeof(device))) <= 0)
            {
                perror("sendto() failed");
                exit(EXIT_FAILURE);
            }
        }
    }

    // Close socket descriptor.
    close(sd);

    // Free allocated memory.
    free(src_mac);
    free(dst_mac);
    free(data);
    free(ether_frame);
    free(interface);
    free(target);
    free(src_ip);
    free(dst_ip);
    free(ip_flags);

    return (EXIT_SUCCESS);
}

/* Receiver  Functions */
void error(const char *msg) {
    perror(msg);
    exit(1);
}

void print_mac(uint8_t *mac) {
    int i = 0;
    for (i = 0; i < 5; i++) {
        printf("%02x:", mac[i]);
    }
    printf("%02x\n", mac[5]);
}

void print_buf(uint8_t* buf, int len) {
    int i = 0;
    for (i = 0; i < len; i++) {
        printf("%c", buf[i]);
    }
}

void print_raw_buf(uint8_t *buf) {
    int i = 0;
    for (i = 0; i < 60; i++) {
        printf("%02x:", buf[i]);
    }
    printf("%02x\n", buf[60]);
}

int recv_udp(uint8_t* frame, int portno, int datalen, int not_frag, int has_more_frag, int offset) {
    struct udphdr udph;
    memcpy(&udph, frame, sizeof(udph));
    // Only parsing the packet to specified port.
    if (ntohs(udph.uh_dport) == (uint16_t)portno) {
        if (not_frag) {
            print_buf(frame + sizeof(struct udphdr), datalen);
        }
        else if (has_more_frag) {
            memcpy(out_buf + offset * datalen, frame + sizeof(struct udphdr), datalen);
        }
        else if (seq_num != offset) {
            printf("[Message](PORT %d): ", ntohs(udph.uh_sport));
            memcpy(out_buf + offset * datalen, frame + sizeof(struct udphdr), datalen);
            print_buf(out_buf, (offset + 1) * datalen);
            seq_num = 0;
        }
    }

    return ntohs(udph.uh_dport) == portno;
}

int recv_ip(uint8_t *frame, int portno) {
    struct ip iph;
    memcpy(&iph, frame, sizeof(iph));
    if (iph.ip_p == IPPROTO_UDP) {
        unsigned short frag_field = ntohs(iph.ip_off);
        // Record ip packet seq number


        // Do not fragment flag (1 bit)
        int not_frag = (frag_field << 1) >> 15;

        // More fragments following flag (1 bit)
        int has_more_frag = (frag_field << 2) >> 15;

        // Fragmentation offset (13 bits)
        short offset = frag_field << 3;
        offset = offset >> 3;
        recv_udp(frame + 20 * sizeof(uint8_t), portno, ntohs(iph.ip_len) - sizeof(struct udphdr),
                 not_frag, has_more_frag, offset);
        seq_num = offset;
    }
    return 0;
}

int recv_eth(int sock, int portno) {
    int n;
    uint8_t * buffer;
    buffer = allocate_ustrmem(256);
    memset(buffer, 0, 256 * sizeof(uint8_t));
    n = read(sock, buffer, 255);
    if (n < 0) {
        error("ERROR reading from socket");
        free(buffer);
        return -1;
    }
    else {
        struct ethhdr ethh;
        memcpy(&ethh, buffer, sizeof(struct ethhdr));
        // Only parsing IP packet
        if (ethh.h_proto == htons(ETH_P_IP)) {
            recv_ip(buffer + sizeof(ethh), portno);
        }
        free(buffer);
        return n;
    }
}

/* Receiver Main Function */
void* receiver(void *arg) {
    struct sockaddr_in serv_addr;
    struct ifreq ifr;
    int i, sd, portno;
    char *interface;

    portno = *(int *)arg;
    interface = allocate_strmem(40);

    int tsd;
    strcpy(interface, "lo");
    // Submit request for a socket descriptor to look up interface.
    if ((tsd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
        perror("socket() failed to get socket descriptor for using ioctl() ");
        exit(EXIT_FAILURE);
    }

    // Use ioctl() to look up interface name and get its MAC address.
    memset(&ifr, 0, sizeof(ifr));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", interface);

    // Initialize server socket.
    if ((sd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
        perror("socket() failed ");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof(sll));
    sll.sll_family = AF_PACKET;
    if (ioctl(sd, SIOCGIFINDEX, &ifr) == 0) {
        sll.sll_ifindex = ifr.ifr_ifindex;
        sll.sll_protocol = htons(ETH_P_ALL);
    }
    if (bind(sd, (struct sockaddr *)&sll, sizeof(sll)) < 0)
        error("ERROR on binding");

    while (recv_eth(sd, portno) > 0) {

    } /* end of while */
    close(sd);
    // Free allocated memory.
    free(interface);
}


// Computing the internet checksum (RFC 1071).
// Note that the internet checksum is not guaranteed to preclude collisions.
uint16_t checksum (uint16_t *addr, int len) {
    int count = len;
    register uint32_t sum = 0;
    uint16_t answer = 0;

    // Sum up 2-byte values until none or only one byte left.
    while (count > 1) {
        sum += *(addr++);
        count -= 2;
    }

    // Add left-over byte, if any.
    if (count > 0) {
        sum += *(uint8_t *) addr;
    }

    // Fold 32-bit sum into 16 bits; we lose information by doing this,
    // increasing the chances of a collision.
    // sum = (lower 16 bits) + (upper 16 bits shifted right 16 bits)
    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    // Checksum is one's compliment of sum.
    answer = ~sum;

    return (answer);
}

// Build IPv4 UDP pseudo-header and call checksum function.
uint16_t udp4_checksum (struct ip iphdr, struct udphdr udphdr, uint8_t *payload, int payloadlen) {
    char buf[IP_MAXPACKET];
    char *ptr;
    int i, chksumlen = 0;

    memset (buf, 0, IP_MAXPACKET * sizeof (uint8_t));

    ptr = &buf[0];  // ptr points to beginning of buffer buf

    // Copy source IP address into buf (32 bits)
    memcpy (ptr, &iphdr.ip_src.s_addr, sizeof (iphdr.ip_src.s_addr));
    ptr += sizeof (iphdr.ip_src.s_addr);
    chksumlen += sizeof (iphdr.ip_src.s_addr);

    // Copy destination IP address into buf (32 bits)
    memcpy (ptr, &iphdr.ip_dst.s_addr, sizeof (iphdr.ip_dst.s_addr));
    ptr += sizeof (iphdr.ip_dst.s_addr);
    chksumlen += sizeof (iphdr.ip_dst.s_addr);

    // Copy zero field to buf (8 bits)
    *ptr = 0; ptr++;
    chksumlen += 1;

    // Copy transport layer protocol to buf (8 bits)
    memcpy (ptr, &iphdr.ip_p, sizeof (iphdr.ip_p));
    ptr += sizeof (iphdr.ip_p);
    chksumlen += sizeof (iphdr.ip_p);

    // Copy UDP length to buf (16 bits)
    memcpy (ptr, &udphdr.len, sizeof (udphdr.len));
    ptr += sizeof (udphdr.len);
    chksumlen += sizeof (udphdr.len);

    // Copy UDP source port to buf (16 bits)
    memcpy (ptr, &udphdr.source, sizeof (udphdr.source));
    ptr += sizeof (udphdr.source);
    chksumlen += sizeof (udphdr.source);

    // Copy UDP destination port to buf (16 bits)
    memcpy (ptr, &udphdr.dest, sizeof (udphdr.dest));
    ptr += sizeof (udphdr.dest);
    chksumlen += sizeof (udphdr.dest);

    // Copy UDP length again to buf (16 bits)
    memcpy (ptr, &udphdr.len, sizeof (udphdr.len));
    ptr += sizeof (udphdr.len);
    chksumlen += sizeof (udphdr.len);

    // Copy UDP checksum to buf (16 bits)
    // Zero, since we don't know it yet
    *ptr = 0; ptr++;
    *ptr = 0; ptr++;
    chksumlen += 2;

    // Copy payload to buf
    memcpy (ptr, payload, payloadlen);
    ptr += payloadlen;
    chksumlen += payloadlen;

    // Pad to the next 16-bit boundary
    i = 0;
    while (((payloadlen+i)%2) != 0) {
        i++;
        chksumlen++;
        ptr++;
    }

    return checksum ((uint16_t *) buf, chksumlen);
}

// Allocate memory for an array of chars.
char * allocate_strmem (int len) {
    void *tmp;

    if (len <= 0) {
        fprintf (stderr, "ERROR: Cannot allocate memory because len = %i in allocate_strmem().\n", len);
        exit (EXIT_FAILURE);
    }

    tmp = (char *) malloc (len * sizeof (char));
    if (tmp != NULL) {
        memset (tmp, 0, len * sizeof (char));
        return (tmp);
    } else {
        fprintf (stderr, "ERROR: Cannot allocate memory for array allocate_strmem().\n");
        exit (EXIT_FAILURE);
    }
}

// Allocate memory for an array of unsigned chars.
uint8_t * allocate_ustrmem (int len) {
    void *tmp;

    if (len <= 0) {
        fprintf (stderr, "ERROR: Cannot allocate memory because len = %i in allocate_ustrmem().\n", len);
        exit (EXIT_FAILURE);
    }

    tmp = (uint8_t *) malloc (len * sizeof (uint8_t));
    if (tmp != NULL) {
        memset (tmp, 0, len * sizeof (uint8_t));
        return (tmp);
    } else {
        fprintf (stderr, "ERROR: Cannot allocate memory for array allocate_ustrmem().\n");
        exit (EXIT_FAILURE);
    }
}

// Allocate memory for an array of ints.
int * allocate_intmem (int len) {
    void *tmp;

    if (len <= 0) {
        fprintf (stderr, "ERROR: Cannot allocate memory because len = %i in allocate_intmem().\n", len);
        exit (EXIT_FAILURE);
    }

    tmp = (int *) malloc (len * sizeof (int));
    if (tmp != NULL) {
        memset (tmp, 0, len * sizeof (int));
        return (tmp);
    } else {
        fprintf (stderr, "ERROR: Cannot allocate memory for array allocate_intmem().\n");
        exit (EXIT_FAILURE);
    }
}


#endif //COMPUTER_NETWORK_PROJ_PORT_CHAT_H
