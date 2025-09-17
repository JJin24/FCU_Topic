#include <stdio.h>
#include <stdlib.h>
#include <pcap.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/ether.h>

struct vlan_hdr {
	__be16 h_vlan_TCI;
	__be16 h_vlan_encapsulated_proto;
};

int de_ipv4(const uint8_t *bytes, int n, unsigned char image[256][n], int i);
int de_ipv6(const uint8_t *bytes, int n, unsigned char image[256][n], int i);
int de_addr(const uint8_t *bytes, int n, unsigned char image[256][n], int i);
void __DENOISE_bytes_to_onehot_image(const uint8_t *bytes, int n, 
    unsigned char image[256][n], int current_get_bytes);


void de_ipv4_gray(uint8_t* image, int i);
void de_ipv6_gray(uint8_t *image, int i);
void de_addr_gray(uint8_t *image, int n, int row);
void __DENOISE_bytes_Grayscale_image(const uint8_t *bytes, int n, int row,
	unsigned char image[row][16], int current_get_bytes);