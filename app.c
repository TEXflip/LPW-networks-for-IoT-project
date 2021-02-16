#include "contiki.h"
#include "lib/random.h"
#include "net/rime/rime.h"
#include "leds.h"
#include "net/netstack.h"
#include <stdio.h>
#include "core/net/linkaddr.h"
/*---------------------------------------------------------------------------*/
#include "sched_collect.h"
#include "sys/node-id.h"
#include "deployment.h"
#include "simple-energest.h"
/*---------------------------------------------------------------------------*/
#ifndef CONTIKI_TARGET_SKY
linkaddr_t sink = {{0xF7, 0x9C}}; /* Testebed: Firefly node 1 will be sink */
#else
linkaddr_t sink = {{0x01, 0x00}}; /* Cooja: Sky node 1 will be our sink */
#endif
/*---------------------------------------------------------------------------*/
/* Application packet */
typedef struct {
  uint16_t seqn;
}
__attribute__((packed))
test_msg_t;
/*---------------------------------------------------------------------------*/
PROCESS(app_process, "App process");
AUTOSTART_PROCESSES(&app_process);
/*---------------------------------------------------------------------------*/
static struct sched_collect_conn sched_collect;
static void recv_cb(const linkaddr_t *originator, uint8_t hops);
struct sched_collect_callbacks cb = {.recv = recv_cb};
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(app_process, ev, data)
{
  static struct etimer et;
  static test_msg_t msg = {.seqn=0};
  static int ret = 0;

  PROCESS_BEGIN();
  printf("CLOCK_SECOND: %lu\n", CLOCK_SECOND);

  /* Set some specific testbed/cooja deployment configuration */
  deployment_init();

  /* Start energest to estimate node duty cycle */
  simple_energest_start();

  if(linkaddr_cmp(&sink, &linkaddr_node_addr)) {
    printf("App: I am sink %02x:%02x with node_id %u\n",
      linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1], node_id);
    etimer_set(&et, CLOCK_SECOND * 2);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    sched_collect_open(&sched_collect, COLLECT_CHANNEL, true, &cb);
  }
  else {
    printf("App: I am normal node %02x:%02x with node_id %u\n",
      linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1], node_id);
    sched_collect_open(&sched_collect, COLLECT_CHANNEL, false, NULL);

    etimer_set(&et, EPOCH_DURATION);
    while(1) {
      /* Set data packet to be sent in the data collection time window */
      ret = sched_collect_send(&sched_collect, (uint8_t *) &msg, sizeof(msg));
      if (ret != 0)
        printf("App: Send seqn %d\n", msg.seqn);
      else
        printf("App: packet with seqn %d could not be scheduled.\n",
          msg.seqn);
      msg.seqn++;
      /* Reset timer */
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
      etimer_reset(&et);
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
static void
recv_cb(const linkaddr_t *originator, uint8_t hops)
{
  test_msg_t msg;
  if (packetbuf_datalen() != sizeof(msg)) {
    printf("App: wrong length: %d\n", packetbuf_datalen());
    return;
  }
  memcpy(&msg, packetbuf_dataptr(), sizeof(msg));
  printf("App: Recv from %02x:%02x seqn %d hops %d\n",
    originator->u8[0], originator->u8[1], msg.seqn, hops);
}
/*---------------------------------------------------------------------------*/
