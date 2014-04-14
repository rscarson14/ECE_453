/* --COPYRIGHT--,BSD_EX
 * Copyright (c) 2012, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *******************************************************************************
 * 
 *                       MSP430 CODE EXAMPLE DISCLAIMER
 *
 * MSP430 code examples are self-contained low-level programs that typically
 * demonstrate a single peripheral function or device feature in a highly
 * concise manner. For this the code may rely on the device's power-on default
 * register values and settings such as the clock configuration and care must
 * be taken when combining code from several examples to avoid potential side
 * effects. Also see www.ti.com/grace for a GUI- and www.ti.com/msp430ware
 * for an API functional library-approach to peripheral configuration.
 *
 * --/COPYRIGHT--*/
//******************************************************************************
//  CC430F513x Demo - ADC12_A, Repeated Sequence of Conversions
//
//  Description: This example shows how to perform a repeated sequence of
//  conversions using "repeat sequence-of-channels" mode.  AVcc is used for the
//  reference and repeated sequence of conversions is performed on Channels A0,
//  A1, A2, and A3. Each conversion result is stored in ADC12MEM0, ADC12MEM1,
//  ADC12MEM2, and ADC12MEM3 respectively. After each sequence, the 4 conversion
//  results are moved to A0results[], A1results[], A2results[], and A3results[].
//  Test by applying voltages to channels A0 - A3. Open a watch window in
//  debugger and view the results. Set Breakpoint1 in the index increment line
//  to see the array values change sequentially and Breakpoint2 to see the entire
//  array of conversion results in A0results[], A1results[], A2results[], and
//  A3results[]for the specified Num_of_Results.
//
//  Note that a sequence has no restrictions on which channels are converted.
//  For example, a valid sequence could be A0, A3, A2, A4, A2, A1, A0, and A7.
//  See the CC430 User's Guide for instructions on using the ADC12_A.
//
//               CC430F5137
//             -----------------
//         /|\|                 |
//          | |                 |
//          --|RST              |
//            |                 |
//    Vin0 -->|P2.0/A0          |
//    Vin1 -->|P2.1/A1          |
//    Vin2 -->|P2.2/A2          |
//    Vin3 -->|P2.3/A3          |
//            |                 |
//
//   M. Morales
//   Texas Instruments Inc.
//   April 2009
//   Built with CCE Version: 3.2.2 and IAR Embedded Workbench Version: 4.11B
//******************************************************************************

#include <msp430.h>
#include <stdio.h>
#include <stdlib.h>

#define   Num_of_Results   	4
#define	  X_POS_TRIGGER  	2300
#define	  X_NEG_TRIGGER		1700
#define	  Y_POS_TRIGGER     2300
#define   Y_NEG_TRIGGER		1700
#define	  Z_POS_TRIGGER		2300
#define   Z_NEG_TRIGGER     1700
#define   FLEX_L_TRIGGER 	2800
#define   FLEX_R_TRIGGER  	2650
#define	  J2_TRIGGER		3750
#define   J3_TRIGGER		0 //TODO

#define   X_POS_CTRL		0x01
#define	  X_NEG_CTRL		0x02
#define   Y_POS_CTRL		0x04
#define	  Y_NEG_CTRL		0x08
#define   FLEX_L_CTRL		0x10
#define   FLEX_R_CTRL		0x20
#define   J2_DOWN_CTRL		0x40
#define   J3_UP_CTRL		0x80

volatile unsigned int A0results[Num_of_Results];
volatile unsigned int A1results[Num_of_Results];
volatile unsigned int A2results[Num_of_Results];
volatile unsigned int A3results[Num_of_Results];
volatile unsigned int A4results[Num_of_Results];
volatile unsigned int A5results[Num_of_Results];

volatile unsigned int results[6];
volatile unsigned int transmit_flag;

// Bit 0 = X-Acc, Bit 1 = Y-Acc, Bit 2 = Z-Acc, Bit 3 = Flex, Bit 4 = J2 (EMG_Down), Bit 5 = J3 (EMG_Up)
volatile unsigned char controls;

void uart_putc(unsigned char c);
void uart_puts(const char *str);

void uart_putc(unsigned char c)
{
	while (!(UCA0IFG&UCTXIFG));             // USCI_A0 TX buffer ready?
	    UCA0TXBUF = c;                  // TX -> RXed character
}
void uart_puts(const char *str)
{
     while(*str) uart_putc(*str++);
}


int main(void)
{
	char str[100];
	int i = 0;

  WDTCTL = WDTPW+WDTHOLD;                   // Stop watchdog timer
  //////////////////////////////////////////
  PMAPPWD = 0x02D52;                        // Get write-access to port mapping regs
  P1MAP5 = PM_UCA0RXD;                      // Map UCA0RXD output to P1.5
  P1MAP6 = PM_UCA0TXD;                      // Map UCA0TXD output to P1.6
  PMAPPWD = 0;                              // Lock port mapping registers

  P1DIR |= BIT6;                            // Set P1.6 as TX output
  P1SEL |= BIT5 + BIT6;                     // Select P1.5 & P1.6 to UART function

  UCA0CTL1 |= UCSWRST;                      // **Put state machine in reset**
  UCA0CTL1 |= UCSSEL_1;                     // CLK = ACLK
  UCA0BR0 = 0x03;                           // 32kHz/9600=3.41 (see User's Guide)
  UCA0BR1 = 0x00;                           //
  UCA0MCTL = UCBRS_3+UCBRF_0;               // Modulation UCBRSx=3, UCBRFx=0
  UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
  UCA0IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt

  uart_puts((char *)"\n\r***************\n\rArmband\n\r");
  uart_puts((char *)"***************\n\r\n\r");


  //////////////////////////////////////////
  
  P2SEL = 0x0F;                             // Enable A/D channel inputs
  
  ADC12CTL0 = ADC12ON+ADC12MSC+ADC12SHT0_12; // Turn on ADC12_A, extend sampling time
                                            // to avoid overflow of results
  ADC12CTL1 = ADC12SHP+ADC12CONSEQ_3;       // Use sampling timer, repeated sequence
  ADC12MCTL0 = ADC12INCH_0 + ADC12SREF_0;                 // ref+=AVcc, channel = A0
  ADC12MCTL1 = ADC12INCH_1 + ADC12SREF_0;                 // ref+=AVcc, channel = A1
  ADC12MCTL2 = ADC12INCH_2 + ADC12SREF_0;                 // ref+=AVcc, channel = A2
  ADC12MCTL3 = ADC12INCH_3 + ADC12SREF_0;
  ADC12MCTL4 = ADC12INCH_4 + ADC12SREF_0;        // ref+=AVcc, channel = A3, end seq.
  ADC12MCTL5 = ADC12INCH_5 + ADC12SREF_0 + ADC12EOS;        // ref+=AVcc, channel = A3, end seq.// ref+=AVcc, channel = A3, end seq.
  ADC12IE = 0x08;                           // Enable ADC12IFG.3
  ADC12CTL0 |= ADC12ENC;                    // Enable conversions
  ADC12CTL0 |= ADC12SC;                     // Start conversion - software trigger
  


  __bis_SR_register(/*LPM0_bits + */GIE);       // Enter LPM0, Enable interrupts
  __no_operation();                         // For debugger  



   while(1)
   {
	   if (transmit_flag)
	   {
		   results[0] = 0;
		   results[1] = 0;
		   results[2] = 0;
		   results[3] = 0;
		   results[4] = 0;
		   results[5] = 0;
		   for(i = 0; i < Num_of_Results; i++)
		   {
			   results[0] += A0results[i];
			   results[1] += A1results[i];
			   results[2] += A2results[i];
			   results[3] += A3results[i];
			   results[4] += A4results[i];
			   results[5] += A5results[i];
		   }
		   results[0] = results[0]/Num_of_Results;
		   results[1] = results[1]/Num_of_Results;
		   results[2] = results[2]/Num_of_Results;
		   results[3] = results[3]/Num_of_Results;
		   results[4] = results[4]/Num_of_Results;
		   results[5] = results[5]/Num_of_Results;

		   /* ADC base
				X - Axis = 2045
				Y - Axis = 2005
				Z - Axis = 2070
				Flex = 2720
				J2 (Channel 5) = 3450
				J3 (Channel 6) = TODO;
			*/

		   controls = (results[0] > X_POS_TRIGGER) ? controls|X_POS_CTRL : controls&(~X_POS_CTRL);
		   controls = (results[0] < X_NEG_TRIGGER) ? controls|X_NEG_CTRL : controls&(~X_NEG_CTRL);
		   controls = (results[1] > Y_POS_TRIGGER) ? controls|Y_POS_CTRL : controls&(~Y_POS_CTRL);
		   controls = (results[1] < Y_NEG_TRIGGER) ? controls|Y_NEG_CTRL : controls&(~Y_NEG_CTRL);
		   //controls = (results[3] > Z_POS_TRIGGER) ? controls|Z_POS_CTRL : controls&(~Z_POS_CTRL);
		   //controls = (results[3] < Z_NEG_TRIGGER) ? controls|Z_NEG_CTRL : controls&(~Z_NEG_CTRL);
		   controls = (results[2] > FLEX_L_TRIGGER) ? controls|FLEX_L_CTRL : controls&(~FLEX_L_CTRL);
		   controls = (results[2] < FLEX_R_TRIGGER) ? controls|FLEX_R_CTRL : controls&(~FLEX_R_CTRL);
		   controls = (results[4] > J2_TRIGGER) ?  controls|J2_DOWN_CTRL : controls&(~J2_DOWN_CTRL);
		   controls = controls&(~J3_UP_CTRL);//(results[5] > J3_TRIGGER) ? controls|J3_UP_CTRL : controls&(~J3_UP_CTRL);

		   sprintf(str, "Active: %x\n\r", controls);
		   //sprintf(str, "x value: %d    y value: %d    z value: %d    flex: %d    EMG: %d\n\r", results[0], results[1], results[3], results[2], results[4]);
		   uart_puts(str);

		   transmit_flag = 0;
	   }

   }
}

#pragma vector=ADC12_VECTOR
__interrupt void ADC12ISR (void)
{
  static unsigned int index = 0;

  switch(__even_in_range(ADC12IV,34))
  {
  case  0: break;                           // Vector  0:  No interrupt
  case  2: break;                           // Vector  2:  ADC overflow
  case  4: break;                           // Vector  4:  ADC timing overflow
  case  6: break;                           // Vector  6:  ADC12IFG0
  case  8: break;                           // Vector  8:  ADC12IFG1
  case 10: break;                           // Vector 10:  ADC12IFG2
  case 12:                                  // Vector 12:  ADC12IFG3
    A0results[index] = ADC12MEM0;           // Move A0 results, IFG is cleared
    A1results[index] = ADC12MEM1;           // Move A1 results, IFG is cleared
    A2results[index] = ADC12MEM2;           // Move A2 results, IFG is cleared
    A3results[index] = ADC12MEM3;           // Move A3 results, IFG is cleared
    A4results[index] = ADC12MEM4;           // Move A2 results, IFG is cleared
    A5results[index] = ADC12MEM5;
    index++;                                // Increment results index, modulo; Set Breakpoint1 here
    
    if (index == Num_of_Results)
    {
    	index = 0;							// Reset index, Set breakpoint 2 here
    	transmit_flag = 1;
    }
    
  case 14: break;                           // Vector 14:  ADC12IFG4
  case 16: break;                           // Vector 16:  ADC12IFG5
  case 18: break;                           // Vector 18:  ADC12IFG6
  case 20: break;                           // Vector 20:  ADC12IFG7
  case 22: break;                           // Vector 22:  ADC12IFG8
  case 24: break;                           // Vector 24:  ADC12IFG9
  case 26: break;                           // Vector 26:  ADC12IFG10
  case 28: break;                           // Vector 28:  ADC12IFG11
  case 30: break;                           // Vector 30:  ADC12IFG12
  case 32: break;                           // Vector 32:  ADC12IFG13
  case 34: break;                           // Vector 34:  ADC12IFG14
  default: break; 
  }  
}

// Echo back RXed character, confirm TX buffer is ready first
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
{
  switch(__even_in_range(UCA0IV,4))
  {
  case 0:break;                             // Vector 0 - no interrupt
  case 2:                                   // Vector 2 - RXIFG
    while (!(UCA0IFG&UCTXIFG));             // USCI_A0 TX buffer ready?
    UCA0TXBUF = UCA0RXBUF;                  // TX -> RXed character
    while (!(UCA0IFG&UCTXIFG));             // USCI_A0 TX buffer ready?
    UCA0TXBUF = 'T';                  // TX -> RXed character
    break;
  case 4:break;                             // Vector 4 - TXIFG
  default: break;
  }
}