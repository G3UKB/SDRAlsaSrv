/*
main.h

Global header.

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

#ifndef _main_h
#define _main_h

#define TRUE 1
#define FALSE 0

// Constants
#define METIS_FRAME_SZ 1032

//==================================================================
// Ring buffer to hold audio samples
ringb_t *rb_iq;

//==================================================================
// Threads for audio capture and UDP dispatch
pthread_t alsa_thd;
pthread_t udp_writer_thd;
pthread_t udp_reader_thd;
pthread_t fcd_thd;

// Thread data structure for ALSA
typedef struct alsa_thread_data {
    int terminate;
    int pause;
	ringb_t *rb;
}alsa_thread_data;
alsa_thread_data *alsa_td;

// Thread data structure for UDP reader/writer
typedef struct udp_thread_data {
    int terminate;
    int pause;
    int socket;
    struct sockaddr_in *cli_addr;
	ringb_t *rb;
}udp_thread_data;
udp_thread_data *udp_writer_td;
udp_thread_data *udp_reader_td;

// Thread data structure for ALSA
typedef struct fcd_thread_data {
    int terminate;
}fcd_thread_data;
fcd_thread_data *fcd_td;

#endif
