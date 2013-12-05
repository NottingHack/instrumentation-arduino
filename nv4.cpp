/*
 * Copyright (c) 2013, Daniel Swann <hs@dswann.co.uk>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the owner nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <Arduino.h>
#include <SoftwareSerial.h>
#include "nv4.h"

static Stream *serial;
static SoftwareSerial *soft_serial;

static void nv4_write(uint8_t b)
{
  serial->write(b);
  delay(100);
}

void nv4_init(nv4_cmd_rcv cb, uint8_t rx, uint8_t tx)
/* Init nv4 using softserial */
{
  cmd_rcv_cb = cb;
  serial = soft_serial = new SoftwareSerial(rx, tx);
  soft_serial->begin(300);
  nv4_disable_all();
}

void nv4_init(nv4_cmd_rcv cb)
/* Init nv4 using hardware serial */
{
  cmd_rcv_cb = cb;
  Serial.begin(300);
  serial = &Serial;

  /* Set 2 stop bits */
  UCSR0C = (1<<USBS0);

  nv4_disable_all();
}

uint8_t nv4_loop()
{
  uint8_t in;

  if (serial->available() > 0)
  {
    in = serial->read();

    if ((in >= (NV4_ACCEPT+NV4_CHANNEL1)) &&
        (in <= (NV4_ACCEPT+NV4_CHANNEL16)))
    {
      cmd_rcv_cb(NV4_ACCEPT, in);
    }
    else
    {
      cmd_rcv_cb(in, 0);
    }
    return 1;
  } else
    return 0;
}

void nv4_inhibit(uint8_t channel)
{
  nv4_write(NV4_INHIBIT + channel);
}

void nv4_uninhibit(uint8_t channel)
{
  nv4_write(NV4_UNINHIBIT + channel);
}

void nv4_enable_escrow()
{
  nv4_write(NV4_ENABLE_ESCROW);
}

void nv4_disable_escrow()
{
  nv4_write(NV4_DISABLE_ESCROW);
}

void nv4_accept_escrow()
{
  nv4_write(NV4_ACCEPT_ESCROW);
}

void nv4_reject_escrow()
{
  nv4_write(NV4_REJECT_ESROW);
}

void nv4_enable_all()
{
  nv4_write(NV4_ENABLE_ALL);
}

void nv4_disable_all()
{
  nv4_write(NV4_DISABLE_ALL);
}
