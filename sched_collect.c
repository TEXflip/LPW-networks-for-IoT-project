#include <stdbool.h>
#include "contiki.h"
#include "lib/random.h"
#include "net/rime/rime.h"
#include "leds.h"
#include "net/netstack.h"
#include <stdio.h>
#include "core/net/linkaddr.h"
#include "node-id.h"
#include "sched_collect.h"
/*---------------------------------------------------------------------------*/
#define RSSI_THRESHOLD -95 // filter bad links
/*---------------------------------------------------------------------------*/
PROCESS(sink_process, "Sink process");
PROCESS(node_process, "Node process");
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sink_process, ev, data)
{
  PROCESS_BEGIN();
  // periodically transmit the synchronization beacon
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(node_process, ev, data)
{
  PROCESS_BEGIN();
  // manage the phases of each epoch
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void
sched_collect_open(struct sched_collect_conn* conn, uint16_t channels,
  bool is_sink, const struct sched_collect_callbacks *callbacks)
{
  /* Create 2 Rime connections: broadcast (for beacons) and unicast (for collection)
   * Start the appropriate process to perform the necessary epoch operations or 
   * use ctimers and callbacks as necessary to schedule these operations.
   */
}
/*---------------------------------------------------------------------------*/
int
sched_collect_send(struct sched_collect_conn *c, uint8_t *data, uint8_t len)
{
  /* Store packet in a local buffer to be send during the data collection 
   * time window. If the packet cannot be stored, e.g., because there is
   * a pending packet to be sent, return zero. Otherwise, return non-zero
   * to report operation success. */
  return 0; 
}
/*---------------------------------------------------------------------------*/
/* Routing and synchronization beacons */
struct beacon_msg { // Beacon message structure
  uint16_t seqn;
  uint16_t metric;
  clock_time_t delay; // embed the transmission delay to help nodes synchronize
} __attribute__((packed));
/*---------------------------------------------------------------------------*/
