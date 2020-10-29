#define M_SYSTEM_STATE_IDLE 0
#define M_SYSTEM_STATE_DISABLED 1
#define M_SYSTEM_STATE_ERROR 2
#define M_SYSTEM_STATE_RUNNING 3

unsigned char g_system_state = M_SYSTEM_STATE_DISABLED;

/*
*
*   [71-78] -> PORTA : "o_system_state_LED" and "o_system_information_LCD" PORTA DESCRIPTION
*
*   -------------------------------------------------------------------------
*   | PORTA7 | PORTA6 | PORTA5 | PORTA4 | PORTA3 | PORTA2 | PORTA1 | PORTA0 |
*   -------------------------------------------------------------------------
*
*   [71] -> PORTA7 :                             -> DDRA7 : 
*   [72] -> PORTA6 : ENABLE LCD                  -> DDRA6 : OUTPUT
*   [73] -> PORTA5 : R/W LCD                     -> DDRA5 : OUTPUT
*   [74] -> PORTA4 : REGISTER SELECT LCD         -> DDRA4 : OUTPUT
*   [75] -> PORTA3 : RUNNING STATE LED           -> DDRA3 : OUTPUT 
*   [76] -> PORTA2 : ERROR STATE LED             -> DDRA2 : OUTPUT 
*   [77] -> PORTA1 : DISABLED STATE LED          -> DDRA1 : OUTPUT 
*   [78] -> PORTA0 : IDLE STATE LED              -> DDRA0 : OUTPUT 
*
*/

unsigned char * o_system_state_LED_port = (unsigned char *) 0x22; 
unsigned char * o_system_state_LED_ddr = (unsigned char *) 0x21;

/*
*
*   [30-37] -> PORTC : "o_system_information_LCD" PORTB DESCRIPTION 
*
*   -------------------------------------------------------------------------
*   | PORTC7 | PORTC6 | PORTC5 | PORTC4 | PORTC3 | PORTC2 | PORTC1 | PORTC0 |
*   -------------------------------------------------------------------------
*
*   [30] -> PORTC7 : DB7                         -> DDRC7 : OUTPUT
*   [31] -> PORTC6 : DB6                         -> DDRC6 : OUTPUT
*   [32] -> PORTC5 : DB5                         -> DDRC5 : OUTPUT
*   [33] -> PORTC4 : DB4                         -> DDRC4 : OUTPUT
*   [34] -> PORTC3 : DB3                         -> DDRC3 : OUTPUT 
*   [35] -> PORTC2 : DB2                         -> DDRC2 : OUTPUT 
*   [36] -> PORTC1 : DB1                         -> DDRC1 : OUTPUT 
*   [37] -> PORTC0 : DB0                         -> DDRC0 : OUTPUT 
*
*/

unsigned char * o_system_information_LCD_port = (unsigned char *) 0x28;
unsigned char * o_system_information_LCD_ddr = (unsigned char *) 0x27;


void setup() {

    

}

void loop() {

}