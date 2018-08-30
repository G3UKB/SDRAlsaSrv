/*
fcdif.c

Set FCD frequency.

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

// This is a relatively long operation so has to be on a separate thread
// otherwise it will interrupt the real time aspects.

// Includes
#include "../common/include.h"

// Forward refs
static int fcd_get_freq();

// Module vars
pthread_mutex_t fcd_mutex = PTHREAD_MUTEX_INITIALIZER;
int freq_hz;
int last_freq = -1;

// Safe set frequency
void fcd_set_freq(int f) {
    pthread_mutex_lock(&fcd_mutex);
    freq_hz = f;
    pthread_mutex_unlock(&fcd_mutex);
}

// Safe get frequency
static int fcd_get_freq() {
    int f;
    pthread_mutex_lock(&fcd_mutex);
    f = freq_hz;
    pthread_mutex_unlock(&fcd_mutex);
    return f;
}

// Thread entry point for FCD processing
void fcdif_imp(void* data){
    int stat, new_freq;

    // Get our thread parameters
    fcd_thread_data* td = (fcd_thread_data*)data;

    printf("Started FCD interface thread\n");

    // Loop until terminated
    while (td->terminate == FALSE) {
        new_freq = fcd_get_freq();
        //printf("%d,%d\n", new_freq,last_freq);
        if (last_freq != new_freq) {
            last_freq = new_freq;
            // Use the FCD controller software to set the frequency
            stat = fcdAppSetFreq(new_freq);
            //printf("Sent freq: %d\n", new_freq);
            if (stat == FCD_MODE_NONE)
                printf("No FCD Detected!\n");
            else if (stat == FCD_MODE_BL)
                printf("FCD in bootloader mode!\n");
        } else {
            sleep(0.1);
        }
    }

    printf("FCD Interface thread exiting...\n");
}
