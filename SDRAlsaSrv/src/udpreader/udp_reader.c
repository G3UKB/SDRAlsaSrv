/*
udp_reader.c

Read data from UDP.

Copyright (C) 2018 by G3UKB Bob Cowdery

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

The authors can be reached by email at:

	bob@bobcowdery.plus.com
*/

// Includes
#include "../common/include.h"

#define STATE_DATA 0
#define STATE_WAIT 1

// Forward refs
static void udprecvdata(int sd, struct sockaddr_in *cliAddr);
int udp_reader_state = STATE_DATA;
// Module vars
unsigned char pcdata[METIS_FRAME_SZ];

// Thread entry point for ALSA processing
void udp_reader_imp(void* data){
    // Get our thread parameters
    udp_thread_data* td = (udp_thread_data*)data;
    int sd = td->socket;
    struct sockaddr_in *cli_addr = td->cli_addr;

    printf("Started UDP reader thread\n");

    while (td->terminate == FALSE) {
         udprecvdata(sd, cli_addr);
         //sleep(0.1);
    }

    printf("UDP Reader thread exiting...\n");
}


// Read and discard data but extract the tuning frequency
// Set the FCD to the frequency
// ToDo - Enhance for other sound card type devices
static void udprecvdata(int sd, struct sockaddr_in *cliAddr) {
    unsigned char b0,b1,b2,b3;
    unsigned int freq = 0;
    int n;
    unsigned int addr_sz = sizeof(*cliAddr);

    // Read a frame size data packet
    memset(pcdata, 0, METIS_FRAME_SZ);
    n = recvfrom(sd, pcdata, METIS_FRAME_SZ, 0, (struct sockaddr_in *)cliAddr, &addr_sz);
    //printf("Got %d", n);
    if (udp_reader_state == STATE_DATA) {
        if(n == METIS_FRAME_SZ) {
            // Extract the control bytes
            //if (pcdata[11] == 0x02)
                printf("%d\n", pcdata[11]);
            if ((pcdata[11] & 0xFE) == 0x02) {
                // Extract freq LSB in bo
                b3 = pcdata[12];
                b2 = pcdata[13];
                b1 = pcdata[14];
                b0 = pcdata[15];
                // Format into an unsigned int as a frequency in Hz
                freq = (int)(b3 << 24);
                freq = (int)(freq | (b2 << 16));
                freq = (int)(freq | (b1 << 8));
                freq = (int)(freq | b0);
                printf("Got freq: %d\n", freq);
                fcd_set_freq(freq);
            }
        } else if (n>0) {
            // Check for a stop message
            if (pcdata[2] == 0x04) {
                printf("Stop, pausing stream...\n");
                alsa_pause();
                udp_writer_pause();
                udp_reader_state = STATE_WAIT;
            }
        }
    } else {
        // Wait for a start message
        if (pcdata[2] == 0x04) {
            printf("Start, running stream...\n");
            alsa_run();
            udp_writer_run();
            udp_reader_state = STATE_DATA;
        }
    }
}
