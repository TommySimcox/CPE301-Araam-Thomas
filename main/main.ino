#define M_SYSTEM_STATE_IDLE 1
#define M_SYSTEM_STATE_DISABLED 0
#define M_SYSTEM_STATE_ERROR 2
#define M_SYSTEM_STATE_RUNNING 3
#define M_WATER_LEVEL_THRESHOLD 10

unsigned char g_system_state = M_SYSTEM_STATE_DISABLED;
unsigned int water_level = 0;

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

volatile unsigned char * led_porta_data = (unsigned char *) 0x22; 
volatile unsigned char * led_porta_ddr = (unsigned char *) 0x21;

volatile unsigned char * push_button_portd_data = (unsigned char *) 0x2B;
volatile unsigned char * push_button_portd_ddr = (unsigned char *) 0x2A;
volatile unsigned char * push_button_portd_pin = (unsigned char *) 0x29;
volatile unsigned char * push_button_eicra = (unsigned char *) 0x69;
volatile unsigned char * push_button_eicrb = (unsigned char *) 0x6A;
volatile unsigned char * push_button_eimsk = (unsigned char *) 0x3D;

volatile unsigned char * water_level_sensor_admux = (unsigned char *) 0x7C;
volatile unsigned char * water_level_sensor_adcsra = (unsigned char *) 0x7A;
volatile unsigned char * water_level_sensor_adcsrb = (unsigned char *) 0x7B;
volatile unsigned int * water_level_sensor_adcl = (unsigned int *) 0x78;


void setup() {

    *led_porta_ddr |= 0b00001111;
    
    *push_button_eicra |= 0b00000001;
    *push_button_eicrb |= 0b00000001;
    *push_button_eimsk |= 0b00000001;
    *push_button_portd_ddr |= 0b00000000;
    *push_button_portd_data |= 0b00000001;

    *water_level_sensor_adcsra |= 0b10001000;
    *water_level_sensor_adcsrb &= 0b00000000;
    *water_level_sensor_admux |= 0b01000000;

    *water_level_sensor_adcsra |= 0b01000000; 

}

void loop() {

    switch (g_system_state) {

        case M_SYSTEM_STATE_IDLE:

            *led_porta_data &= 0b00000001;
            *led_porta_data |= 0b00000001;
            if (water_level < M_WATER_LEVEL_THRESHOLD) {
              
                g_system_state = M_SYSTEM_STATE_ERROR;
              
            }

            break;

        case M_SYSTEM_STATE_DISABLED: 

            *led_porta_data &= 0b00000010;
            *led_porta_data |= 0b00000010;

            break;

        case M_SYSTEM_STATE_ERROR:

            *led_porta_data &= 0b00000100;
            *led_porta_data |= 0b00000100;

            if (water_level >= M_WATER_LEVEL_THRESHOLD) {

              g_system_state = M_SYSTEM_STATE_IDLE;
              
            }

            break;

        case M_SYSTEM_STATE_RUNNING:

            *led_porta_data &= 0b00001000;
            *led_porta_data |= 0b00001000;

            break;

    }


}

ISR(INT0_vect) {

    if (*push_button_portd_pin & 0b00000001) {
      
        for (volatile unsigned int index = 0; index < 200; index++);
        
        if (*push_button_portd_pin & 0b00000001) {
          
            g_system_state = g_system_state ? M_SYSTEM_STATE_DISABLED : M_SYSTEM_STATE_IDLE;
            while (*push_button_portd_pin & 0b00000000);
            
        }
        
    }
    
}

ISR(ADC_vect) {

  water_level = *water_level_sensor_adcl;
  *water_level_sensor_adcsra |= 0b01000000; 
  
}
