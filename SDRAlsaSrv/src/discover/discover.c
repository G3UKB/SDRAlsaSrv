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

// Initialise UDP
int udpinit() {
    int rc;

    // Create socket
    sock=socket(AF_INET, SOCK_DGRAM, 0);
    if (sock<0) {
        printf("Cannot open UDP socket!\n");
        return -1;
    }

    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast,sizeof broadcast) == -1) {
        printf("Faile to setsockopt (SO_BROADCAST)");
        return -1;
    }

    // Bind local server port
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(LOCAL_SERVER_PORT);
    rc = bind (sock, (struct sockaddr *) &servAddr,sizeof(servAddr));
    if (rc<0) {
    printf("Cannot bind port number %d\n", LOCAL_SERVER_PORT);
        return -1;
    }

    printf("\nWaiting for discovery on UDP port %u\n", LOCAL_SERVER_PORT);

    return 0;
}
