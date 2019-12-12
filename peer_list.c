#include <sys/param.h>
#include "bt_parse.h"

/* 
 * peer_list.c
 *
 * Counting and checking various properties on a peer list.
 */

/*                                                                         
 * id_in_ids                                                               
 *                                                                         
 * Returns a boolean for whether a given integer `id` is in an array       
 * of integers `ids` of length `ids_len`.                                  
 */                                                                        
int id_in_ids(int id, int *ids, int ids_len) {                             
  for (int i = 0; i < ids_len; i++) {                                      
    if (id == ids[i])                                                      
      return 1;                                                            
  }                                                                        
  return 0;                                                                
}

/*                                                                         
 * get_largest_peer_id                                                     
 *                                                                         
 * Helper function which returns the largest peer id in the peers list. 
 */                                                                        
int get_largest_peer_id(bt_config_t *config) {                             
  bt_peer_t *p = config->peers;                                            
  int largest = p->id;                                                     
  for (p = p->next; p != NULL; p = p->next) {                              
    largest = MAX(largest, p->id);                                         
  }                                                                        
  return largest;                                                          
}                                                                          
                                                                           
/* Returns the number of peers in the network. */                          
int get_n_peers(bt_config_t *config) {                                     
  int n = 0;                                                               
  for (bt_peer_t *p = config->peers; p != NULL; p = p->next)                                
    n++;                                                                   
  return n;                                                                
}      
