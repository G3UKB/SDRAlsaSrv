/*
alsa.c

Read data from an ALSA device.

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

// Forward decs
static int alsa_init();
static void alsa_clean();
static void alsa_read_frame();

// Module globals
ringb_t *rb;                    // The audio ring buffer
snd_pcm_uframes_t frames;       // Number of frames to read
snd_pcm_t *handle;              // ALSA device handle
snd_pcm_hw_params_t *hw_params; // ALSA hardware params
snd_pcm_sw_params_t *sw_params; // ALSA software params
short *buffer = NULL;           // The read buffer

// Thread entry point for ALSA processing
void alsa_imp(void* data){
    // Get our thread parameters
    alsa_thread_data* td = (alsa_thread_data*)data;
    rb = td->rb;
    printf("Started ALSA thread...\n");

    if (alsa_init() != 0) {
        printf("Failed to initialise ALSA!\n");
        printf("ALSA thread exiting...\n");
    }

    while (td->terminate == FALSE) {
        alsa_read_frame();
    }

    alsa_clean();
    printf("ALSA thread exiting...\n");
}

// Clean up
static void alsa_clean() {
    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    if (buffer != NULL ) free(buffer);
}

// Initialise the ALSA system
static int alsa_init() {

    // Local variables
    int err, rc;
    unsigned int val;
    int dir;
    int size;
    unsigned int rate_min;
    unsigned int rate_max;
    unsigned int ch_min;
    unsigned int ch_max;
    unsigned int theformat;
    snd_pcm_uframes_t thesize;

    // Open audio device for capture
    // ToDo - pass in device name
    rc = snd_pcm_open(&handle, "hw:1,0", SND_PCM_STREAM_CAPTURE, 0);
    if (rc < 0) {
        fprintf(stderr, "Unable to open FCD device: %s\n", snd_strerror(rc));
        return rc;
    }

    /* Allocate a hardware and software parameters object. */
    snd_pcm_hw_params_alloca(&hw_params);
    snd_pcm_sw_params_alloca(&sw_params);

    // Fill hw params with default values.
    snd_pcm_hw_params_any(handle, hw_params);

    // Set the desired hardware parameters.
    // Interleaved mode
    if ((err = snd_pcm_hw_params_set_access(handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
    {
        printf("could not set audio access format:%s\n",snd_strerror(err));
        return -1;
    }

    // Signed 16-bit little-endian format as only supported by the FCD
    if ((err = snd_pcm_hw_params_set_format(handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0)
    {
        printf("could not set audio sample format:%s\n",snd_strerror(err));
        return -1;
    }

    /* Two channels (stereo) I&Q */
    if ((err = snd_pcm_hw_params_set_channels(handle, hw_params, 2)) < 0)
    {
        printf("could not set audio channels:%s\n",snd_strerror(err));
        return -1;
    }

    // 48000 bits/second sampling rate (FCD 48K firmware)
    val = 48000;
    if ((err = snd_pcm_hw_params_set_rate_near(handle, hw_params, &val, &dir)) < 0)
    {
        printf("could not set audio sample rate:%s\n",snd_strerror(err));
        return -1;
    }

    // Set period size to 1024 frames
    // ToDo - pass this in
    frames = 1024;
    if ((err = snd_pcm_hw_params_set_period_size_near(handle, hw_params, &frames, &dir)) < 0)
    {
        printf("could not set audio period size to 1024 [%s]\n",snd_strerror(err));
        return -1;
    }

    // Use a buffer large enough to hold one period of samples
    size = frames * 2; // 16 bit samples 2 channels
    buffer = (short*) malloc(size * sizeof(short));

	// Write the parameters to the driver
    rc = snd_pcm_hw_params(handle, hw_params);
    if (rc < 0) {
        fprintf(stderr, "Unable to set hw parameters: %s\n", snd_strerror(rc));
        return rc;
    }

    // Set sw params
    if ((snd_pcm_sw_params_current (handle, sw_params)) < 0) {
		printf ("Cannot set software capture parameters\n");
		return -1;
	}

    // Print out some useful values
	if (snd_pcm_hw_params_get_rate_min(hw_params, &rate_min, &dir) != 0)
		printf("Error\n");	// Error
	if (snd_pcm_hw_params_get_rate_max(hw_params, &rate_max, &dir) != 0)
		printf("Error\n");	// Error
	if (snd_pcm_hw_params_get_channels_min(hw_params, &ch_min) != 0)
		printf("Error\n");	// Error
	if (snd_pcm_hw_params_get_channels_max(hw_params, &ch_max) != 0)
		printf("Error\n");	// Error
    if (snd_pcm_hw_params_get_format (hw_params, &theformat) != 0)
		printf("Error\n");	// Error
    if (snd_pcm_hw_params_get_buffer_size (hw_params, &thesize) != 0)
		printf("Error\n");	// Error
	printf("Sample rate min %d  max %d\n",  rate_min, rate_max);
	printf("Number of channels min %d  max %d\n",  ch_min, ch_max);
	printf("Format %d\n",  theformat);
	printf("Buffer size %d\n",  thesize);

    return 0;
}

// Read a single frame and write to the ring buffer
static void alsa_read_frame()
{
    snd_pcm_sframes_t delay;
    int rc, avail;

    // Check state
    switch(snd_pcm_state(handle)) {
        case SND_PCM_STATE_RUNNING:
            // printf("read_alsa: State SND_PCM_STATE_RUNNING\n");
            break;
        case SND_PCM_STATE_PREPARED:
            // printf("read_alsa: State SND_PCM_STATE_PREPARED\n");
            snd_pcm_start(handle);
            break;
        case SND_PCM_STATE_XRUN:
            printf("read_alsa: State SND_PCM_STATE_XRUN\n");
            snd_pcm_prepare(handle);
            break;
        default:
            printf("read_alsa: State UNKNOWN\n");
            break;
	}

	// Attempt to read a frame
    snd_pcm_delay(handle, &delay);
    avail = snd_pcm_avail(handle);
    if (avail > frames) {
        // Enough samples in the fame buffer
        rc = snd_pcm_readi(handle, (void*)buffer, frames);
        if (rc == -EPIPE) {
            // EPIPE means overrun
            fprintf(stderr, "Overrun occurred\n");
            snd_pcm_prepare(handle);
        } else if (rc < 0) {
            printf("Error: %s\n", snd_strerror(rc));
        } else if (rc != (int)frames) {
            fprintf(stderr, "Short read, read %d frames\n", rc);
        } else {
            // Valid frame of correct size
            // Add to the ring buffer
            // Remember one frame is I 16bits, Q 16 bits so 4 bytes
            if (ringb_write_space (rb) > frames*4) {
				ringb_write (rb, (char *)buffer, frames*4);
			} else {
                printf("Ring buffer is full, skipping samples...\n");
			}
        }
    }
}
