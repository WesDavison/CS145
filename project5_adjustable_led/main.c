#include "tm4c123gh6pm.h"

void GPIOB_Init(void);
void GPIOE_Init(void);
void ADC_Init(void);
void ADC_Conversion(void);
void Adjust_PWM_Output(void);
void PWM_Init(void);

unsigned int analog_to_digital;

void delay(int d){while(--d);}


int main(void)
{
    GPIOB_Init();
    GPIOE_Init();
    ADC_Init();
    PWM_Init();
    while(1)
    {
        ADC_Conversion();


    }
    return 0;
}

void GPIOB_Init(void) //PB6 is M0PWM0
{
    SYSCTL_RCGCGPIO_R |= 0x02; // enable Clock for Port B.
    GPIO_PORTB_DIR_R |= 0x40; // 0100 0000 PB6 as output
    GPIO_PORTB_AFSEL_R |= 0x40; // alternate function select for PB6.
    GPIO_PORTB_PCTL_R &=  ~(0xF000000); // select digital function 4 (M0PWM0) for PB6.
    GPIO_PORTB_PCTL_R |= 0x4000000; // select digital function 4 (M0PWM0) for PB6.
    GPIO_PORTB_DEN_R |= 0x40; //0100 0000 PB6 digital enable.
}

void GPIOE_Init(void)
{
    SYSCTL_RCGCGPIO_R |= 0x10; // enable Clock for Port E.
    GPIO_PORTE_DIR_R &= ~(0x08); // 0000 1000 PE3 set as input.
    GPIO_PORTE_AFSEL_R |= 0x08; // enable alternate function select
    GPIO_PORTE_DEN_R &= ~(0x08); //0000 0100 PE3 disable digital.
    GPIO_PORTE_AMSEL_R |= (0x08); //enable analog select.
}

void ADC_Init(void)
{
    SYSCTL_RCGCADC_R |= 0x01; //enable ADC clock R0.
    delay(500000); // leave here pls.
    ADC0_ACTSS_R &= ~(0x08);  // disable SS3 on ADC.
    ADC0_SSMUX3_R = 0x0; //using MUX3, select (SS3 ; Ain0 ; PE3)
    ADC0_EMUX_R = 0x0000; // trigger source for SS3 is processor.
    ADC0_SSCTL3_R = 0x6; // sample sequence control for SS3.
    ADC0_ACTSS_R = 0x08;  // enable SS3 on ADC.
}

void PWM_Init(void) //using 0 PWM 0 (PB6)
{
    SYSCTL_RCGCPWM_R |= 0x1; //Activate PWM clock to module 0.
    while ((SYSCTL_PRPWM_R & 0x01) == 0) {};
    SYSCTL_RCC_R |= SYSCTL_RCC_USEPWMDIV; //set clck divider on
    SYSCTL_RCC_R = (SYSCTL_RCC_R & ~SYSCTL_RCC_PWMDIV_M) | SYSCTL_RCC_PWMDIV_2; // config divider to /2.
    PWM0_0_CTL_R = 0; //disable to set config and turn back on.
    PWM0_0_GENA_R = 0x8c; // 00 00 10 00 11 00 toggle on A_down.
    PWM0_0_LOAD_R = 0x00001000; //Some initial Load for PWM0
    PWM0_0_CMPA_R = 0x0000012B; //Some initial value for Comp A, will be replaced by ADC value
    PWM0_0_CTL_R |= 0x1;
    PWM0_CTL_R &= 0x0; // PWM module 0 set to count down mode.
    PWM0_ENABLE_R |= 0x01; // Enable PWM0 output
}

void ADC_Conversion(void)
{
    ADC0_PSSI_R |= 0x8; // initiate ADC conversion trigger for SS3.

    char complete = ADC0_RIS_R & 0x8;
    while (complete == 0){complete = ADC0_RIS_R & 0x8;} // reading SS3 completion flag.


    analog_to_digital = ADC0_SSFIFO3_R & (0xFFF); //read digital result from SSFIFO3 low 12 bits.
    ADC0_ISC_R |= 0x8; //send 1 to IN3 to clear SS3 flag.
    PWM0_0_CMPA_R = analog_to_digital; //set new power value for PWM (CompA).

}

