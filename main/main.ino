/*
 * Author(s): Araam Zaremehrjardi, Thomas Simcox
 * Title: CPE 301 – Team Project
 * Description: A simple swamp cooler program that has four states all 
 * having specific tasks of switching different states and managing 
 * external sensors. 
 * -> This is a interrupt based program. Interrupts are mainly used for 
 * updating sensor values. Sensors using interrupts to update sensor
 * data include Water Level Module, Push Button, and Temperature/Humidity 
 * Sensor.
 * -> Program includes two different flags to print and or update certain
 * devices of the swamp cooler. These flags are used to either update the 
 * LCD (lcd_display_update_flag) and send different data to Serial 
 * (serial_status_flag).
 */ 

#include <DS3231.h>
#include <dht_nonblocking.h>
#include <LiquidCrystal.h>
#include <Wire.h>

/* 
 *  PROGRAM THRESHOLDS
 *  
 *  water_level_threshold: water_level_threshold is used to define the water level 
 *  threshold which determines when the program moves from Idle/Running State
 *  to the Error State.
 *  -> 300 is 1 inch(s) of water. 
 *  temperature_threshold: temperature_threshold is used to define the 
 *  water level threshold which determines the program moves from Idle 
 *  State to Running State.
 *  -> 25 Celsius is the temperature.
 */

#define water_level_threshold 300 
#define temperature_threshold 25

/* 
 *  PROGRAM MACROS
 *  
 *  These macros are mainly used to easily define specific bits
 *  and or values for pins within the program. These serve to 
 *  two purposes.
 *  -> Easily provide a DRY solution to changing only one value
 *  rather than multiple if a device bit is re-wired.
 *  -> Provide a friendly user facing program by abstracting 
 *  values with identifiable names that associate their 
 *  purpose and specific purpose in one name.
 */

#define system_state_idle 1
#define system_state_disabled 0
#define system_state_error 2
#define system_state_running 3
#define motor_bit 0b00000001 
#define led_idle_bit 0b00001000 
#define led_disabled_bit 0b00000100
#define led_error_bit 0b00000001
#define led_running_bit 0b00000010 
#define push_button_bit 0b00001000
#define temperature_humidity_sensor_type DHT_TYPE_11
#define temperature_humidity_sensor_pin 3
#define temperature_humidity_sensor_sampling_tick 5000
#define lcd_display_rs 9
#define lcd_display_en 8
#define lcd_display_d4 13
#define lcd_display_d5 12
#define lcd_display_d6 11
#define lcd_display_d7 10

volatile unsigned char * motor_porte = (unsigned char *) 0x22;
volatile unsigned char * motor_ddre = (unsigned char *) 0x21;
volatile unsigned char * led_portl = (unsigned char *) 0x10B;
volatile unsigned char * led_ddrl = (unsigned char *) 0x10A;
volatile unsigned char * push_button_portd = (unsigned char *) 0x2B;
volatile unsigned char * push_button_ddrd = (unsigned char *) 0x2A;
volatile unsigned char * push_button_pind = (unsigned char *) 0x29;
volatile unsigned char * push_button_eicra = (unsigned char *) 0x69;
volatile unsigned char * push_button_eicrb = (unsigned char *) 0x6A;
volatile unsigned char * push_button_eimsk = (unsigned char *) 0x3D;
volatile unsigned char * water_level_sensor_admux = (unsigned char *) 0x7C;
volatile unsigned char * water_level_sensor_adcsra = (unsigned char *) 0x7A;
volatile unsigned char * water_level_sensor_adcsrb = (unsigned char *) 0x7B;
volatile unsigned int * water_level_sensor_adcl = (unsigned int *) 0x78;
DHT_nonblocking temperature_humidity_sensor (temperature_humidity_sensor_pin, temperature_humidity_sensor_type);
volatile unsigned char * temperature_humidity_sensor_timsk1 = (unsigned char *) 0x6F;
volatile unsigned char * temperature_humidity_sensor_tccr1a = (unsigned char *) 0x80;
volatile unsigned char * temperature_humidity_sensor_tccr1b = (unsigned char *) 0x81;
volatile unsigned int * temperature_humidity_sensor_tcnt1 = (unsigned int *) 0x84;
DS3231 rtc_clock;
LiquidCrystal lcd_display (lcd_display_rs, lcd_display_en, lcd_display_d4, lcd_display_d5, lcd_display_d6, lcd_display_d7);

volatile unsigned char system_state = system_state_idle;
volatile unsigned int water_level = 0;
float temperature = 0;
float humidity = 0;
RTCDateTime date_time_value;
bool lcd_display_update_flag = 1;
unsigned int serial_status_flag = 1;

void setup() {

    *motor_ddre |= motor_bit;
    *motor_porte &= ~(motor_bit);

    *led_ddrl |= led_idle_bit;
    *led_ddrl |= led_disabled_bit;
    *led_ddrl |= led_error_bit;
    *led_ddrl |= led_running_bit;

    *push_button_eicra |= 0b11111111;
    *push_button_eicrb |= 0b11111111;
    *push_button_eimsk |= push_button_bit;
    *push_button_ddrd |= push_button_bit;
    *push_button_portd |= push_button_bit;

    *water_level_sensor_adcsra |= 0b10001000;
    *water_level_sensor_adcsrb &= 0b00000000;
    *water_level_sensor_admux |= 0b01000000;
    *water_level_sensor_adcsra |= 0b01000000; 

    *temperature_humidity_sensor_timsk1 |= 0b00000001;
    *temperature_humidity_sensor_tccr1a &= 0b00000000;
    *temperature_humidity_sensor_tccr1b |= 0b11111001;

    Serial.begin(9600);
    
    rtc_clock.begin();
    //rtc_clock.setDateTime(2020, 11, 29, 12, 56, 00);
    
    lcd_display.begin(16, 2);

}

/*
 * PROGRAM STATES
 * 
 * -> All States: The LCD is continusly refreshing every second with
 * new temperature/humidity values retrieved from DHT11 interrupt  
 * timer with sampling happening 900 Hz.
 * -> Disabled State: No monitoring is occuring with all external
 * devices that aren't outputs or inputs being turned off. Only
 * device being actively interacted with is the LCD Display and
 * Temperature and Humidity Sensor.
 * -> Idle State: Monitoring of all external devices happens
 * during this state with two state switch conditions in tow.
 * Idle State can move to either Error State or Running State 
 * with their own respective conditions. It is made sure that 
 * corresponding LED is turned on only and no vent activity 
 * is occuring.
 * -> Error State: Monitoring of water level is occuring 
 * this state with emphasis of the LCD display responding
 * to this state. LCD update flag is set to zero meaning 
 * no refreshing of the display is to occur ensuring 
 * the Error is presented to the user untill is resolved.
 * Only one state change occurs being from Error State
 * to Idle State once the water level is at or above 
 * the water_level_threshold.
 * -> Running State: Motor is turned on during this state
 * with the only monitoring occuring on the Temperature/Humidity 
 * Sensor and Water Level sensor. Mainly the value temperature and
 * water_level is monitored untill it is above or equal to 
 * temperature_threshold. Two state changes could occur at this
 * state being from Running State to Idle State or Running
 * State to Error State. This occurs with their conditons being met.
 * Vital function that occurs during enterance and exit of this state
 * is toggling of the motor and a serial send of the date of motor
 * being turned on and off respectively.
 * 
 */

void loop() {

    if (lcd_display_update_flag == 1) {

      delay(1000);
      lcd_display.clear();
      lcd_display.print("Temperature:");
      lcd_display.print(temperature);
      lcd_display.setCursor(0,1);
      lcd_display.print("Humidity:");
      lcd_display.print(humidity);
      
    }

    switch (system_state) {

        case system_state_disabled:

            lcd_display_update_flag = 1;
            *led_portl |= led_disabled_bit;
            *led_portl &= ~(led_idle_bit);
            *led_portl &= ~(led_error_bit);
            *led_portl &= ~(led_running_bit);
            *motor_porte &= ~(motor_bit);

        break;

        case system_state_idle: 

            *led_portl &= ~(led_disabled_bit);
            *led_portl |= led_idle_bit;
            *led_portl &= ~(led_error_bit);
            *led_portl &= ~(led_running_bit);
            *motor_porte &= ~(motor_bit);

            if (water_level < water_level_threshold) {

                lcd_display_update_flag = 1;
                system_state = system_state_error;
              
            }
            if (temperature > temperature_threshold) {

                system_state = system_state_running;
              
            }
            
        break;

        case system_state_error:
            
            *led_portl &= ~(led_disabled_bit);
            *led_portl &= ~(led_idle_bit);
            *led_portl |= led_error_bit;
            *led_portl &= ~(led_running_bit);
            *motor_porte &= ~(motor_bit);

            if (lcd_display_update_flag == 1) {

                lcd_display.clear();
                lcd_display.print("Water is low.");
                lcd_display_update_flag = 0;
              
            }

            if (water_level >= water_level_threshold) {

              lcd_display_update_flag = 1;
              system_state = system_state_idle;
              
            }

        break;

        case system_state_running:

            *led_portl &= ~(led_disabled_bit);
            *led_portl &= ~(led_idle_bit);
            *led_portl &= ~(led_error_bit);
            *led_portl |= led_running_bit;
            *motor_porte |= motor_bit;

            send_timestamp();
            serial_status_flag = 0;
            
            if (water_level < water_level_threshold) {

                lcd_display_update_flag = 1;
                serial_status_flag = 2;
                send_timestamp();
                serial_status_flag = 1;
                system_state = system_state_error;
              
            }
            if (temperature <= temperature_threshold) {

                *motor_porte &= ~(motor_bit);
                serial_status_flag = 2;
                send_timestamp();
                serial_status_flag = 1;
                system_state = system_state_idle;
              
            }

        break;
      
    }
  
}

/*
 * PROGRAM INTERRUPTS
 * 
 * -> INT3_vect: This interrupt is made to start a interrupt in the event
 * that the Push Button is pressed. During a external interrupt request 
 * from the Push Button, debouncing occures to make sure such button 
 * wasn't a false activation. Once checked, state switching occurs
 * being any State can move from it's Current State to Disabled State
 * by setting the system_state value.
 * -> ADC_vect: This interrupt is made to start a interrupt to the Stack
 * once ADC has completed conversion of reading from the Water Level Sensor.
 * Once completed, this value is then made to be set for water_level.
 * -> TIMER1_OVF_vect: This interrupt is made to continuously update the
 * temperature and humidity values every amount of CPU ticks set by the
 * temperature_humidity_sensor_sampling_tick macro.
 */

ISR (INT3_vect) {

    if (*push_button_pind & push_button_bit) {
      
        for(volatile unsigned int index = 0; index < 500; index++);
        
        if (*push_button_pind & push_button_bit) {
         
            if (system_state == system_state_disabled) {

                system_state = system_state_idle;
              
            } else {

                system_state = system_state_disabled;
              
            }

        }
        
    }

}

ISR (ADC_vect) {

    water_level = *water_level_sensor_adcl;
    *water_level_sensor_adcsra |= 0b01000000;
  
}

ISR (TIMER1_OVF_vect) {

    *temperature_humidity_sensor_tccr1b &= 0b11111000;
    *temperature_humidity_sensor_tcnt1 = (unsigned int) (65536 - temperature_humidity_sensor_sampling_tick);
    temperature_humidity_sensor.measure(&temperature, &humidity);
    *temperature_humidity_sensor_tccr1b |= 0b11111001;
  
}

void send_timestamp() {

    date_time_value = rtc_clock.getDateTime();
    if (serial_status_flag == 1) {

      Serial.print("FAN IS ON AT ");
      Serial.print(date_time_value.year);   
      Serial.print("-");
      Serial.print(date_time_value.month);  
      Serial.print("-");
      Serial.print(date_time_value.day);    
      Serial.print(" ");
      Serial.print(date_time_value.hour);   
      Serial.print(":");
      Serial.print(date_time_value.minute); 
      Serial.print(":");
      Serial.print(date_time_value.second); 
      Serial.println("");
      
      
    }
    else if (serial_status_flag == 2) {

      Serial.print("FAN IS OFF AT ");
      Serial.print(date_time_value.year);   
      Serial.print("-");
      Serial.print(date_time_value.month);  
      Serial.print("-");
      Serial.print(date_time_value.day);    
      Serial.print(" ");
      Serial.print(date_time_value.hour);   
      Serial.print(":");
      Serial.print(date_time_value.minute); 
      Serial.print(":");
      Serial.print(date_time_value.second); 
      Serial.println("");
      
    }
    
  
}
