#include "tm4c123gh6pm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// ***** Pin Assignments *****
/*
 * Keypad :
 *  * B0-B3 = Rows
 *  * B4-B7 = Columns
 *
 *  LCD:
 *  * C4 = RS
 *  * C5 = RW
 *  * C6 = E
//    D0 = PE4
//    D1 = PE5
//    D2 = PD2
//    D3 = PD3
//    D4 = PE0
//    D5 = PE1
//    D6 = PE2
//    D7 = PE3
 */


// LCD Data Pins
#define GPIO_PORTB_DATA_0 (*((volatile unsigned long *) (0x40005004)))
#define GPIO_PORTB_DATA_1 (*((volatile unsigned long *) (0x40005008)))
#define GPIO_PORTB_DATA_2 (*((volatile unsigned long *) (0x40005010)))
#define GPIO_PORTB_DATA_3 (*((volatile unsigned long *) (0x40005020)))
#define GPIO_PORTB_DATA_4 (*((volatile unsigned long *) (0x40005040)))
#define GPIO_PORTB_DATA_5 (*((volatile unsigned long *) (0x40005080)))
#define GPIO_PORTB_DATA_6 (*((volatile unsigned long *) (0x40005100)))
#define GPIO_PORTB_DATA_7 (*((volatile unsigned long *) (0x40005200)))
#define GPIO_PORTD_DATA (*((volatile unsigned long *) (0x40007030))) // 00 0011 0000 D2-D3
#define GPIO_PORTE_DATA (*((volatile unsigned long *) (0x400240FC))) // 00 1111 1100 E0-E5

char A[9];
char B[9];
int A_ptr;
int B_ptr;
int state; // 0 - initial, 1 - start, 2 - A, 3 - B, 4 - display

void init_state(void);
void start_state(void);
void A_state(void);
void B_state(void);
void display_state(void);
void printFromKeypad(void);
void GPIOB_Init(void);
void GPIOD_Init(void);
void GPIOE_Init(void);
void GPIOC_Init(void);
void delay(int);
void LCD_Init(void);
void WriteToDR(unsigned char);
void WriteToIR(unsigned char);
void clearLCD(void);
void lcd_product(int, int);
void writeToDataBus(unsigned char);
void clearLCDTopRow(void);

/**
 * main.c
 */
int main(void)
{
    delay(1000); //allow 1 second for the LCD to power on
    GPIOB_Init();
    GPIOD_Init();
    GPIOE_Init();
    GPIOC_Init();
    LCD_Init();
    init_state();
    while(1){
        printFromKeypad();
    }
    return 0;
}

void GPIOB_Init(void)
{
    // Enable GPIO Port B
    SYSCTL_RCGCGPIO_R |= 0x02; //clock activated for B

    GPIO_PORTB_DIR_R |= 0x0F; //pins 0-3 output
    GPIO_PORTB_DIR_R &= ~0xF0; // 1110 0000 pins 4-7 input
    GPIO_PORTB_DEN_R |= 0xFF;
    GPIO_PORTB_PUR_R |= 0xF0; // 1110 0000 pins 4-7 pull up.

}


void GPIOD_Init(void)
{
    // Enable GPIO Port D
    SYSCTL_RCGCGPIO_R |= 0x08;


    // Configure PD2-3 as output
    GPIO_PORTD_DIR_R |= 0x0C; //0000 1100
    GPIO_PORTD_DEN_R |= 0x0C;
    //GPIO_PORTD_DATA_R |= 0x0F;
}

void GPIOE_Init(void){
    // Enable GPIO Port E
    SYSCTL_RCGCGPIO_R |= 0x10;

    // Configure PE as output
    GPIO_PORTE_DIR_R |= 0x3F; // 011 1111
    GPIO_PORTE_DEN_R |= 0x3F;
    //GPIO_PORTE_PUR_R |= 0x1E;
}

void GPIOC_Init(void)
{
    // Enable GPIO Port C
    SYSCTL_RCGCGPIO_R |= 0x04; // 0000 0100


    // Configure PC4-6 as output
    GPIO_PORTC_DIR_R |= 0x70; //pins 4,5,6 0111 0000
    GPIO_PORTC_DEN_R |= 0x70; //digital enable. 0111 000
}

void LCD_Init(void)
{
    delay(1000);
    clearLCD();
}

// Function to provide a small delay
void delay(int d){
    while(--d);
}

void WriteToIR(unsigned char command)
{
    GPIO_PORTC_DATA_R = 0x00; // setting RS & R/W low to write to IR
    delay(1000);
    GPIO_PORTC_DATA_R = 0x40; // set E high
    delay(1000);
    writeToDataBus(command); //send data through
    delay(1000);
    GPIO_PORTC_DATA_R = 0x00; // E goes low
    delay(1000);
    writeToDataBus(0x00);

    delay(1000);
}

void WriteToDR(unsigned char character)
{
    GPIO_PORTC_DATA_R = 0x10; // setting RS high & R/W low to write to DR
    delay(1000);
    GPIO_PORTC_DATA_R = 0x50; // set E high
    delay(1000);
    writeToDataBus(character); //send data through
    delay(1000);
    GPIO_PORTC_DATA_R = 0x10; // E goes low
    delay(1000);
    writeToDataBus(0x00);

    delay(1000);
}


void init_state(void){
    state = 0;
    // clear LCD display
    clearLCD();
    delay(2000);
    // go to next state
    start_state();
}

void start_state(void){
    state = 1;
    memset(A, '\0', sizeof(A));
    memset(B, '\0', sizeof(B));
    A_ptr = 0;
    B_ptr = 0;
    // reset cursor
    WriteToIR(0x02);
    // move to a state
    A_state();
}

void A_state(void){
    state = 2;

}

void B_state(void){
    state = 3;
    clearLCDTopRow();

}

void display_state(void){
    clearLCD();
    state = 4;
    int A_num = atoi(A);
    int B_num = atoi(B);
    int product = A_num * B_num;
    WriteToIR(0xC0); // switch to bottom row
    lcd_product(product, 0); // print output
    start_state(); // return to start state


}

void lcd_product(int integer, int count){
    // convert each digit position to char and print to lcd
    if (integer == 0 || count == 8)
        return;
    char c;
    int digit = integer % 10;
    lcd_product(integer / 10, ++count);
    c = '0' + digit;
    WriteToDR(c);
}

void clearLCD(){
    WriteToIR(0x01); // clearLCD screen
    delay(1000);
    WriteToIR(0x38); // 8bit mode utilizing 16 columns 2 rows
    delay(1000);
    WriteToIR(0x06); // auto incrementing the cursor when prints the data in current
    delay(1000);
    WriteToIR(0x0F); // cursor off and display on 0000 1111
    delay(1000);
}

void writeToDataBus(unsigned char data) {

    // LCD bit DB0 = MCU pin PE4
    if ((data&0x01) == 0x01) {
        GPIO_PORTE_DATA |= (1<<4);
    } else {
        GPIO_PORTE_DATA &= (~(1<<4));
    }
    // LCD bit DB1 = MCU pin PE5
    if ((data&0x02) == 0x02) {
        GPIO_PORTE_DATA |= (1<<5);
    } else {
        GPIO_PORTE_DATA &= (~(1<<5));
    }
    // LCD bit DB2 = MCU pin PD2
    if ((data&0x04) == 0x04) {
        GPIO_PORTD_DATA |= (1<<2);
    } else {
        GPIO_PORTD_DATA &= (~(1<<2));
    }
    // LCD bit DB3 = MCU pin PD3
    if ((data&0x08) == 0x08) {
        GPIO_PORTD_DATA |= (1<<3);
    } else {
        GPIO_PORTD_DATA &= (~(1<<3));
    }
    // LCD bit DB4 = MCU pin PE0
    if ((data&0x10) == 0x10) {
        GPIO_PORTE_DATA |= (1<<0);
    } else {
        GPIO_PORTE_DATA &= (~(1<<0));
    }
    // LCD bit DB5 = MCU pin PE1
    if ((data&0x20) == 0x20) {
        GPIO_PORTE_DATA |= (1<<1);
    } else {
        GPIO_PORTE_DATA &= (~(1<<1));
    }
    // LCD bit DB6 = MCU pin PE2
    if ((data&0x40) == 0x40) {
        GPIO_PORTE_DATA |= (1<<2);
    } else {
        GPIO_PORTE_DATA &= (~(1<<2));
    }
    // LCD bit DB7 = MCU pin PE3
    if ((data&0x80) == 0x80) {
        GPIO_PORTE_DATA |= (1<<3);
    } else {
        GPIO_PORTE_DATA &= (~(1<<3));
    }
}

void clearLCDTopRow(){
    //Set cursor to home
    WriteToIR(0x02);
    int j;
    for(j = 0; j < 8; j++){
        WriteToDR(0x20); //whitespace = 0x20
    }
    WriteToIR(0x02);
}


void printFromKeypad() {
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
                char curr_char = charMap[row][col];
                if (curr_char == 'A' || curr_char == 'B' || curr_char == 'D'){
                    continue;
                }
                if (curr_char == 'C'){
                    init_state();
                }
                else if (curr_char == '#'){
                    display_state();
                }
                else if (curr_char == '*'){
                    B_state();
                }
                else {
                    switch(state){
                        case 2:
                            // A
                            A[A_ptr] = curr_char;
                            WriteToDR(curr_char);
                            A_ptr += 1;
                            if (A_ptr == 8) {
                                B_state();
                            }
                            break;
                        case 3:
                            // B
                            B[B_ptr] = curr_char;
                            WriteToDR(curr_char);
                            B_ptr += 1;
                            if (B_ptr == 8){
                                display_state();
                            }
                    }
                }
                int i;
                for(i = 0; i < 500000; i++){}
            }
        }
    }
}

