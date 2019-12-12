Group 3
Kyle Patterson
Tahsin Oshin

We added onto the process UDP function from the starter code to call
the respective functions for each packet type.  It currently assumes
that header sizes are always the default, which could be an issue in
some cases.  Then the functions with the names process_X process each
packet of type X.  These functions call helpers from the following files

chunk.c Chunk operations
packet.c Packet creation, parsing
peer_list.c Checking properties of a peer linked list (e.g. length)
download.c Tracking and managing downloads, dealing with incoming data
upload.c Tracking and managing uploads, dealing with outgoing data

The main data structures are

server_state_t in 'server_state.h'
download_t in 'download.h'
upload_t in 'upload.h'

and are used for functioned related to their names.  Thus, our code is
fairly modular.

FEATURES IMPLEMENTED:
Our peer can download a file 100% correctly when neither peer 
disconnects.  It is okay if packets are corrupted or lost (both cases
of which we handle, except for when the first data packet is lost), so
we have basic reliability features.  Currently we support concurrent
uploads and sequential downloads.  It works fine for multiple GET
commands given in a row.  We also accept packets with non-standard
headers.  Finally, our code has extensive debug statements (which
can lean to the verbose side).

TODO:
-Handle abrupt disconnects (e.g. no more data packets, no more acks)
    -Retransmit GET (if no response within timeout)
    -Picking a different peer (or removing the first from the peer list
        if the first peer has crashed) so it is not chosen again
-Handle dropped data packets using timeout
-Handle stalled downloads using timeout
-Handle stalled uploads using timeout
-Implement concurrency for downloads
    -Allow flexible numbers of downloads and uploads instead of fixed
-Implement congestion control
