#define SYSCTL_RCGCUART_R   (*((volatile unsigned long *)0x400FE618)) // Run Mode Clock Gating Control Register for UART
#define SYSCTL_RCGCGPIO_R   (*((volatile unsigned long *)0x400FE608)) // Run Mode Clock Gating Control Register for GPIO
#define GPIO_PORTA_AFSEL_R  (*((volatile unsigned long *)0x40004420)) // Alternate Function Select Register for GPIO Port A
#define GPIO_PORTA_DEN_R    (*((volatile unsigned long *)0x4000451C)) // Digital Enable Register for GPIO Port A
#define GPIO_PORTA_PCTL_R   (*((volatile unsigned long *)0x4000452C)) // Port Control Register for GPIO Port A
#define UART0_CTL_R         (*((volatile unsigned long *)0x4000C030)) // UART Control Register for UART0
#define UART0_IBRD_R        (*((volatile unsigned long *)0x4000C024)) // Integer Baud Rate Divisor Register for UART0
#define UART0_FBRD_R        (*((volatile unsigned long *)0x4000C028)) // Fractional Baud Rate Divisor Register for UART0
#define UART0_LCRH_R        (*((volatile unsigned long *)0x4000C02C)) // Line Control Register for UART0
#define UART0_FR_R          (*((volatile unsigned long *)0x4000C018)) // UART Flag Register for UART0
#define UART0_DR_R          (*((volatile unsigned long *)0x4000C000)) // UART Data Register for UART0
#define GPIO_PORTC_DIR_R    (*((volatile unsigned long *)0x40006400)) // Direction Register for GPIO Port C
#define GPIO_PORTC_DEN_R    (*((volatile unsigned long *)0x4000651C)) // Digital Enable Register for GPIO Port C
#define GPIO_PORTC_DATA_R   (*((volatile unsigned long *)0x400063FC)) // Data Register for GPIO Port C
#define GPIO_PORTF_DIR_R    (*((volatile unsigned long *)0x40025400)) // Direction Register for GPIO Port F
#define GPIO_PORTF_DEN_R    (*((volatile unsigned long *)0x4002551C)) // Digital Enable Register for GPIO Port F
#define GPIO_PORTF_DATA_R   (*((volatile unsigned long *)0x400253FC)) // Data Register for GPIO Port F
#define GPIO_PORTF_PUR_R    (*((volatile unsigned long *)0x40025510)) // Pull-Up Select Register for GPIO Port F


void UART0_Init(void);
char UART0_Receive(void);
void GPIOF_Init(void);
void GPIOC_Init(void);
void ControlLED(unsigned char state);

unsigned char systemState = 1;

int main(void)
{
    char receivedChar;

    // Initialize UART0 and GPIO Ports F and C
    UART0_Init();
    GPIOF_Init();
    GPIOC_Init();

    while (1)
    {
        // Check if a character has been received from UART
        if ((UART0_FR_R & 0x10) == 0) // UART RX FIFO not empty
        {
            receivedChar = (char)(UART0_DR_R & 0xFF);

            // Update system state based on received character
            if (receivedChar == 'p')
                systemState = 1;
            else if (receivedChar == 'n')
                systemState = 0;
        }

        ControlLED(systemState);
    }
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


void GPIOF_Init(void)
{
    // Enable GPIO Port F
    SYSCTL_RCGCGPIO_R |= 0x20;

    // Configure PF4 as input with pull-down resistor
    GPIO_PORTF_DIR_R &= ~0x10;
    GPIO_PORTF_DEN_R |= 0x10;
    GPIO_PORTF_PUR_R |= 0x10;
}


void GPIOC_Init(void)
{
    // Enable GPIO Port C
    SYSCTL_RCGCGPIO_R |= 0x04;

    // Configure PC4 as an output
    GPIO_PORTC_DIR_R |= 0x10;
    GPIO_PORTC_DEN_R |= 0x10;
}

void ControlLED(unsigned char state)
{
    unsigned char pushButton = GPIO_PORTF_DATA_R & 0x10;

    if (state == 1)
    {
        if (pushButton == 0x10){ // Button pressed
            GPIO_PORTC_DATA_R |= 0x10;  // Turn on external LED
        }
        else{
            GPIO_PORTC_DATA_R &= ~0x10; // Turn off external LED
        }
    }
    else
    {
        if (pushButton == 0x10){ // Button pressed
            GPIO_PORTC_DATA_R &= ~0x10; // Turn off external LED
        }
        else{
            GPIO_PORTC_DATA_R |= 0x10;  // Turn on external LED
        }

    }
}

