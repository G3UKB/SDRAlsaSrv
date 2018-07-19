/*
discover.c

Do HPSDR discovery protocol 1

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

// Module vars
// Client and server addresses
struct sockaddr_in cliAddr, servAddr;
unsigned char msg[MAX_MSG];
unsigned char resp[MAX_RESP];
unsigned char pcdata[METIS_FRAME_SZ];

// Forward refs
static int udprecvcontrol(int sd);
static int udpsendresp(int sd, int type);

// Discover protocol
struct sockaddr_in *do_discover(int sd) {
    int rc, udpmsg;

    // Wait for discovery message
    printf("Waiting for discovery message...\n");
    while((udpmsg = udprecvcontrol(sd)) != DISCOVERY_MSG) {
        sleep(0.1);
    }
    printf("Discover... sending response\n");
    udpsendresp(sd, DISCOVERY_RESP);

    // Wait for start message
    printf("Waiting for start message...\n");
    while((udpmsg = udprecvcontrol(sd)) != START_MSG) {
        sleep(0.1);
    }
    printf("Starting...\n");

    return &cliAddr;
}

// Receive one packet from the client
static int udprecvcontrol(sd) {
    int n;
    unsigned int cliLen;

    // Clear message buffer
    memset(msg,0x0,MAX_MSG);
    // receive message
    cliLen = sizeof(cliAddr);
    n = recvfrom(sd, msg, MAX_MSG, 0, (struct sockaddr *) &cliAddr, &cliLen);

    if(n<0) {
        return READ_FAILURE;
    }

    // Announce
    // printf("Received %d bytes from %s:UDP%u\n", n, inet_ntoa(cliAddr.sin_addr), ntohs(cliAddr.sin_port));

    // Decode message
    if (msg[2] == 0x02) {
        // Discovery message
        return DISCOVERY_MSG;
    } else if (msg[2] == 0x04) {
        if ((msg[3] & 0x01) == 0x01)
            return START_MSG;
        else if ((msg[3] & 0x01) == 0x00)
            return STOP_MSG;
        else {
            printf("Received message 0x02 subtype %d which is not implemented!\n", msg[3]);
            return UNKNOWN_MSG;
        }
    }
    printf("Received message %d which is not implemented!\n", msg[2]);
    return UNKNOWN_MSG;
}

// Write discover response packet to the UDP client
static int udpsendresp(int sd, int type) {
    if (type == DISCOVERY_RESP) {
        memset(msg,0x0,MAX_RESP);
        resp[0] = 0xEF;
        resp[1] = 0xFE;
        resp[2] = 0x02;
        //Send discovery response packet
        if (sendto(sd, resp, MAX_RESP, 0, (struct sockaddr*) &cliAddr, sizeof(cliAddr)) == -1)
        {
            printf("Failed to send discover response!");
            return FALSE;
        }
    }
    return TRUE;
}

