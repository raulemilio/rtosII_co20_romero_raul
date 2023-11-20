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

#define TASK_PERIOD_MS_         (1000)
#define QUEUE_LENGTH_            (5)
#define QUEUE_ITEM_SIZE_         (sizeof(event_t_))
#define STATE_LOCKED			 true
#define STATE_DEFAULT			 false

/********************** internal data declaration ****************************/

typedef struct {
	ebtn_state_t state;
} event_t_;

/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/

/********************** external data definition *****************************/

/********************** internal functions definition ************************/

static void task_(void *argument) {
	ao_led_t *hao = (ao_led_t*) argument;
	while (true) {
		event_t_ *event;
		if (pdPASS == xQueueReceive(hao->hqueue, &event, portMAX_DELAY)) {
			ELOG("ao, led:%d", event->state);

			if (event->state == LONG) {
				eboard_led_red_toggle();
			} else {
				if (event->state == LOCKED) {
					eboard_led_red(STATE_LOCKED);
				} else {
					eboard_led_red(STATE_DEFAULT);
				}
			}
			hao->callback(hao, event->state);
			vPortFree((void*) event);
		}
		vTaskDelay((TickType_t) (TASK_PERIOD_MS_ / portTICK_PERIOD_MS));
	}
}

/********************** external functions definition ************************/

bool ao_led_red_send(ao_led_t *hao, ebtn_state_t state) {
	bool ret = false;
	/* Puntero a estructura, en el caso anterior aca la estructura
	 * se creaba de forma estática
	 */
	event_t_ *event;

	event = (event_t_*) pvPortMalloc(sizeof(event_t_));

	if (NULL != event) {
		ELOG("Se alocaron %d bytes", sizeof(event_t_));
		event->state = state;

		if (pdPASS == xQueueSend(hao->hqueue, (void* )&event, 0)) {
			ret = true;
		} else {
			ELOG("No hay lugar en la cola para enviar el mensajes");
			vPortFree((void*) event);
			ELOG("Memoria liberada");
			ret = false;
		}
	} else {
		ELOG("No hay suficiente memoria para alocar más eventos");
	}

	return ret;
}

void ao_led_red_init(ao_led_t *hao, ao_led_callbak_t callback) {
	eboard_led_red(false);

	hao->callback = callback;

	hao->hqueue = xQueueCreate(QUEUE_LENGTH_, QUEUE_ITEM_SIZE_);
	while (NULL == hao->hqueue) {
		// error
	}

	BaseType_t status;
	status = xTaskCreate(task_, "task_ao_led_red", 128, (void* const ) hao,
	tskIDLE_PRIORITY, NULL);
	while (pdPASS != status) {
		// error
	}
}

/********************** end of file ******************************************/
