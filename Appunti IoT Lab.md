# Appunti LpIoT
## ```ctimer``` (callback timer)
```c
void ctimer_set(
        struct ctimer *c,
        clock_time_t t,
        void(*f)(void *), /* The CALLBACK FUNCTION */
        void *ptr); /* Data Pointer */

/* Restart timer from the previous expiration time */
void ctimer_reset(struct ctimer *t);

/* Restart the timer from current time */
void ctimer_restart(struct ctimer *t);

/* Stop the timer. */
void ctimer_stop(struct ctimer *t);

/* Check if the timer has expired. */
int ctimer_expired(struct ctimer *t); 
```

## ```etimer``` (event timer)
```c
#define PERIOD CLOCK_SECOND

static struct etimer timer; // event timer structure
etimer_set(&timer, PERIOD); // start timer
PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
// --------------------------------------------------------
PROCESS_THREAD(demo_process, ev, data){
    // ...
    PROCESS_WAIT_EVENT();
    // ev and data are updated
    if(ev==PROCESS_EVENT_TIMER && etimer_expired(&et))
    {
    //...
    }
    else if (ev==some_other_event)
    {
    //...
    }
}
```

## ```Custom Event```
```c
process_event_t alarm_event;

PROCESS_THREAD(demo_process, ev, data) {
    PROCESS_BEGIN();
    alarm_event = process_alloc_event();
    //...
    PROCESS_END();
}

// Send an event process:
process_post(&a_process, /*Destination process*/ 
          // PROCESS_BROADCAST, /* to send to all processes */
             alarm_event,/*Event type*/
             &alarm_event_data); // data passed (or NULL) to the process

// Wait for a specific event
PROCESS_WAIT_EVENT(); // waits for ANY event
// Often we need a specific event (or condition)
do {
    PROCESS_WAIT_EVENT();
} while ( !(ev == PROCESS_EVENT_TIMER && etimer_expired(&et)) )

// There is a shorter way of doing exactly that:
PROCESS_WAIT_EVENT_UNTIL( ev==PROCESS_EVENT_TIMER && etimer_expired(&et) );
// It still wakes up on any event, but goes back to sleep
if the condition is not true
```

## ```Reading the Hardware metrics```
```c
int16_t rssi, lqi;
rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);
lqi = packetbuf_attr(PACKETBUF_ATTR_LINK_QUALITY);
```