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
#define SLOT_TIME ((clock_time_t)(CLOCK_SECOND * MAX_HOPS * 0.005))
#define GUARD_TIME ((clock_time_t)(CLOCK_SECOND * MAX_HOPS * 0.05))
/*---------------------------------------------------------------------------*/
PROCESS(sink_process, "Sink process");
PROCESS(node_process, "Node process");
/*---------------------------------------------------------------------------*/
/* Callback function declarations */
void bc_recv(struct broadcast_conn *conn, const linkaddr_t *sender);
void uc_recv(struct unicast_conn *c, const linkaddr_t *from);
/* Other function declarations */
void send_beacon();
void send_collect();
void sleep_cb(void *p) { NETSTACK_MAC.off(false); }
void wakeup_cb(void *p);
/*---------------------------------------------------------------------------*/
/* Rime Callback structures */
struct broadcast_callbacks bc_cb = {
    .recv = bc_recv,
    .sent = NULL};
struct unicast_callbacks uc_cb = {
    .recv = uc_recv,
    .sent = NULL};
/*---------------------------------------------------------------------------*/
static struct etimer beacon_etimer;
static struct ctimer beacon_ctimer;
static struct etimer collect_timer;
static struct ctimer sleep_timer;
static struct ctimer wakeup_timer;
static clock_time_t process_time;
process_event_t collect_event;
struct sched_collect_conn *conn_ptr;

PROCESS_THREAD(sink_process, ev, data)
{
  PROCESS_BEGIN();
  collect_event = process_alloc_event();
  etimer_set(&beacon_etimer, (clock_time_t)0);

  // periodically transmit the synchronization beacon
  while (1)
  {
    PROCESS_WAIT_EVENT();

    if (ev == PROCESS_EVENT_TIMER && etimer_expired(&beacon_etimer))
    {
      NETSTACK_MAC.on();
      conn_ptr->beacon_seqn++;
      send_beacon(NULL);

      etimer_set(&beacon_etimer, EPOCH_DURATION);
      etimer_set(&collect_timer, MAX_HOPS * CLOCK_SECOND);
      ctimer_set(&sleep_timer, MAX_HOPS * CLOCK_SECOND + (MAX_NODES-1) * SLOT_TIME, sleep_cb, NULL);
    }
    else if (ev == PROCESS_EVENT_TIMER && etimer_expired(&collect_timer))
    {
      printf("collect: %u in collection phase\n", node_id);
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(node_process, ev, data)
{
  PROCESS_BEGIN();
  collect_event = process_alloc_event();
  clock_time_t tot_delay = 0;

  // manage the phases of each epoch
  while (1)
  {
    PROCESS_WAIT_EVENT();

    if (ev == collect_event)
    {
      tot_delay = (*(clock_time_t *)data);
      etimer_set(&collect_timer, MAX_HOPS * CLOCK_SECOND + ((node_id - 2) * SLOT_TIME) - tot_delay);
      ctimer_set(&sleep_timer, MAX_HOPS * CLOCK_SECOND + (MAX_NODES - 1) * SLOT_TIME - tot_delay, sleep_cb, NULL);
      ctimer_set(&wakeup_timer, EPOCH_DURATION - tot_delay - GUARD_TIME, wakeup_cb, NULL);
    }
    else if (ev == PROCESS_EVENT_TIMER && etimer_expired(&collect_timer))
      send_collect();
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
  conn->pending_msg.busy = false;

  linkaddr_copy(&conn->parent, &linkaddr_null);
  conn->metric = 65535;
  conn->beacon_seqn = 0;
  conn->callbacks = callbacks;

  broadcast_open(&conn->bc, channels, &bc_cb);
  unicast_open(&conn->uc, channels + 1, &uc_cb);
  conn_ptr = conn;

  if (is_sink)
  {
    conn->metric = 0;
    conn->delay = 0;
    process_start(&sink_process, conn);
  }
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

  if (c->pending_msg.busy)
    return 0;
  else
  {
    // printf("collect: pre copy %d, old mem %d\n", *((uint16_t *) data), *((uint16_t *) c->pending_msg.data));
    #ifndef CONTIKI_TARGET_SKY
    c->pending_msg.data = data; // for some reason in Zolertia Firefly I cannot copy the content of data
    #else
    memcpy(c->pending_msg.data, data, len);
    #endif
    // unsigned short i;
    // for (i = 0; i < len; i++){
    //   printf("collect: bef cp %u, %u = %u\n", i, c->pending_msg.data[i], data[i]);
    //   c->pending_msg.data[i] = data[i];
    //   printf("collect: aftr cp %u, %u = %u\n", i, c->pending_msg.data[i], data[i]);
    // }
    
    c->pending_msg.len = len;
    c->pending_msg.busy = true;

    // printf("collect: post copy %d\n", *((uint16_t *) c->pending_msg.data));
    return 1;
  }
}
/*---------------------------------------------------------------------------*/
/* Routing and synchronization beacons */
struct beacon_msg
{ // Beacon message structure
  uint16_t seqn;
  uint16_t metric;    // TODO: use LQI?
  clock_time_t delay; // embed the transmission delay to help nodes synchronize
} __attribute__((packed));
/* Header structure for data packets */
struct collect_header
{
  linkaddr_t source;
  uint8_t hops;
} __attribute__((packed));
/*---------------------------------------------------------------------------*/
/* Beacon receive callback */
void bc_recv(struct broadcast_conn *bc_conn, const linkaddr_t *sender)
{
  process_time = clock_time();
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
  clock_time_t tot_delay = beacon.delay;

  printf("collect: recv beacon from %02x:%02x, seqn %u, metric %u, rssi %d, delay %u - my_seqn %u, my_metric %u\n",
         sender->u8[0], sender->u8[1],
         beacon.seqn, beacon.metric, rssi, (u_int16_t)tot_delay, conn->beacon_seqn, conn->metric);

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
    tot_delay += (clock_time() - process_time) * 2 + 1;
    conn->delay = new_delay + tot_delay;

    process_post(&node_process, collect_event, &tot_delay);

    if (conn->metric < MAX_HOPS) // do not send beacons with metric >= MAX_HOPS
      ctimer_set(&beacon_ctimer, new_delay, send_beacon, NULL);
  }
}
/*---------------------------------------------------------------------------*/
/* Data receive callback */
void uc_recv(struct unicast_conn *uc_conn, const linkaddr_t *from)
{
  if (packetbuf_datalen() < sizeof(struct collect_header))
  {
    printf("collect: too short unicast packet %d\n", packetbuf_datalen());
    return;
  }

  struct collect_header hdr;

  if (conn_ptr->metric == 0)
  {
    memcpy(&hdr, packetbuf_dataptr(), sizeof(struct collect_header));
    packetbuf_hdrreduce(sizeof(struct collect_header));

    linkaddr_t source = hdr.source;
    conn_ptr->callbacks->recv(&source, hdr.hops + 1);
  }
  else
  {
    struct collect_header *hdr_ptr = packetbuf_dataptr();
    hdr_ptr->hops++;
    unicast_send(&conn_ptr->uc, &conn_ptr->parent);
  }
}
/*---------------------------------------------------------------------------*/
/* Send beacon using the current seqn and metric */
void send_beacon(void *ptr)
{
  struct sched_collect_conn *conn = conn_ptr;
  struct beacon_msg beacon = {
      .seqn = conn->beacon_seqn,
      .metric = conn->metric,
      .delay = conn->delay};

  packetbuf_clear();
  packetbuf_copyfrom(&beacon, sizeof(beacon));
  printf("collect: sending beacon: seqn %d metric %d\n", conn->beacon_seqn, conn->metric);
  broadcast_send(&conn->bc);
}
/*---------------------------------------------------------------------------*/
/* Send collect msg with unicast */
void send_collect()
{
  if (!conn_ptr->pending_msg.busy || linkaddr_cmp(&conn_ptr->parent, &linkaddr_null))
    return;

  struct msg_buffer *msg = &conn_ptr->pending_msg;
  struct collect_header hdr = {.source = linkaddr_node_addr, .hops = 0};

  // add data to buffer
  packetbuf_clear();
  memcpy(packetbuf_dataptr(), msg->data, msg->len);
  packetbuf_set_datalen(msg->len);

  // add header
  packetbuf_hdralloc(sizeof(struct collect_header));
  memcpy(packetbuf_hdrptr(), &hdr, sizeof(struct collect_header));

  // send packet
  printf("collect: %u sending msg\n", node_id);
  unicast_send(&conn_ptr->uc, &conn_ptr->parent);
  conn_ptr->pending_msg.busy = false; // free the buffer
}
/*---------------------------------------------------------------------------*/
/* wake up callback */
void wakeup_cb(void *p)
{
  NETSTACK_MAC.on();
  conn_ptr->metric = 65535;
}