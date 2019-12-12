Group 3
Kyle Patterson
Tahsin Oshin

I added onto the process UDP function from the starter code to call
the respective functions for each packet type.  It currently assumes
that header sizes are always the default, which could be an issue in
some cases.  Then the functions with the names process_X process each
packet of type X.  These functions call helpers from the following files

chunk.c Chunk operations
packet.c Packet creation, parsing
peer_list.c Checking properties of a peer linked list (e.g. length)
download.c Tracking and managing downloads, dealing with incoming data
upload.c Tracking and managing uploads, dealing with outgoing data

TODO:
-Handle abrupt disconnects
-Retransmit get
-Implement concurrency
-Implement congestion control
