#ifndef HAST_TWO_H
#define HAST_TWO_H

#include "common.h"

void Flow2img_HAST_Two(void *arg);
unsigned int extract_n_packets_from_pcap(uint8_t *pcap_data, size_t pcap_len, int n, const uint8_t **pkt_arr, int *pkt_lens);
uint8_t* get_n_byte_from_pkt(int n, const uint8_t *pkt, int pkt_len);
void bytes_to_onehot_image(const uint8_t *bytes, int n, unsigned char image[256][n], int current_get_bytes);

#endif // HAST_TWO_H