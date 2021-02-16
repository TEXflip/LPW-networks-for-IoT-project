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
 *      Deployment 
 *
 * \author
 *      Pablo Corbalan <pablo.corbalan@cit.ie>
 */

#include "contiki.h"
#include "deployment.h"
#include "sys/node-id.h"
#include <stdio.h>
/*---------------------------------------------------------------------------*/
#ifndef CONTIKI_TARGET_SKY
unsigned short node_id = 0;
#endif
/*---------------------------------------------------------------------------*/
/* List of ID <-> MAC mapping used for different deployments */
static const id_mac_t id_mac_list[] = {
#ifndef CONTIKI_TARGET_SKY
  { 1,  {{0xf7, 0x9c}}},
  { 2,  {{0xd9, 0x76}}},
  { 3,  {{0xf3, 0x84}}},
  { 4,  {{0xf3, 0xee}}},
  { 5,  {{0xf7, 0x92}}},
  { 6,  {{0xf3, 0x9a}}},
  { 7,  {{0xde, 0x21}}},
  { 8,  {{0xf2, 0xa1}}},
  { 9,  {{0xd8, 0xb5}}},
  {10,  {{0xf2, 0x1e}}},
  {11,  {{0xd9, 0x5f}}},
  {12,  {{0xf2, 0x33}}},
  {13,  {{0xde, 0x0c}}},
  {14,  {{0xf2, 0x0e}}},
  {15,  {{0xd9, 0x49}}},
  {16,  {{0xf3, 0xdc}}},
  {17,  {{0xd9, 0x23}}},
  {18,  {{0xf3, 0x8b}}},
  {19,  {{0xf3, 0xc2}}},
  {20,  {{0xf3, 0xb7}}},
  {21,  {{0xde, 0xe4}}},
  {22,  {{0xf3, 0x88}}},
  {23,  {{0xf7, 0x9a}}},
  {24,  {{0xf7, 0xe7}}},
  {25,  {{0xf2, 0x85}}},
  {26,  {{0xf2, 0x27}}},
  {27,  {{0xf2, 0x64}}},
  {28,  {{0xf3, 0xd3}}},
  {29,  {{0xf3, 0x8d}}},
  {30,  {{0xf7, 0xe1}}},
  {31,  {{0xde, 0xaf}}},
  {32,  {{0xf2, 0x91}}},
  {33,  {{0xf2, 0xd7}}},
  {34,  {{0xf3, 0xa3}}},
  {35,  {{0xf2, 0xd9}}},
  {36,  {{0xd9, 0x9f}}},
#else
  {0,   {{0x00, 0x00}}},
#endif
};
/*---------------------------------------------------------------------------*/
void
deployment_set_node_id_from_lladdr(linkaddr_t *addr)
{
#ifdef CONTIKI_TARGET_SKY
  /* Tmote Sky platform with Cooja: 
   * Cooja directly assigns the node_id variable */
  return;
#else 
  /* Testbed experiment */
  if(addr == NULL) {
    node_id = 0;
  } else {
    const id_mac_t *curr = id_mac_list;
    while(curr->id != 0) {
      /* Assume network-wide unique 16-bit MAC addresses */
      if(curr->mac.u8[0] == addr->u8[0] && curr->mac.u8[1] == addr->u8[1]) {
        node_id = curr->id;
        printf("Deployment: node_id %d\n", node_id);
        return;
      }
      curr++;
    }
  }
#endif
}
/*---------------------------------------------------------------------------*/
void
deployment_init(void)
{
  deployment_set_node_id_from_lladdr(&linkaddr_node_addr);
}
/*---------------------------------------------------------------------------*/
