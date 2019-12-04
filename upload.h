/* Upload header file */


#define MAX_UPLOADS 5 // max no. of simultaneous connections for upload
#define MAX_SEEDERS 20 // max # peers conn. during a single upload
#define TIMEOUT_DATA 10 // ms
#define TIMEOUT_ACK 10 // ms
#define MAX_RETRIES_SEND_DATA 10
#define MAX_RETRIES_REC_DATA 10
#define SEQ_NUM 0
#define ACK_NUM 0
 
 typedef struct upload {
     char hash[20]; // chunk to upload 
     data_packet_t *packets;  // chunks divided to packets in array
     int recv[]; // to keep count of packets sent for upload and recv 
     int ack_ind; //ack as int to index into recv
     clock_t last_ack_rec;
    } upload_t;


    typedef struct upload{
        upload_t uploads[MAX_UPLOADS];
    }upload_t;

    // Function to get ID for a specific chunk HASH
// Function to read a chunk based on its specific ID from a file; should
//   also verify that chunk is not corrupt using hash, then return a pointer to that chunk
// Function to divide a specific chunk into packets and store those packets in data structure
// Function to send packets
// Function to check timeouts for GET and DATA
// Function to process a GET request from another client -> do something, then update the struct
// Function to process an ACK from another client -> do something, then update the struct
// Function to process a DATA packet from another client -> do something, then update the struct