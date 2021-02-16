/*---------------------------------------------------------------------------*/
#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_
/*---------------------------------------------------------------------------*/
/* Change to match your configuration */
#define IEEE802154_CONF_PANID         0xABCD

/* Enable energy estimation */
#define ENERGEST_CONF_ON              1

/* System clock runs at 32 MHz */
#define SYS_CTRL_CONF_SYS_DIV         SYS_CTRL_CLOCK_CTRL_SYS_DIV_32MHZ

/* IO clock runs at 32 MHz */
#define SYS_CTRL_CONF_IO_DIV          SYS_CTRL_CLOCK_CTRL_IO_DIV_32MHZ

#define NETSTACK_CONF_WITH_IPV6       0

#define CC2538_RF_CONF_CHANNEL        26

#define COFFEE_CONF_SIZE              0

#define LPM_CONF_MAX_PM               LPM_PM0
/*---------------------------------------------------------------------------*/
#undef NETSTACK_CONF_RDC
#define NETSTACK_CONF_RDC nullrdc_driver
#define NULLRDC_CONF_802154_AUTOACK   1
/*---------------------------------------------------------------------------*/
#undef CLOCK_CONF_SECOND
#define CLOCK_CONF_SECOND 1024UL
/*---------------------------------------------------------------------------*/
#endif /* PROJECT_CONF_H_ */
/*---------------------------------------------------------------------------*/
