/*
 * Copyright (c) 2020, University of Trento.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**
 * \file
 *      Deployment file for Zoul testbed setup
 *
 * \author
 *      Pablo Corbalan <p.corbalanpelegrin@unitn.it>
 */

#ifndef __DEPLOYMENT_H__
#define __DEPLOYMENT_H__
/*---------------------------------------------------------------------------*/
#include "net/linkaddr.h"
/*---------------------------------------------------------------------------*/
/* ID <-> MAC address mapping */
typedef struct id_mac {
  uint8_t id;
  linkaddr_t mac;
} id_mac_t;
/*---------------------------------------------------------------------------*/
#ifndef CONTIKI_TARGET_SKY
extern unsigned short node_id;
#endif
/*---------------------------------------------------------------------------*/
/* Function to set node_id of a node on a testbed */
void deployment_set_node_id_from_lladdr(linkaddr_t *addr);
/*---------------------------------------------------------------------------*/
void deployment_init(void);
/*---------------------------------------------------------------------------*/
#endif /* __DEPLOYMENT_H__ */
