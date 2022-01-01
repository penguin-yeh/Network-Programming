#include <pcap.h>
#include <net/ethernet.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PCAP_ERRBUF_SIZE 1024
#define IPv6 0x86dd
#define UDP 0x0011

/*  struct pcap_pkthdr { 
    struct timeval ts;   time stamp  
    bpf_u_int32 caplen;  length of portion present  
    bpf_u_int32;         lebgth this packet (off wire)  
    } 
*/

/*  struct	ether_header 
    {
        u_char	ether_dhost[6];
        u_char	ether_shost[6];
        u_short	ether_type;
    };
    #define	ETHERTYPE_PUP	0x0200		PUP protocol
    #define	ETHERTYPE_IP	0x0800		IP protocol
    #define ETHERTYPE_ARP	0x0806		Addr. resolution protocol 
*/

void pcap_cb(u_char *arg, const struct pcap_pkthdr *header, const u_char *packet)
{
    int *id = (int *)arg;
    printf("%dth packet\n", *id);
    *id += 1;
    //header information
    // printf("Packet Length: %d\n", header->len);
    // printf("Number of bytes: %d\n", header->caplen);
    printf("Recieved time: %s", ctime((const time_t *)&header->ts.tv_sec)); 

    //packet infromation
    struct ether_header *ethernet_hdr;
    struct ip *ip_hdr;
    struct tcphdr *tcp_hdr;
    char src_ip[1024];
    char dst_ip[1024];

    ethernet_hdr = (struct ether_header*)packet;
    printf("Src MAC Address: ");
    for(int i=0;i<6;i++)
    {
        printf("%02x", ethernet_hdr->ether_shost[i]);
        if(i!=5)
            printf(":");
    }    
    printf("\n");
    printf("Dst MAC Address: ");
    for(int i=0;i<6;i++)
    {
        printf("%02x ", ethernet_hdr->ether_dhost[i]);
        if(i!=5)
            printf(":");
    }
    printf("\n");
    printf("Ethernet Type: %04x\n",ntohs(ethernet_hdr->ether_type));//%02x -> 16進位
    if(ntohs(ethernet_hdr->ether_type) == ETHERTYPE_IP)//IPv4
    {
        printf("IP Type: IPv4\n");
        ip_hdr = (struct ip*)(packet + sizeof(struct ether_header));
        inet_ntop(AF_INET, &(ip_hdr->ip_src), src_ip, INET_ADDRSTRLEN);
        printf("Src IP Address: %s\n", src_ip);
        inet_ntop(AF_INET, &(ip_hdr->ip_dst), dst_ip, INET_ADDRSTRLEN);
        printf("Dst IP Address: %s\n", dst_ip);
        //printf("%d\n", ip_hdr->ip_p);
        if(ip_hdr->ip_p == IPPROTO_TCP || ip_hdr->ip_p == UDP)
        {
            if(ip_hdr->ip_p == IPPROTO_TCP)
                printf("Protocol: TCP\n");
            else if(ip_hdr->ip_p == UDP)
                printf("Protocol: UDP\n");
            tcp_hdr = (struct tcphdr*)(packet + sizeof(struct ether_header) + sizeof(struct ip));
            u_int16_t src_port = ntohs(tcp_hdr->th_sport);//just 16 bits
            u_int16_t dst_port = ntohs(tcp_hdr->th_dport);
            printf("Src port: %u\n", src_port);
            printf("Dst port: %u\n", dst_port);
        }
    }
    else if(ntohs(ethernet_hdr->ether_type) == IPv6)//IPv6
    {
        printf("IP Type: IPv6\n");
        ip_hdr = (struct ip*)(packet + sizeof(struct ether_header) - 4);
        inet_ntop(AF_INET6, &(ip_hdr->ip_src), src_ip, INET6_ADDRSTRLEN);
        printf("Src IP Address: %s\n", src_ip);
        ip_hdr = (struct ip*)(packet + sizeof(struct ether_header) + 8);
        inet_ntop(AF_INET6, &(ip_hdr->ip_dst), dst_ip, INET6_ADDRSTRLEN);
        printf("Dst IP Address: %s\n", dst_ip);

        ip_hdr = (struct ip*)(packet + sizeof(struct ether_header) - 3);
        //printf("%04x\n", ip_hdr->ip_p);
        if(ip_hdr->ip_p == IPPROTO_TCP || ip_hdr->ip_p == UDP)
        {
            if(ip_hdr->ip_p == IPPROTO_TCP)
                printf("Protocol: TCP\n");
            else if(ip_hdr->ip_p == UDP)
                printf("Protocol: UDP\n");
            tcp_hdr = (struct tcphdr*)(packet + sizeof(struct ether_header) + sizeof(struct ip) + 20);
            u_int16_t src_port = ntohs(tcp_hdr->th_sport);//just 16 bits
            u_int16_t dst_port = ntohs(tcp_hdr->th_dport);
            printf("Src port: %u\n", src_port);
            printf("Dst port: %u\n", dst_port);
        }
    }
    printf("\n");
}


int main(int argc, char**argv)
{
    char *dev, errbuf[PCAP_ERRBUF_SIZE];
    char filename[1024];
    int pkt_num = -1;
    int id = 1;

    dev = pcap_lookupdev(errbuf);
    if(dev == NULL)
    {
        fprintf(stderr, "Couldn't find default device:%s\n", errbuf);
        exit(1);
    }
    printf("Device:%s\n", dev);

    pcap_t *handle;
    handle = pcap_open_live(dev, 1024, 1, 0, errbuf);
    // default: read from device
    if(argc == 3)//read from pcap
    {
        strcpy(filename, argv[2]);
        handle = pcap_open_offline(filename, errbuf);
        if(!handle)
        {
            fprintf(stderr, "pcap_open_offline(): %s\n", errbuf);
            exit(1);
        }
        printf("Open: %s\n", filename);
    }
    else if(argc == 4 && strcmp(argv[1], "-n")==0)//read n pkt from device
    {
        strcpy(filename, argv[3]);
        handle = pcap_open_offline(filename, errbuf);
        if(!handle)
        {
            fprintf(stderr, "pcap_open_offline(): %s\n", errbuf);
            exit(1);
        }
        printf("Open: %s\n\n", filename);
        pkt_num = atoi(argv[2]);
    }
    else
    {
        printf("Usage: ./hw3 -r pcap_filename\n");
        printf("Usage: ./hw3 -n pcap_num pcap_filename\n");
    }
    pcap_loop(handle, pkt_num, pcap_cb, (u_char*)&id);
    return 0;
}
