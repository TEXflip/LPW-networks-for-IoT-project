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
#define BEACON_FORWARD_DELAY (random_rand() % CLOCK_SECOND)
#define SEQN_OVERFLOW_TH 3 // number of accepting SEQN after overflow
/*---------------------------------------------------------------------------*/
PROCESS(sink_process, "Sink process");
PROCESS(node_process, "Node process");
/*---------------------------------------------------------------------------*/
/* Callback function declarations */
void bc_recv(struct broadcast_conn *conn, const linkaddr_t *sender);
void uc_recv(struct unicast_conn *c, const linkaddr_t *from);
/* Other function declarations */
void send_beacon(void *ptr);
void beacon_timer_cb(void *ptr);
/*---------------------------------------------------------------------------*/
/* Rime Callback structures */
struct broadcast_callbacks bc_cb = {
    .recv = bc_recv,
    .sent = NULL};
struct unicast_callbacks uc_cb = {
    .recv = uc_recv,
    .sent = NULL};
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sink_process, ev, data)
{
  PROCESS_BEGIN();

  struct sched_collect_conn *conn = (struct sched_collect_conn *)data;

  conn->metric = 0;
  conn->delay = 0;

  ctimer_set(&conn->beacon_timer, 0 * CLOCK_SECOND, beacon_timer_cb, conn);

  // periodically transmit the synchronization beacon
  while (1)
  {

    PROCESS_WAIT_EVENT();
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(node_process, ev, data)
{
  PROCESS_BEGIN();

  // struct sched_collect_conn *conn = (struct sched_collect_conn *)data;

  // manage the phases of each epoch
  while (1)
  {
    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void sched_collect_open(struct sched_collect_conn *conn, uint16_t channels,
                        bool is_sink, const struct sched_collect_callbacks *callbacks)
{
  /* Create 2 Rime connections: broadcast (for beacons) and unicast (for collection)
   * Start the appropriate process to perform the necessary epoch operations or 
   * use ctimers and callbacks as necessary to schedule these operations.
   */

  linkaddr_copy(&conn->parent, &linkaddr_null);
  conn->metric = 65535;
  conn->beacon_seqn = 0;
  conn->callbacks = callbacks;

  // NETSTACK_MAC.on(); // TODO: handle duty cycle

  broadcast_open(&conn->bc, channels, &bc_cb);
  unicast_open(&conn->uc, channels + 1, &uc_cb);

  if (is_sink)
    process_start(&sink_process, conn);
  else
    process_start(&node_process, conn);
}
/*---------------------------------------------------------------------------*/
int sched_collect_send(struct sched_collect_conn *c, uint8_t *data, uint8_t len)
{
  /* Store packet in a local buffer to be send during the data collection 
   * time window. If the packet cannot be stored, e.g., because there is
   * a pending packet to be sent, return zero. Otherwise, return non-zero
   * to report operation success. */
  return 0;
}
/*---------------------------------------------------------------------------*/
/* Routing and synchronization beacons */
struct beacon_msg
{ // Beacon message structure
  uint16_t seqn;
  uint16_t metric;    // TODO: use LQI?
  clock_time_t delay; // embed the transmission delay to help nodes synchronize
} __attribute__((packed));
/*---------------------------------------------------------------------------*/
/* Beacon receive callback */
void bc_recv(struct broadcast_conn *bc_conn, const linkaddr_t *sender)
{
  struct beacon_msg beacon;
  int16_t rssi;
  struct sched_collect_conn *conn = (struct sched_collect_conn *)(((uint8_t *)bc_conn) - offsetof(struct sched_collect_conn, bc));

  if (packetbuf_datalen() != sizeof(struct beacon_msg))
  {
    printf("collect: broadcast of wrong size\n");
    return;
  }

  memcpy(&beacon, packetbuf_dataptr(), sizeof(struct beacon_msg));
  rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);
  printf("collect: recv beacon from %02x:%02x, seqn %u, metric %u, rssi %d, timestamp: %lu-%u=%lu\n",
         sender->u8[0], sender->u8[1],
         beacon.seqn, beacon.metric, rssi, (clock_time() * 1000 / CLOCK_SECOND), (u_int16_t)beacon.delay, (clock_time() * 1000 / CLOCK_SECOND) - (u_int16_t)beacon.delay);

  uint16_t my_seqn = conn->beacon_seqn, beacon_seqn = beacon.seqn;

  if ((beacon_seqn >= my_seqn ||
       (my_seqn >= 65536 - SEQN_OVERFLOW_TH && beacon_seqn <= (my_seqn + SEQN_OVERFLOW_TH) % 65536)) && // cheap way to handle overflow
      beacon.metric < conn->metric &&                                                                   // accept better metrics
      rssi > RSSI_THRESHOLD)                                                                            // discard bad RSSI
  {
    conn->metric = beacon.metric + 1;
    conn->parent = *sender;
    conn->beacon_seqn = beacon_seqn;

    clock_time_t new_delay = BEACON_FORWARD_DELAY;
    conn->delay = new_delay + beacon.delay;

    if (conn->metric <= MAX_HOPS) // do not send beacons with metric > MAX_HOPS
      ctimer_set(&conn->beacon_timer, new_delay, send_beacon, conn);
  }
}
/*---------------------------------------------------------------------------*/
/* Data receive callback */
void uc_recv(struct unicast_conn *uc_conn, const linkaddr_t *from)
{
}
/*---------------------------------------------------------------------------*/
/* beacon event callback */
void beacon_timer_cb(void *ptr)
{
  struct sched_collect_conn *conn = (struct sched_collect_conn *)ptr;

  send_beacon(conn);
  conn->beacon_seqn++;

  ctimer_set(&conn->beacon_timer, EPOCH_DURATION, beacon_timer_cb, conn);
}
/* Send beacon using the current seqn and metric */
void send_beacon(void *ptr)
{
  struct sched_collect_conn *conn = (struct sched_collect_conn *)ptr;
  struct beacon_msg beacon = {
      .seqn = conn->beacon_seqn,
      .metric = conn->metric,
      .delay = conn->delay};

  packetbuf_clear();
  packetbuf_copyfrom(&beacon, sizeof(beacon));
  printf("collect: sending beacon: seqn %d metric %d\n", conn->beacon_seqn, conn->metric);
  broadcast_send(&conn->bc);
}