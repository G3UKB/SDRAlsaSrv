/*
udp_writer.c

Write data to UDP.

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

// Thread entry point for ALSA processing
void udp_writer_imp(void* data){
    // Get our thread parameters
    udp_thread_data* td = (udp_thread_data*)data;
    ringb_t *rb = td->rb;
    printf("Started UDP writer thread\n");

    while (td->terminate == FALSE) {
         sleep(1);
    }

    printf("UDP Writer thread exiting...\n");
}
