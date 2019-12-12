/* 
 * upload.c
 * 
 * Implementations of file upload feature for torrent client.
 */

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "chunk.h"
#include "packet.h"
#include "bt_parse.h"
#include "upload.h"
#include "download.h"
#include "debug.h"
#include "input_buffer.h"

// Test functions

// Making packets after reading chunk
void make_packets(upload_t *upl, char* buf, int buf_size) {

    int rem = buf_size % DATALEN;
    DPRINTF(DEBUG_UPLOAD, "make_packets: Total data %d bytes\nMax packet size \
%d bytes\nLast packet %d bytes\n", buf_size, DATALEN, rem);
    int listsize = buf_size / DATALEN + !!rem;
    DPRINTF(DEBUG_UPLOAD, "make_packets: Splitting chunk across %d packets\n", listsize);
    data_packet_t *packs = malloc(sizeof(*packs) * listsize); // Array of packets to be created
    int offs = 0;

        // Divide into packets
        for(int i = 0; i < listsize; i++) {
            char* mem_curr = buf + offs; 
            pct_data(&packs[i], i + 1, mem_curr,
                (i == (listsize - 1)) ? rem : DATALEN);         
            offs += DATALEN;
        }
        upl->chunk.packetlist = packs;
        upl->chunk.l_size = listsize;
        DPRINTF(DEBUG_UPLOAD, "make_packets: Finished creating %d packets\n", listsize);
        // Initiaize and allocate array for future acks to be received
        upl->recv = malloc(sizeof(*upl->recv) * listsize);
        memset(upl->recv, 0, sizeof(*upl->recv) * listsize);
}

// Check if ack receieved is a duplicate that was rec before for another packet
char check_dup_ack(upload_t *upl, int sequence) {
    return upl->recv[sequence] > 1;
}

char should_resend_data(upload_t *upl, int sequence) {
    return upl->recv[sequence] >= MAX_DUP_ACK;
}
 
// Check when the last data packet was received
void handle_duplicate_ack(upload_t *upl, int seq, int sock, struct sockaddr_in *dest) {

    //double tdiff_last_ack = difftime(time(0), upl->chunk.last_ack_recv); 
    //(tdiff_last_ack > T_OUT_ACK) || 

    // Time-out occurs, or dup ack rec 3 times, incrementing recv array int from 2 -> 3 -> 4
    if(check_dup_ack(upl, seq)) {
        if (should_resend_data(upl, seq)) { 
            data_packet_t *resend_pac = &upl->chunk.packetlist[seq];
            pct_send(resend_pac, dest, sock); // in recv array 
            DPRINTF(DEBUG_UPLOAD, "handle_duplicate_ack: Re-sending lost packet %d\n", seq + 1);
        }
    }
    // Ack never seen before
    else if (upl->last_available < upl->chunk.l_size) {
        data_packet_t *send_next = &upl->chunk.packetlist[upl->last_available++]; // Get the next packet to send
        pct_send(send_next, dest, sock); // Send the next data packet if ack rec is not seen before
        DPRINTF(DEBUG_UPLOAD, "handle_duplicate_ack: Sending next packet %d\n", upl->last_available);
    }

}

void free_chunku(chunku_t *chk) {
    free(chk->packetlist);
}

// Clear memory after upload is downloaded by the peer
void upload_clear(upload_t *upl, struct sockaddr_in from) {
    if (upl->last_available == upl->chunk.l_size) { // Check if chunk upload has completed
        // Acks count arr from upl struct and structs in 'chunku'
        free_chunku(&upl->chunk);
        free(upl->recv);

    }
    memset(upl, 0, sizeof(*upl));
}

/* 
    if (ack number is not yet seen)
        send data[ack number + window size]

    // inside of function called by process get to send first series of data packets
    for (data packet from 0 to window size)
        send data packet


        DPRINTF(DEBUG_UPLOADS, "Msg %d\n", int);
*/
