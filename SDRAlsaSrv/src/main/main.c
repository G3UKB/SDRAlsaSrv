/*
main.c

Serve data from a sound card device on Linux (specifically RPi)
using Alsa and UDP to emulate an HPSDR Protocol 1 system.

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

// Defines
// Allow 10 reads of 1024 IQ samples
#define iq_ring_byte_sz 10*1024*4

// Program entry point
int main() {

    // Local variables
    int rc, iq_ring_sz;

    printf("SDR ALSA Server starting...\n");

    //===========================================================================
    // Do discovery protocol 1 as per HPSDR

    //===========================================================================
    // Allocate a ring buffer to hold audio samples
    iq_ring_sz = pow(2, ceil(log(iq_ring_byte_sz)/log(2)));
    rb_iq = ringb_create (iq_ring_sz);

    //===========================================================================
    // ALSA init
    // Allocate thread data structure
	alsa_td = (alsa_thread_data*)safealloc(sizeof(alsa_thread_data), sizeof(char), "ALSA_TD_STRUCT");
	// Init with thread data items
	alsa_td->terminate = FALSE;
    alsa_td->rb = rb_iq;

	// Create the ALSA thread
	rc = pthread_create(&alsa_thd, NULL, alsa_imp, (void *)alsa_td);
	if (rc){
        printf("Failed to create ALSA thread [%d]\n", rc);
        exit(1);;
	}

	//===========================================================================
    // UDP writer init
    // Allocate thread data structure
	udp_writer_td = (udp_thread_data*)safealloc(sizeof(udp_thread_data), sizeof(char), "UDP_TD_STRUCT");
	// Init with thread data items
	udp_writer_td->terminate = FALSE;
    udp_writer_td->rb = rb_iq;
    udp_writer_td->sock = 0;

	// Create the UDP writer thread
	rc = pthread_create(&udp_writer_thd, NULL, udp_writer_imp, (void *)udp_writer_td);
	if (rc){
        printf("Failed to create UDP writer thread [%d]\n", rc);
        exit(1);;
	}

	//===========================================================================
    // UDP reader init
    // Allocate thread data structure
	udp_reader_td = (udp_thread_data*)safealloc(sizeof(udp_thread_data), sizeof(char), "UDP_TD_STRUCT");
	// Init with thread data items
	udp_reader_td->terminate = FALSE;
    udp_reader_td->rb = rb_iq;
    udp_reader_td->sock = 0;

	// Create the UDP writer thread
	rc = pthread_create(&udp_reader_thd, NULL, udp_reader_imp, (void *)udp_reader_td);
	if (rc){
        printf("Failed to create UDP reader thread [%d]\n", rc);
        exit(1);;
	}

	sleep(15);
	alsa_td->terminate = TRUE;
	udp_writer_td->terminate = TRUE;
	udp_reader_td->terminate = TRUE;
    sleep(1);

	exit(0);
}
