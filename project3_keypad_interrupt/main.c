#include "tm4c123gh6pm.h"

void GPIOA_Init(void);
void GPIOC_Init(void);
void GPIOD_Init(void);
void GPIOE_Init(void);
void Timer0A_Init(void);
void Timer0A_Handler(void);
void buttonPressed(void);
void UART0_Init(void);
void printFromKeypad();



/**
 * main.c
 */
int main(void)
  {
    GPIOA_Init();
    GPIOC_Init();
    GPIOD_Init();
    GPIOE_Init();
    Timer0A_Init();
    UART0_Init();

    while(1) {
        printFromKeypad();
    }
    return 0;
}

void Timer0A_Init(void) {
    SYSCTL_RCGCTIMER_R |= 0x01; // Enable clock to Timer Block 0
    TIMER0_CTL_R &= ~0x01; // Disable Timer A before initialization
    TIMER0_CFG_R = 0x04; // Configure for 32-bit timer mode
    TIMER0_TAMR_R = 0x02; // Configure for periodic mode and down-counter
    TIMER0_TAILR_R = 65307; // Set load value for 1 second interval (16MHz clock)
    TIMER0_TAPR_R = 0xF5; // 246 Prescalar
    TIMER0_ICR_R = 0x01; // Clear the TimerA timeout flag
    TIMER0_IMR_R |= 0x01; // Enable interrupt on Timer A timeout
    NVIC_EN0_R |= 1 << (19); // Enable IRQ 19 (Timer 0A) in NVIC
    TIMER0_CTL_R |= 0x01; // Enable Timer A after initialization
}

void GPIOA_Init(void){
    // Enable GPIO Port A
    SYSCTL_RCGCGPIO_R |= 0x01;

    // Configure PA as input with pull-down resistor
    GPIO_PORTA_DIR_R |= 0x01;
    GPIO_PORTA_DEN_R |= 0xFF;
    GPIO_PORTA_PUR_R |= 0xFF;
}

void GPIOC_Init(void)
{
    // Enable GPIO Port C
    SYSCTL_RCGCGPIO_R |= 0x04;

    // Configure PC4 as an output
    GPIO_PORTC_DIR_R |= 0x10;
    GPIO_PORTC_DEN_R |= 0x10;
}

void GPIOD_Init(void)
{
    // Enable GPIO Port D
    SYSCTL_RCGCGPIO_R |= 0x08;


    // Configure PD0-3 as output
    GPIO_PORTD_DIR_R |= 0x0F;
    GPIO_PORTD_DEN_R |= 0x0F;
    GPIO_PORTD_DATA_R |= 0x0F;
}

void GPIOE_Init(void){
    // Enable GPIO Port E
    SYSCTL_RCGCGPIO_R |= 0x10;

    //Interrupt handling
    GPIO_PORTE_IM_R |= (1 << 5); //bitwise and interrupt mask for pin E5, which is connected to button
    GPIO_PORTE_IS_R &= ~0x20; //set to edge-sensitive
    GPIO_PORTE_IEV_R |= 0x20; //set to rising edge
    GPIO_PORTE_ICR_R = 0x20; // Clear any prior interrupts on PE5
    NVIC_EN0_R |= 1 << (4); // Enable IRQ 4 (Port E) in NVIC

    // Configure PE as input with pull-up resistor
    GPIO_PORTE_DIR_R &= ~0xFF;
    GPIO_PORTE_DEN_R |= 0xFF;
    GPIO_PORTE_PUR_R |= 0x1E;
}



void UART0_Init(void)
{
    // Enable UART0 and GPIOA in RCGC
    SYSCTL_RCGCUART_R |= 0x01;
    SYSCTL_RCGCGPIO_R |= 0x01;

    // Enable PA0 and PA1 for UART0
    GPIO_PORTA_AFSEL_R |= 0x03;
    GPIO_PORTA_DEN_R |= 0x03;
    GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R & 0xFFFFFF00) | 0x00000011;

    // Configure UART0
    UART0_CTL_R &= ~0x01; // Disable UART
    UART0_IBRD_R = 104;   // 16 MHz bus for 9600 baud rate
    UART0_FBRD_R = 11;    // Fraction part
    UART0_LCRH_R = 0x70;  // 8-bit, no parity, 1-stop bit, FIFO
    UART0_CTL_R |= 0x301; // Enable UART
}


void UART_SendString(const char* str) {
    while (*str != '\0') {
        while ((UART0_FR_R & (1 << 5)) != 0);
        UART0_DR_R = *str;
        str++;
    }
}

void sendCharAsConstCharPtr(char c) {
    char str[2];
    str[0] = c;
    str[1] = '\0';

    const char* ptr = str;

    UART_SendString(ptr);
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
        GPIO_PORTD_DATA_R = byteMap[row];

        // Iterate through columns
        for (col = 1; col < 5; col++) {
            // Check if button is pressed
            unsigned char cond = GPIO_PORTE_DATA_R & (1 << col);
            if (cond == 0x0){
                // button was pressed, send char
                sendCharAsConstCharPtr(charMap[row][col - 1]);
                // delay to prevent multiple outputs for one button press
                int i;
                for(i = 0; i < 500000; i++){}
            }
        }
    }
}

void Timer0A_Handler(void) {
    // Acknowledge the timer interrupt
    TIMER0_ICR_R = TIMER_ICR_TATOCINT; // This clears the timer interrupt

    // Toggle the LED
    if (GPIO_PORTC_DATA_R & 0x10) { // If LED is on
        GPIO_PORTC_DATA_R &= ~0x10; // Turn it off
    } else {
        GPIO_PORTC_DATA_R |= 0x10; // Turn it on
    }
}

void buttonPressed(void)
{
    GPIO_PORTE_ICR_R = 0x20; // resets interrupt
    UART_SendString("$");

}
