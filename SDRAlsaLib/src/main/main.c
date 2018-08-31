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
static void *run_imp(void* data);
static int open_bc_socket();
static int revert_sd(int sd);

// Module vars
struct sockaddr_in *cli_addr;
int sd;

//===============================================================================
// Lib initialisation
int lib_init() {

    // Local variables
    int rc, iq_ring_sz;

    printf("SDR ALSA Lib initialising...\n");

    // Initialise thread data structures
    run_td = (run_thread_data *)NULL;
    alsa_td = (alsa_thread_data *)NULL;
    udp_writer_td = (udp_thread_data *)NULL;
    udp_reader_td = (udp_thread_data *)NULL;
    fcd_td = (fcd_thread_data *)NULL;

    //===========================================================================
    // Open a broadcast socket
    if ((sd = open_bc_socket()) == -1) {
        printf("Failed to open broadcast socket!\n");
        return FALSE;
    }

    //===========================================================================
    // Allocate a ring buffer to hold audio samples
    iq_ring_sz = pow(2, ceil(log(iq_ring_byte_sz)/log(2)));
    rb_iq = ringb_create (iq_ring_sz);

    //===========================================================================
    // Init FCD
    if( fcdOpen() == (hid_device*)NULL ) {
        printf("No FCD Detected!\n");
        return FALSE;
    }

    //===========================================================================
    // Init formatting
    fmtinit();

    //===========================================================================
    // Create/start the run thread
    // Allocate thread data structure
	run_td = (alsa_thread_data*)safealloc(sizeof(run_thread_data), sizeof(char), "RUN_TD_STRUCT");
    run_td->run = FALSE;
    run_td->terminate = FALSE;
    run_td->state = STATE_RUNNING;
	rc = pthread_create(&run_thd, NULL, run_imp, (void *)run_td);
	if (rc){
        printf("Failed to create run thread [%d]\n", rc);
        return FALSE;
	}

    return TRUE;
}

//===============================================================================
// Run lib
int lib_run()
{
    run_td->run = TRUE;
}

//===============================================================================
// Get state
int lib_state()
{
    return run_td->state;
}

//===============================================================================
// Run lib
static void *run_imp(void* data)
{
    int rc;

    // Get our thread parameters
    run_thread_data* td = (run_thread_data*)data;

    // Wait for run
    if (!td->run) sleep(0.2);

    //===========================================================================
    // Do discovery protocol
    cli_addr = do_discover(sd);
    if (cli_addr == (struct sockaddr_in *)NULL) {
        printf("Sorry, discovery protocol failed!\n");
        td->state = STATE_ERROR;
    }

    if (!revert_sd(sd)) {
        printf("Sorry, failed to revert discovery socket!\n");
        td->state = STATE_ERROR;
    }

    //===========================================================================
    // ALSA init
    // Allocate thread data structure
	alsa_td = (alsa_thread_data*)safealloc(sizeof(alsa_thread_data), sizeof(char), "ALSA_TD_STRUCT");
	// Init with thread data items
	alsa_td->terminate = FALSE;
	alsa_td->pause = FALSE;
    alsa_td->rb = rb_iq;

	// Create the ALSA thread
	rc = pthread_create(&alsa_thd, NULL, alsa_imp, (void *)alsa_td);
	if (rc){
        printf("Failed to create ALSA thread [%d]\n", rc);
        td->state = STATE_ERROR;
	}

	//===========================================================================
    // UDP writer init
    // Allocate thread data structure
	udp_writer_td = (udp_thread_data*)safealloc(sizeof(udp_thread_data), sizeof(char), "UDP_TD_STRUCT");
	// Init with thread data items
	udp_writer_td->terminate = FALSE;
	udp_writer_td->pause = FALSE;
    udp_writer_td->rb = rb_iq;
    udp_writer_td->socket = sd;
    udp_writer_td->cli_addr = cli_addr;

	// Create the UDP writer thread
	rc = pthread_create(&udp_writer_thd, NULL, udp_writer_imp, (void *)udp_writer_td);
	if (rc){
        printf("Failed to create UDP writer thread [%d]\n", rc);
        td->state = STATE_ERROR;
	}

	//===========================================================================
    // UDP reader init
    // Allocate thread data structure
	udp_reader_td = (udp_thread_data*)safealloc(sizeof(udp_thread_data), sizeof(char), "UDP_TD_STRUCT");
	// Init with thread data items
	udp_reader_td->terminate = FALSE;
    udp_reader_td->rb = rb_iq;
    udp_reader_td->socket = sd;
    udp_reader_td->cli_addr = cli_addr;

	// Create the UDP writer thread
	rc = pthread_create(&udp_reader_thd, NULL, udp_reader_imp, (void *)udp_reader_td);
	if (rc){
        printf("Failed to create UDP reader thread [%d]\n", rc);
        td->state = STATE_ERROR;
	}

	//===========================================================================
    // FCD init
    // Allocate thread data structure
	fcd_td = (fcd_thread_data*)safealloc(sizeof(fcd_thread_data), sizeof(char), "FCD_TD_STRUCT");
	// Init with thread data items
	fcd_td->terminate = FALSE;

	// Create the FCD interface thread
	rc = pthread_create(&fcd_thd, NULL, fcdif_imp, (void *)fcd_td);
	if (rc){
        printf("Failed to create FCD thread [%d]\n", rc);
        td->state = STATE_ERROR;
	}
}

//===============================================================================
// Tidy close
void lib_close()
{
    printf("SDR ALSA Lib closing...\n");
    if (alsa_td != NULL) {
        alsa_td->terminate = TRUE;
        pthread_join(alsa_thd, NULL);
    }
    if (udp_writer_td != NULL) {
        udp_writer_td->terminate = TRUE;
        pthread_join(udp_writer_thd, NULL);
    }
    if (udp_reader_td != NULL) {
        udp_reader_td->terminate = TRUE;
        pthread_join(udp_reader_thd, NULL);
    }
    if (fcd_td != NULL) {
        fcd_td->terminate = TRUE;
        pthread_join(fcd_thd, NULL);
    }
    if (run_td != NULL) {
        run_td->terminate = TRUE;
        pthread_join(run_thd, NULL);
    }
    printf("All threads terminated");
}

//===============================================================================
// Reset to wait for broadcast message
int lib_reset()
{
    lib_close();
    return lib_init();
}

//===============================================================================
// Local methods
// Open a broadcast socket
static int open_bc_socket() {
    int sd, rc;
    int  broadcast = 1;
    struct sockaddr_in serv_addr;
    struct timeval recv_timeout;
    recv_timeout.tv_sec = 5;
    recv_timeout.tv_usec = 0;


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

    // Set a timeout
    //if (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout,sizeof recv_timeout) == -1) {
    //    printf("Failed to set SO_RCVTIMEO!\n");
    //    return -1;
    //}

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
static int revert_sd(int sd) {
    int broadcast = 0;
    int sendbuff = 32000;
    int recvbuff = 32000;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100;

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
    if (setsockopt(sd, SOL_SOCKET, SO_RCVBUF, &recvbuff, sizeof(recvbuff)) == -1) {
         printf("Failed to set option SO_RCVBUF!\n");
        return FALSE;
    }
    // Set receive timeout
    if (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv) == -1) {
         printf("Failed to set option SO_RCVTIMEO!\n");
        return FALSE;
    }
    return TRUE;
}


