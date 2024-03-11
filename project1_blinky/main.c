/*
 * initialization
 */

#define PORT_F_CLOCK_ADDR (*((volatile unsigned long *) 0x400FE608)) // Address for Clock setting from SysCtl Register

#define PORT_F_DIG_ENABLE_ADDR (*((volatile unsigned long *) 0x4002551C)) // Address for Port F + Offset for Digital Enable

#define PORT_F_DIRECTION_ADDR (*((volatile unsigned long *) 0x40025400)) // Address for Port F + Offset for Digital Output Direction

#define GPIODATA_F (*((volatile unsigned long *) 0x40025038)) // Base address + mask || 00 for pins 1, 2, 3


/*
 * main
 */
int main(void)
{
    volatile unsigned long i;

	PORT_F_CLOCK_ADDR |= 0x20; // 00100000 -> Port F on
	PORT_F_DIG_ENABLE_ADDR |= 0x04; // 00000100 -> Enable digital function of pin 2 (blue LED)
	PORT_F_DIRECTION_ADDR |= 0x04; // 00000100 -> Enable digital output of pin 2 (blue LED)

	while(1){
	    GPIODATA_F = 0x04; // Turn LED on
	    for(i=0; i<1000000;i++); // Blinks ~1 time per second
	    GPIODATA_F = 0x00; // Turn LED off
	    for(i=0; i<1000000;i++); // Blinks ~1 time per second
	}
}
