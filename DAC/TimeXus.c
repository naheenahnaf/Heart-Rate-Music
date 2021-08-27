
#include "configuration.h"

/*----------------------------------------------------------------------------
 
 void TimeXus(u16 u16Time)
 Sets Timer0 to count u16Microseconds_
 
 Requires:
 -Timer0 configured such that each timer tick is 1 microsecond
 -u16Time is the value in microseconds to time from 1 to 65535
 
 Promises:
 -Pre-loads TMROH:L to clock out desired period
 -TMR0IF cleared
 -Timer0 enabled
 */


void TimeXus(u16 u16Time)
{
    /* Disable the timer during configuration */
    T0CON0 &= 0x70;
    /* Preload TMR0H and TMR0L based on u16TimeXus */
    u16 u16InitialValue = 0xFFFF - u16Time + 0x0001;
    /* Subtract time from max count and add 1 due to roll over */
    TMR0H =  u16InitialValue >> 8;          
    TMR0L =  u16InitialValue & 0x00FF;
    /* Clear TMR0IF and enable Timer0 */
    TMR0IF = 0;
    T0CON0 |= 0xF0;

} /* end TimeXus() */ 
