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
 * @file   : task_button.c
 * @date   : 13-11-23
 * @author : Raúl Romero <rraulemilioromero@gmail.com>
 * @version	v1.0.0
 */

/********************** inclusions *******************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "driver.h"
#include "task_button.h"
#include "ao_led_red.h"
#include "ao_led_green.h"
#include "API_delay.h"

/********************** macros and definitions *******************************/

#define TASK_PERIOD_MS_         (1000)
#define BUTTON_PRESS             true
#define BUTTON_RELEASED          false
#define TIME_DETECT				 100
#define TIME_SHORT				 1000
#define TIME_LONG				 8000
#define DEBOUNCE_TIME 	   		 200 // tiempo de delay para antirebote


/********************** internal data declaration ****************************/

// variable para la FSM
typedef enum {
	BUTTON_UP, BUTTON_FALLING, BUTTON_DOWN, BUTTON_RAISING,
} debounceState_t;

debounceState_t debounce_state; // variable de la FSM
delay_t debounceDelay; // variable de delay no bloqueante usado para tiempo antirebote

/********************** internal functions declaration ***********************/

static void debounceFSM_init(void);		// Se carga el estado inicial de la FSM
static void debounceFSM_update(void);	// Actualización de la FSM
void buttonPressed(void);
void buttonReleased(void);

static void get_btn_state(void);

/********************** internal data definition *****************************/

static ao_led_t ao_led_red;
static ao_led_t ao_led_green;

static bool led_state_red;
static bool led_state_green;

static uint32_t pressed_time;
static uint32_t released_time;
static uint32_t delta_time = 0;

ebtn_state_t btn_state;

/********************** external data definition *****************************/

/********************** internal functions definition ************************/

void led_red_state_callback_(ao_led_t* hao, bool state)
{
  led_state_red= state;
  ELOG("callback, led:%d", led_state_red);
}

void led_green_state_callback_(ao_led_t* hao, bool state)
{
  led_state_green= state;
  ELOG("callback, led:%d", led_state_green);
}

static void debounceFSM_init(void) {
	// Inicialización de la variable de estado
	debounce_state = BUTTON_UP;
	// inicialización del tiempo de delay para el retardo
	delayInit(&debounceDelay, DEBOUNCE_TIME);
}

static void debounceFSM_update(void) {
	switch (debounce_state) {
	case BUTTON_UP:
		// Chequear condiciones de transición de estado
		if (eboard_switch() == BUTTON_PRESS) {
			// Cambio a próximo estado
			debounce_state = BUTTON_FALLING;

		} else {
			// Vuelvo a estado anterior
			debounce_state = BUTTON_UP;
		}
		break;

	case BUTTON_FALLING:
		// Se verifica que haya pasado el tiempo de delay para el antirebote y
		// además que el el botón esté pulsado
		if ((delayRead(&debounceDelay) == true)
				& (eboard_switch() == BUTTON_PRESS)) {
			// cambio de estado del LED1
			buttonPressed();
			debounce_state = BUTTON_DOWN;

		} else {
			debounce_state = BUTTON_UP;
		}
		break;

	case BUTTON_DOWN:
		// Cambio de estado
		if (eboard_switch() == BUTTON_RELEASED) {
			debounce_state = BUTTON_RAISING;
		} else {
			// Vuelvo a estado anterior
			debounce_state = BUTTON_DOWN;

		}
		break;

	case BUTTON_RAISING:
		// Se verifica que haya pasado el tiempo de delay para el antirebote y
		// además que el el botón esté soltado
		if ((delayRead(&debounceDelay) == true)
				& (eboard_switch() == BUTTON_RELEASED)) {
			buttonReleased();
			debounce_state = BUTTON_UP;

		} else {
			debounce_state = BUTTON_DOWN;
		}

		break;

	default:
		// si no se reconoce un estado conocido se reinicia la máquina
		debounceFSM_init();
		break;
	}
}

void buttonPressed(void)
{
	pressed_time = get_actual_time_tick();
}
void buttonReleased(void)
{
	released_time = get_actual_time_tick();
	if(	btn_state = LOCKED)
	{
		btn_state = DEFAULT;
	}
}
void get_btn_state(void){

	delta_time = released_time - pressed_time;

	if (delta_time < TIME_DETECT)
	{
		btn_state = DEFAULT;
	}
	else
	{
		if (delta_time < TIME_SHORT && delta_time > TIME_DETECT )
		{
			btn_state = SHORT;
			ELOG("task_button, SHORT STATE");
		}
		else
		{
			if (delta_time < TIME_LONG && delta_time > TIME_SHORT )
			{
				btn_state = LONG;
				ELOG("task_button, LONG STATE");
			}
			else
			{
				btn_state = LOCKED;
				ELOG("task_button, LOCKED STATE");
			}
		}
	}
	ELOG("task_button, STATE %d",btn_state);
}
/********************** external functions definition ************************/

void task_button(void* argument)
{
  led_state_red = false;
  led_state_green = false;

  // se crea un objeto activo por cada LED
  ao_led_red_init(&ao_led_red, led_red_state_callback_);
  ao_led_green_init(&ao_led_green, led_green_state_callback_);

  // Inicialización de la máquina de estados para la detección de la pulsación del botón
  debounceFSM_init();

  while(true)
  {
	    // actualización de FSM
		debounceFSM_update();

		// chequear estado: SHORT, LONG, LOCKED, DEFAULT
		get_btn_state();

		//ELOG("task_button, send red Task");
		ao_led_red_send(&ao_led_red, btn_state);

		//ELOG("task_button, send green Task");
		ao_led_green_send(&ao_led_green, btn_state);
  }
}

/********************** end of file ******************************************/
