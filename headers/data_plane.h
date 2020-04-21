#ifndef __data_plane__
#define __data_plane__

#include "enb.h"
#include "ue.h"
#include <stdio.h>
#include <stdlib.h>

#define SIMULATED_TRAFFIC	0x01
#define EXTERNAL_TRAFFIC	0x02

int open_enb_data_plane_socket(eNB * enb);
int setup_ue_data_plane(UE * ue);
void generate_udp_traffic(UE * ue, eNB * enb, uint8_t * dest_ip, int port_dest);
void start_uplink_thread(UE * ue, eNB * enb);

#endif