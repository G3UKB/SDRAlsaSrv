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

// Program variables
// Ring buffer to hold audio samples
ringb_t *rb_iq;
// Threads for audio capture and UDP dispatch
pthread_t alsa_thd;
pthread_t udp_thd;

// Passed to the threads
typedef struct ThreadData {
	ringb_t *rb;
}ThreadData;

ThreadData *td;

int main() {

    // Local variables
    int rc;

    printf("SDR ALSA Server starting...\n");

    // Allocate a ring buffer to hold audio samples
    rb_iq = ringb_create (iq_ring_byte_sz);

    // Allocate thread data structure
	td = (ThreadData *)safealloc(sizeof(ThreadData), sizeof(char), "TD_STRUCT");
	// Init with thread data items
    td->rb = rb_iq;

	// Create the ALSA thread
	rc = pthread_create(&alsa_thd, NULL, alsa_imp, (void *)td);
	if (rc){
        printf("Failed to create ALSA thread [%d]\n", rc);
        exit(1);;
	}

}
