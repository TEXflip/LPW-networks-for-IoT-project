#ifndef SCHED_COLLECT_H
#define SCHED_COLLECT_H
/*---------------------------------------------------------------------------*/
#include <stdbool.h>
#include "contiki.h"
#include "net/rime/rime.h"
#include "net/netstack.h"
#include "core/net/linkaddr.h"
/*---------------------------------------------------------------------------*/
#define EPOCH_DURATION (30 * CLOCK_SECOND)  // collect every minute
/*---------------------------------------------------------------------------*/
#ifndef CONTIKI_TARGET_SKY
/* Testbed experiments with Zoul Firefly platform */
#define MAX_HOPS 4
#define MAX_NODES 35
#else
/* Cooja experiments with Tmote Sky platform */
#define MAX_HOPS 3
#define MAX_NODES 9
#endif
/*---------------------------------------------------------------------------*/
#define COLLECT_CHANNEL 0xAA
/*---------------------------------------------------------------------------*/
/* Callback structure */
struct sched_collect_callbacks {
  void (* recv)(const linkaddr_t *originator, uint8_t hops);
};
/*---------------------------------------------------------------------------*/
/* Connection object */
struct sched_collect_conn {
  struct broadcast_conn bc;
  struct unicast_conn uc;
  const struct sched_collect_callbacks* callbacks;
  linkaddr_t parent;
  struct ctimer beacon_timer;
  uint16_t metric;
  uint16_t beacon_seqn;
  // you can add other useful variables to the object
};
/*---------------------------------------------------------------------------*/
/* Initialize a collect connection
 *  - conn -- a pointer to a connection object
 *  - channels -- starting channel C (the collect uses two: C and C+1)
 *  - is_sink -- initialize in either sink or router mode
 *  - callbacks -- a pointer to the callback structure */
void sched_collect_open(
    struct sched_collect_conn* conn,
    uint16_t channels,
    bool is_sink,
    const struct sched_collect_callbacks *callbacks);
/*---------------------------------------------------------------------------*/
/* Send packet to the sink 
 * Parameters:
 *  - conn -- a pointer to a connection object
 *  - data -- a pointer to the data packet to be sent
 *  - len  -- data length to be send in bytes
 * 
 * Returns zero if the packet cannot be stored nor sent.
 * Non-zero otherwise.
 */
int sched_collect_send(
    struct sched_collect_conn *c,
    uint8_t *data,
    uint8_t len);
/*---------------------------------------------------------------------------*/
#endif //SCHED_COLLECT_H
