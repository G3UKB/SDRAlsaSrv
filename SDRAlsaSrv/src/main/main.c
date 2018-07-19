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

// Forward decs
int open_bc_socket();
int revert_sd(int sd);
void INThandler(int sig);

// Program entry point
int main() {

    // Local variables
    int rc, sd, iq_ring_sz;
    struct sockaddr_in *cli_addr;

    printf("SDR ALSA Server starting...\n");

    // Set an exit handler
    signal(SIGINT, INThandler);

    //===========================================================================
    // Do discovery protocol 1 as per HPSDR
    if ((sd = open_bc_socket()) == -1) {
        printf("Failed to open broadcast socket!\n");
        exit(1);
    }
    cli_addr = do_discover(sd);
    if (cli_addr == (struct sockaddr_in *)NULL) {
        printf("Sorry, discovery protocol failed!\n");
        exit(1);
    }

    if (!revert_sd(sd)) {
        printf("Sorry, failed to revert socket!\n");
        exit(1);
    }

    //===========================================================================
    // Allocate a ring buffer to hold audio samples
    iq_ring_sz = pow(2, ceil(log(iq_ring_byte_sz)/log(2)));
    rb_iq = ringb_create (iq_ring_sz);

    //===========================================================================
    // Format init
    fmtinit();

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
    udp_writer_td->socket = sd;
    udp_writer_td->cli_addr = cli_addr;

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
    udp_writer_td->socket = sd;
    udp_writer_td->cli_addr = cli_addr;

	// Create the UDP writer thread
	rc = pthread_create(&udp_reader_thd, NULL, udp_reader_imp, (void *)udp_reader_td);
	if (rc){
        printf("Failed to create UDP reader thread [%d]\n", rc);
        exit(1);;
	}

    // Wait for the exit signal
	while(1) {
        pause();
	}
}

// Open a broadcast socket
int open_bc_socket() {
    int sd, rc;
    int  broadcast = 1;
    struct sockaddr_in serv_addr;

    // Create socket
    sd=socket(AF_INET, SOCK_DGRAM, 0);
    if (sd<0) {
        printf("Cannot open UDP socket!\n");
        return -1;
    }

    // Set to broadcast
    if (setsockopt(sd, SOL_SOCKET, SO_BROADCAST, &broadcast,sizeof broadcast) == -1) {
        printf("Failed to set SO_BROADCAST!\n");
        return -1;
    }

    // Bind local server port
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(LOCAL_SERVER_PORT);
    rc = bind (sd, (struct sockaddr *) &serv_addr,sizeof(serv_addr));
    if (rc<0) {
    printf("Cannot bind port number %d\n", LOCAL_SERVER_PORT);
        return -1;
    }

    return sd;
}

// Revert broadcast socket to a normal socket
int revert_sd(int sd) {
    int broadcast = 0;
    int sendbuff = 32000;
    int recvbuff = 32000;

    // Turn off broadcast
    if (setsockopt(sd, SOL_SOCKET, SO_BROADCAST, &broadcast,sizeof broadcast) == -1) {
        printf("Failed to set option SO_BROADCAST!\n");
        return FALSE;
    }
    // Set send buffer size
    if (setsockopt(sd, SOL_SOCKET, SO_SNDBUF, &sendbuff, sizeof(sendbuff)) == -1) {
         printf("Failed to set option SO_SNDBUF!\n");
        return FALSE;
    }
    // Set receive buffer size
    if (setsockopt(sd, SOL_SOCKET, SO_RCVBUF, &sendbuff, sizeof(recvbuff)) == -1) {
         printf("Failed to set option SO_RCVBUF!\n");
        return FALSE;
    }
    return TRUE;
}

// SIGINT Handler
void  INThandler(int sig)
{
     char  c;

     signal(sig, SIG_IGN);
     printf("Do you really want to quit? [y/n] ");
     c = getchar();
     if (c == 'y' || c == 'Y') {
        printf("SDR ALSA Server exiting...\n");
        alsa_td->terminate = TRUE;
        udp_writer_td->terminate = TRUE;
        udp_reader_td->terminate = TRUE;
        sleep(1);
        exit(0);
     }
     else
          signal(SIGINT, INThandler);
     getchar(); // Get new line character
}
