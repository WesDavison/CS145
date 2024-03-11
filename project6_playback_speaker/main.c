#include "tm4c123gh6pm.h"
#include <stdint.h>


#define EEPROM_ADDR 0x50

void reset_I2C(void);
void GPIOB_Init(void);
void GPIOC_Init(void);
void GPIOF_Init(void);
void I2C3_Init(void);
void I2C3_WaitUntilReady(void);
void I2C3_Write(uint8_t);
void I2C3_Stop(void);
void I2C3_Read(uint8_t*);
void EEPROM_WriteByte(uint16_t, uint8_t);
uint8_t EEPROM_ReadByte(uint16_t);
void Timer1A_Handler(void);
void playNote(unsigned int);
void recordNote(unsigned int);
void handleKeypadPress(unsigned int);
void changeStateKeypad(void);
void setStateLED(void);

// 0 = play mode (yellow)
// 1 = record mode (red)
// 2 = playback mode (green)


unsigned int state; // current state
unsigned int initialRecordingAddress = 0x00; // address to begin writing at
unsigned int currRecordingAddress; // address currently being written to
unsigned int nextLoadValue; // next frequency to play

char charHold = '\0'; // hold var to check for held buttons

unsigned int note_values[] = {30577, 27241, 24269, 22907, 20407, 18181, 16197, 15288}; // C major scale values


int main(void) {
    GPIOB_Init();  // Keypad Init
    GPIOC_Init();  // LED Init
    GPIOF_Init();  // Timer Init
    setStateLED(); // Set LEDs
    reset_I2C();   // reset I2c
    I2C3_Init();   // Initialize I2C3

    while(1) {
        changeStateKeypad(); // Check keypad for notes or state changes
    }
    return 0;
}

/*
 * Resets all I2C modules to default state
 */
void reset_I2C(void){
    SYSCTL_SRI2C_R |= SYSCTL_SRI2C_R0;
    SYSCTL_SRI2C_R &= ~(SYSCTL_SRI2C_R0);

    SYSCTL_SRI2C_R |= SYSCTL_SRI2C_R1;
    SYSCTL_SRI2C_R &= ~(SYSCTL_SRI2C_R1);

    SYSCTL_SRI2C_R |= SYSCTL_SRI2C_R2;
    SYSCTL_SRI2C_R &= ~(SYSCTL_SRI2C_R2);

    SYSCTL_SRI2C_R |= SYSCTL_SRI2C_R3;
    SYSCTL_SRI2C_R &= ~(SYSCTL_SRI2C_R3);

    SYSCTL_RCGCGPIO_R |= 0x01; // Enable clock to Port A
    SYSCTL_RCGCGPIO_R |= 0x08; // Enable clock to Port D


    GPIO_PORTD_AFSEL_R &= ~0x03; // Clear AFSEL for PD0 and PD1
    GPIO_PORTD_PCTL_R &= ~0xFF; // Clear PCTL for PD0 and PD1

    GPIO_PORTA_AFSEL_R &= ~0xC0; // Clear AFSEL for PA6 and PA7
    GPIO_PORTA_PCTL_R &= ~0xFF000000;  // Clear PCTL for PA6 and PA7

}

/*
 * Keypad pins initialization
 */
void GPIOB_Init(void)
{
    // Enable GPIO Port B
    SYSCTL_RCGCGPIO_R |= 0x02; //clock activated for B

    GPIO_PORTB_DIR_R |= 0x0F; //pins 0-3 output
    GPIO_PORTB_DIR_R &= ~0xF0; // 1110 0000 pins 4-7 input
    GPIO_PORTB_DEN_R |= 0xFF;
    GPIO_PORTB_PUR_R |= 0xF0; // 1110 0000 pins 4-7 pull up.

}

/*
 * LED pins initialization
 */
void GPIOC_Init(void)
{
    // Enable GPIO Port C
    SYSCTL_RCGCGPIO_R |= 0x04;

    // Configure PC4, PC5 and PC6 as an output
    GPIO_PORTC_DIR_R |= 0x70;
    GPIO_PORTC_DEN_R |= 0x70;
}

/*
 * Timer initialization for speaker
 */
void GPIOF_Init(void){
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGC2_GPIOF;   // GPIO Port F
    GPIO_PORTF_DIR_R |= (1<<2);                // Make PF2 an output
    GPIO_PORTF_DEN_R |= (1<<2);                // Enable digital function on PF2

    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R1; // Enable Timer 1
    TIMER1_CTL_R = 0;                          // Disable Timer 1 during setup
    TIMER1_CFG_R = 0x04;                       // Configure for 16-bit mode
    TIMER1_TAMR_R = 0x02;                      // Configure for periodic mode
    TIMER1_TAILR_R = 45000;                    // Dummy value for initial load
    TIMER1_IMR_R = 0x01;                       // Enable Timer 1A timeout interrupt
    NVIC_EN0_R = 1 << (INT_TIMER1A - 16);      // Enable interrupt for Timer 1A in NVIC
}


/*
 * I2C initialization to communicate with EEPROM
 */
void I2C3_Init(void) {
    SYSCTL_RCGCI2C_R |= 0x02;   // Enable the I2C1 module
    SYSCTL_RCGCGPIO_R |= 0x01;  // Enable clock to Port A

    // Configure PD0 and PD1 for I2C
    GPIO_PORTA_AFSEL_R |= 0xC0; // Enable alt func for PA6, PA7
    GPIO_PORTA_ODR_R |= 0x80;   // Enable open drain on PA7
    GPIO_PORTA_PUR_R |= 0xC0;   // Enable pull up resistor on PA6 and PA7
    GPIO_PORTA_DEN_R |= 0xC0;   // Digital enable PA6, PA7
    GPIO_PORTA_PCTL_R &= ~0xFF000000; // Clear PCTL bits for PA6 and PA7
    GPIO_PORTA_PCTL_R |= 0x33000000;  // Configure PA6 and PA7 for I2C1

    // Initialize I2C3 master
    I2C1_MCR_R = 0x10;  // Initialize I2C Master
    I2C1_MTPR_R = 0x09; // Set SCL clock speed to 50Kbps (assuming 16MHz system clock)
}

/*
 * Releases when busy bit driven low
 */
void I2C3_WaitUntilReady(void) {
    while((I2C1_MCS_R & 0x01)){}
}

/*
 * Writes a byte to the bus and sends a continue transaction single
 */
void I2C3_Write(uint8_t data) {
    I2C1_MDR_R = data; // Data to send
    I2C1_MCS_R = 0x01; // Continue the transmission
    I2C3_WaitUntilReady();
}

/*
 * Stops the current transaction
 */
void I2C3_Stop(void) {
    I2C1_MCS_R = 0x05; // Stop condition
    I2C3_WaitUntilReady();
}

/*
 * Reads byte from I2C into the passed in parameter
 */
void I2C3_Read(uint8_t *data) {
    I2C1_MCS_R = (I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_STOP); // Start and then stop after read
    I2C3_WaitUntilReady();
    *data = I2C1_MDR_R; // Read the received data
}

/*
 * Writes a byte of data to the given memory address in EEPROM
 */
void EEPROM_WriteByte(uint16_t memAddress, uint8_t data) {
    I2C1_MSA_R = (EEPROM_ADDR << 1) | 0; // Set slave address and R/W bit to write
    uint16_t temp = memAddress >> 8;
    I2C1_MDR_R = temp; // Data to send
    I2C1_MCS_R = 0x03; // Start the transmission
    I2C3_WaitUntilReady();

    I2C3_Write(memAddress & 0xFF); // LSB of memory address
    I2C3_Write(data); // Send data
    I2C3_Stop(); // Stop condition
}

/*
 * Reads a byte of data from the memory address in EEPROM
 */
uint8_t EEPROM_ReadByte(uint16_t memAddress) {
    uint8_t data;
    I2C1_MSA_R = (EEPROM_ADDR << 1) | 0; // Set slave address and R/W bit to write
    I2C1_MDR_R = memAddress >> 8; // MSB tof memory address
    I2C1_MCS_R = 0x03; // Start the transmission
    I2C3_WaitUntilReady();
    I2C3_Write(memAddress & 0xFF); // LSB of memory address
    I2C1_MSA_R = (EEPROM_ADDR << 1) | 1; // Set slave address and R/W bit to read
    I2C1_MCS_R = 0x0B; // Second Start into Receive
    I2C3_WaitUntilReady();
    data = I2C1_MDR_R;
    I2C1_MCS_R = 0x05; // Receive then stop
    return data;
}

/*
 * Plays current note from nextLoadValue global var
 */
void Timer1A_Handler(void) {
    GPIO_PORTF_DATA_R ^= (1<<2);  // Toggle PF2
    TIMER1_ICR_R = 0x01;          // Clear the Timer 1A timeout interrupt
    TIMER1_TAILR_R = nextLoadValue; // Load the next value to change frequency if needed
}

void setStateLED(){
    switch(state){
        case 0:
            //play mode
            GPIO_PORTC_DATA_R &= 0x20;
            GPIO_PORTC_DATA_R |= 0x20;
            break;
        case 1:
            //record
            GPIO_PORTC_DATA_R &= 0x10;
            GPIO_PORTC_DATA_R |= 0x10;
            break;
        case 2:
            //playback
            GPIO_PORTC_DATA_R &= 0x40;
            GPIO_PORTC_DATA_R |= 0x40;
            break;
    }
}

/*
 * Sets note to be played and triggers Timer1A_Handler to be called for ~1 second
 */
void playNote(unsigned int frequencyLoadValue){
    nextLoadValue = frequencyLoadValue;
    TIMER1_CTL_R |= 0x01;
    int i;
    for(i = 500000; i>0; --i){}
    TIMER1_CTL_R &= ~0x01;

}

/*
 * Writes played note to EEPROM memory and increments address
 */
void recordNote(unsigned int noteValue){
    EEPROM_WriteByte(currRecordingAddress, noteValue);
    currRecordingAddress += 1;
}

/*
 * Reads notes from EEPROM memory and plays them until a null terminator is reached
 */
void playback(void){
    currRecordingAddress = initialRecordingAddress;
    uint8_t note = EEPROM_ReadByte(currRecordingAddress);
    while(note != 0xFF){
        // play note
        playNote(note_values[note]);
        // increment forward
        currRecordingAddress += 1;
        // read next note
        note = EEPROM_ReadByte(currRecordingAddress);
    }
}

/*
 * Checks state to determine if we should play, record, or playback
 */
void handleKeypadPress(unsigned int note){
    switch(state){
        case 0:
            playNote(note_values[note]);
            break;
        case 1:
            recordNote(note);
            playNote(note_values[note]);
            break;
        case 2:
            playback();
            break;
    }
}


/*
 * Checks if a key is being pressed on the keypad
 * If the key that was pressed is the same as the last cycle, no action is performed
 * Otherwise, pass the pressed key (and note) to handleKeypadPress
 */
void changeStateKeypad(void) {
    char charMap[4][4] = {
        {'1', '2', '3', 'A'},
        {'4', '5', '6', 'B'},
        {'7', '8', '9', 'C'},
        {'*', '0', '#', 'D'}
    };

    unsigned char byteMap[4] = {
       0x0E, 0x0D, 0x0B, 0x07
    };

    int row;
    int col;
    // Iterate through rows
    for (row = 0; row < 4; row++) {
        // Set current row low and others high
        GPIO_PORTB_DATA_R = byteMap[row];

        // Iterate through columns
        for (col = 0; col < 4; col++) {
            // Check if button is pressed
            unsigned char cond = GPIO_PORTB_DATA_R & (1 << col + 4);
            if (cond == 0x0){
                char curr_char = charMap[row][col]; // grab pressed char
                if (curr_char == charHold)
                {
                    return; // pressed char is same as last keypad check, return and do no-op
                }
                charHold = curr_char; // set charHold to current char
                switch (curr_char){
                    case 'A': // play state
                        if(state == 1){
                            EEPROM_WriteByte(currRecordingAddress, 0xFF); // check if switching from record mode, if so end recording
                        }
                        state = 0;
                        setStateLED();
                        break;
                    case 'B': // record state
                        currRecordingAddress = initialRecordingAddress; // reset record address to 0x00
                        state = 1;
                        setStateLED();
                        break;
                    case 'C': // playback state
                        if(state == 1){
                            EEPROM_WriteByte(currRecordingAddress, 0xFF);  // check if switching from record mode, if so end recording
                        }
                        state = 2;
                        setStateLED();
                        handleKeypadPress(0); // note param does not matter, playback will be called
                        break;
                    case '1':
                        // C (lo)
                        handleKeypadPress(0);
                        break;
                    case '2':
                        // D
                        handleKeypadPress(1);
                        break;
                    case '3':
                        // E
                        handleKeypadPress(2);
                        break;
                    case '4':
                        // F
                        handleKeypadPress(3);
                        break;
                    case '5':
                        // G
                        handleKeypadPress(4);
                        break;
                    case '6':
                        // A
                        handleKeypadPress(5);
                        break;
                    case '7':
                        //B
                        handleKeypadPress(6);
                        break;
                    case '8':
                        // C (hi)
                        handleKeypadPress(7);
                        break;
                }
                int i;
                for(i = 0; i < 250000; i++){} // debounce
                return;
            }
        }
    }
    charHold = '\0'; //reset character holder
}
