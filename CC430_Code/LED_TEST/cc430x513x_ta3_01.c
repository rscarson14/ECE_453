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
//  CC430F513x Demo - Timer_A3, Toggle P1.0, CCR0 Cont. Mode ISR, DCO SMCLK
//
//  Description: Toggle P1.0 using software and TA_0 ISR. Toggles every
//  50000 SMCLK cycles. SMCLK provides clock source for TACLK.
//  During the TA_0 ISR, P1.0 is toggled and 50000 clock cycles are added to
//  CCR0. TA_0 ISR is triggered every 50000 cycles. CPU is normally off and
//  used only during TA_ISR.
//  ACLK = n/a, MCLK = SMCLK = TACLK = default DCO ~1.045MHz
//
//           CC430F5137
//         ---------------
//     /|\|               |
//      | |               |
//      --|RST            |
//        |               |
//        |           P1.0|-->LED
//
//   M Morales
//   Texas Instruments Inc.
//   April 2009
//   Built with CCE Version: 3.2.2 and IAR Embedded Workbench Version: 4.11B
//******************************************************************************

#include <msp430.h>

void InitButtonLeds();

volatile int counter;
unsigned char buttonPressed = 0;

int main(void)
{
   WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
  counter = 0;
  P2DIR |= 0x40;                            // P2.6 output
  P3DIR |= 0x20;

 //P2OUT &= 0x3F;                            // Toggle P2.6
 // P2OUT |= 0x40;
  //P3OUT &= 0xC0;
//  InitButtonLeds();
  TA1CCTL0 = CCIE;                          // CCR0 interrupt enabled
  TA1CCR0 = 50000;
  TA1CTL = TASSEL_2 + MC_2 + TACLR;         // SMCLK, contmode, clear TAR

 // __bis_SR_register(LPM0_bits + GIE);       // Enter LPM0, enable interrupts
 // __no_operation();                         // For debugger
/*
  while (1)
    {
      __bis_SR_register( LPM3_bits + GIE );
      __no_operation();

      if (buttonPressed)                      // Process a button press->transmit
      {
    	  P2OUT &= 0x3F;                        // Pulse LED during Transmit
    	  buttonPressed = 0;
    	  P1IFG = 0;

    	  P1IE |= BIT0;                         // Re-enable button press
      }

    }

 */
}

void InitButtonLeds()
{
  // Set up the button as interruptible
	P1DIR &= ~BIT0;
	  P1REN |= BIT0;
	  P1IES &= BIT0;
	  P1IFG = 0;
	  P1OUT |= BIT0;
	  P1IE  |= BIT0;

  // Initialize Port J
//  PJOUT = 0x00;
//  PJDIR = 0xFF;

  // Set up LEDs
//  P1OUT &= ~BIT0;
//  P1DIR |= BIT0;
//  P3OUT &= ~BIT6;
//  P3DIR |= BIT6;
}

// Timer A0 interrupt service routine
#pragma vector=TIMER1_A0_VECTOR
__interrupt void TIMER1_A0_ISR(void)
{
  counter++;
  TA1CCR0 += 50000;                         // Add Offset to CCR0

  if (counter > 10)
  {
//	  P2OUT ^= 0xC0;                            // Toggle P2.6
//      P3OUT ^= 0x3F;
	  counter = 0;
  }
}

#pragma vector=PORT1_VECTOR
__interrupt void PORT1_ISR(void)
{
	//P2OUT &= 0x3F;
  switch(__even_in_range(P1IV, 16))
  {
    case  0: break;     					// Exit active
    case  2:
        P1IE = 0;                             // Debounce by disabling buttons
        buttonPressed = 1;
        __bic_SR_register_on_exit(LPM3_bits); // Exit active
    	break;                         // P1.0 IFG
    case  4: break;                         // P1.1 IFG
    case  6: break;                         // P1.2 IFG
    case  8: break;                         // P1.3 IFG
    case 10: break;                         // P1.4 IFG
    case 12: break;                         // P1.5 IFG
    case 14: break;                         // P1.6 IFG
    case 16:                                // P1.7 IFG
      P1IE = 0;                             // Debounce by disabling buttons
      buttonPressed = 1;
      __bic_SR_register_on_exit(LPM3_bits); // Exit active
      break;
 //   default:   P2OUT &= 0x3F;                        // Pulse LED during Transmit
//      break;
  }
}
