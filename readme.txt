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

Current issues:
The server currently only implements stop and wait with fixed packet
sizes.  It does not clear uploads properly upons completion (e.g. 
freeing memory and zeroing everything out) and does not reset the
information stored on downloads upon a new GET command.  Upload supports
concurrency.  Download can support concurrency in theory but the behavior
is currently configured only for sequential downloads for simplicity.

The server will likely encounter issues with crashing peers, corrupt
packets, and lost packets.  Although there are some small checks in
place, I did not get to implement major reliability controls like
cumulative ACKs.  I am able to download individual chunks correctly
but there appears to be an issue combining the chunks into a single
file (which does not in any way match the input data file when I
compared using VIMDIFF).  Likewise, I have not implemented any test
code as of yet, although I have implemented extensive debug statements.
