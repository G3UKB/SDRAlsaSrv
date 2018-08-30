/*
formatter.c

Format an HPSDR frame.

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

// Forward references
static void addsync(int offset);
static void copydataset(short* usbframes, int f_b_offset, int m_b_offset, int f_b_end);
static void fmt_add_seq();

/* Module variables */
unsigned char* metis_frame = NULL;
unsigned int fmt_seq_no = 0;

// Initialise the formatter
void fmtinit() {
    /* Allocate one metis size frame */
    metis_frame = (unsigned char *) malloc(METIS_FRAME_SZ);
    memset(metis_frame, 0, METIS_FRAME_SZ);
    /* Init the fixed byte sequence */
    metis_frame[0] = 0xEF;
    metis_frame[1] = 0xFE;
    metis_frame[2] = 0x01;
    metis_frame[3] = 0x06;
}

// Clean up
void fmtclean() {
    if (metis_frame != NULL ) free(metis_frame);
}

// Format an HPSDR Metis frame
unsigned char *fmtframe(short *usbframes) {
    /*
    ** I&Q plus Microphone data from Metis to a PC is via UDP/IP, sent to the IP address of
    ** the PC and ‘from port’ of the originating PC (and from port 1024), with the following
    ** payload :-
    **  <0xEFFE><0x01><End Point><Sequence Number><2 x HPSDR frames>
    ** where :-
    **  End point = 1 byte (0x06 – representing USB EP6)
    **  Sequence Number = 4 bytes (32 bit unsigned integer starting at zero and incremented each
    **  frame and independently for each end point)
    **  HPSDR data = 1024 bytes (2 x 512 byte USB format frames)
    */
    //int i,j,k;

    // The data is raw interleaved 16 bit LE x 126 samples (i.e. 2 frames)
    // Add the 4 byte BE 32 bit sequence number
    fmt_add_seq();
    // Add first USB sync
    addsync(8);
    // Allow 5 CC bytes
    // Copy fist USB dataset
    copydataset(usbframes, 0, 16, 126);
    /* Add second USB sync */
    addsync(520);
    // Allow 5 CC bytes
    // Copy second USB dataset
    copydataset(usbframes, 126, 528, 252);
    return metis_frame;
}

// Add USB frame sync characters
static void addsync(int offset) {
    metis_frame[offset] = 0x7f;
    metis_frame[offset+1] = 0x7f;
    metis_frame[offset+2] = 0x7f;
}

// Copy and format the dataset from the frame buffer to the metis buffer
static void copydataset(short* usbframes, int f_b_offset, int m_b_offset, int f_b_end) {
    // frame buffer is 16 bit LE
    // metis buffer requires 24 bit BE
    // We iterate over the frame buffer in increments of 2 16 bit words (I & Q)
    // The metis buffer is cleared and has 2x24 bit + 1x16 bit values per sample (I, Q 24 bit, Mic 16 bit (ignored)
    int i,j;
    short i_smpl;
    short q_smpl;

    for (i=f_b_offset, j=m_b_offset ; i<f_b_end ; i+=2, j+=8) {
        i_smpl = usbframes[i+1];
        if (i_smpl < 0) {
            metis_frame[j+0] = 0xFF;
        } else {
            metis_frame[j+0] = 0x00;
        }
        metis_frame[j+1] = i_smpl >> 8 & 0xFF;
        metis_frame[j+2] = i_smpl & 0xFF;

        q_smpl = usbframes[i];
        if (q_smpl < 0) {
            metis_frame[j+3] = 0xFF;
        } else {
            metis_frame[j+3] = 0x00;
        }
        metis_frame[j+4] = q_smpl >> 8 & 0xFF;
        metis_frame[j+5] = q_smpl & 0xFF;
    }
}

// Add the incrementing sequence number
static void fmt_add_seq() {
    metis_frame[7] = fmt_seq_no & 0xFF;
    metis_frame[6] = (fmt_seq_no >> 8) & 0xFF;
    metis_frame[5] = (fmt_seq_no >> 16) & 0xFF;
    metis_frame[4] = (fmt_seq_no >> 24) & 0xFF;
    fmt_seq_no++;
}
