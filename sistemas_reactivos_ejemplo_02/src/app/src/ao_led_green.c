/*
 * Copyright (c) 2023 Sebastian Bedin <sebabedin@gmail.com>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @file   : ao_led.c
 * @date   : 13-11-23
 * @author : Raúl Romero <rraulemilioromero@gmail.com>
 * @version	v1.0.0
 */

/********************** inclusions *******************************************/


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "driver.h"
#include "ao_led_red.h"
#include "task_button.h"

/********************** macros and definitions *******************************/

#define TASK_PERIOD_MS_          (1000)
#define QUEUE_LENGTH_            (5)
#define QUEUE_ITEM_SIZE_         (sizeof(event_t_))
#define STATE_LOCKED			 true
#define STATE_DEFAULT			 false

/********************** internal data declaration ****************************/

typedef struct
{
  ebtn_state_t state;
} event_t_;

/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/

/********************** external data definition *****************************/

/********************** internal functions definition ************************/

static void task_(void* argument)
{
  ao_led_t* hao = (ao_led_t*)argument;
  while(true)
  {
    event_t_ event;
    if(pdPASS == xQueueReceive(hao->hqueue, &event, portMAX_DELAY))
    {
        ELOG("ao, led:%d", event.state);

        if (event.state==SHORT){
            eboard_led_green_toggle();
        }
        else
        {
      	  if (event.state==LOCKED){
      	      eboard_led_green(STATE_LOCKED);
      	  }
      	  else
      	  {
      		  eboard_led_green(STATE_DEFAULT);
      	  }
        }
        hao->callback(hao, event.state);
    }
    vTaskDelay((TickType_t)(TASK_PERIOD_MS_ / portTICK_PERIOD_MS));
  }
}

/********************** external functions definition ************************/

bool ao_led_green_send(ao_led_t* hao, ebtn_state_t state)
{
  event_t_ event;
  event.state = state;
  return (pdPASS == xQueueSend(hao->hqueue, (void*)&event, 0));
}

void ao_led_green_init(ao_led_t* hao, ao_led_callbak_t callback)
{
  eboard_led_green(false);

  hao->callback = callback;

  hao->hqueue = xQueueCreate(QUEUE_LENGTH_, QUEUE_ITEM_SIZE_);
  while(NULL == hao->hqueue)
  {
    // error
  }

  BaseType_t status;
  status = xTaskCreate(task_, "task_ao_led_green", 128, (void* const)hao, tskIDLE_PRIORITY, NULL);
  while (pdPASS != status)
  {
    // error
  }
}

/********************** end of file ******************************************/
