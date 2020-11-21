#include <Arduino.h>
#define M_SYSTEM_STATE_IDLE 1
#define M_SYSTEM_STATE_DISABLED 0
#define M_SYSTEM_STATE_ERROR 2
#define M_SYSTEM_STATE_RUNNING 3

unsigned char g_system_state = M_SYSTEM_STATE_DISABLED;

/*
*
*   [22-29] -> PORTA : "o_system_state_LED" PORTA DESCRIPTION
*
*   -------------------------------------------------------------------------
*   | PORTA7 | PORTA6 | PORTA5 | PORTA4 | PORTA3 | PORTA2 | PORTA1 | PORTA0 |
*   -------------------------------------------------------------------------
*
*   [29] -> PORTA7 :                             -> DDRA7 : 
*   [28] -> PORTA6 :                             -> DDRA6 : 
*   [27] -> PORTA5 :                             -> DDRA5 : 
*   [26] -> PORTA4 :                             -> DDRA4 : 
*   [25] -> PORTA3 : RUNNING STATE LED           -> DDRA3 : OUTPUT 
*   [24] -> PORTA2 : ERROR STATE LED             -> DDRA2 : OUTPUT 
*   [23] -> PORTA1 : DISABLED STATE LED          -> DDRA1 : OUTPUT 
*   [22] -> PORTA0 : IDLE STATE LED              -> DDRA0 : OUTPUT 
*
*/

volatile unsigned char * o_system_state_LED_port = (unsigned char *) 0x22; 
volatile unsigned char * o_system_state_LED_ddr = (unsigned char *) 0x21;

volatile unsigned char * portd_data = (unsigned char *) 0x2B;
volatile unsigned char * portd_ddr = (unsigned char *) 0x2A;
volatile unsigned char * portd_pin = (unsigned char *) 0x29;
volatile unsigned char * push_button_eicra = (unsigned char *) 0x69;
volatile unsigned char * push_button_eicrb = (unsigned char *) 0x6A;
volatile unsigned char * push_button_eimsk = (unsigned char *) 0x3D;


void setup() {

    *o_system_state_LED_ddr |= 0b00001111;
    
    *push_button_eicra |= 0b00000001;
    *push_button_eicrb |= 0b00000001;
    *push_button_eimsk |= 0b00000001;
    *portd_ddr |= 0b00000000;
    *portd_data |= 0b00000001;

}

void loop() {

    switch (g_system_state) {

        case M_SYSTEM_STATE_IDLE:

            *o_system_state_LED_port &= 0b00000001;
            *o_system_state_LED_port |= 0b00000001;

            break;

        case M_SYSTEM_STATE_DISABLED: 

            *o_system_state_LED_port &= 0b00000010;
            *o_system_state_LED_port |= 0b00000010;

            break;

        case M_SYSTEM_STATE_ERROR:

            break;

        case M_SYSTEM_STATE_RUNNING:

            break;

    }


}

ISR(INT0_vect) {

    if (*portd_pin & 0b00000001) {
      
        for (volatile unsigned int index = 0; index < 200; index++);
        
        if (*portd_pin & 0b00000001) {
          
            g_system_state = g_system_state ? M_SYSTEM_STATE_DISABLED : M_SYSTEM_STATE_IDLE;
            while (*portd_pin & 0b00000000);
            
        }
        
    }
    
}
