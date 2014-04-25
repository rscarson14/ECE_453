/******************************************************************************
* CC430 RF Code Example - TX and RX (fixed packet length =< FIFO size)
*
* Simple RF Link to Toggle Receiver's LED by pressing Transmitter's Button    
* Warning: This RF code example is setup to operate at either 868 or 915 MHz, 
* which might be out of allowable range of operation in certain countries.
* The frequency of operation is selectable as an active build configuration
* in the project menu. 
* 
* Please refer to the appropriate legal sources before performing tests with 
* this code example. 
* 
* This code example can be loaded to 2 CC430 devices. Each device will transmit 
* a small packet, less than the FIFO size, upon a button pressed. Each device will also toggle its LED 
* upon receiving the packet. 
* 
* The RF packet engine settings specify fixed-length-mode with CRC check 
* enabled. The RX packet also appends 2 status bytes regarding CRC check, RSSI 
* and LQI info. For specific register settings please refer to the comments for 
* each register in RfRegSettings.c, the CC430x513x User's Guide, and/or 
* SmartRF Studio.
* 
* G. Larmore
* Texas Instruments Inc.
* June 2012
* Built with IAR v5.40.1 and CCS v5.2
******************************************************************************/

#include "RF_Toggle_LED_Demo.h"
#include <stdio.h>
#include <stdlib.h>

#define  PACKET_LEN         (0x05)      // PACKET_LEN <= 61
#define  RSSI_IDX           (PACKET_LEN)    // Index of appended RSSI 
#define  CRC_LQI_IDX        (PACKET_LEN+1)  // Index of appended LQI, checksum
#define  CRC_OK             (BIT7)          // CRC_OK bit 
#define  PATABLE_VAL        (0x51)          // 0 dBm output 

extern RF_SETTINGS rfSettings;

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

unsigned char packetReceived;
unsigned char packetTransmit; 

volatile unsigned char RxBuffer[PACKET_LEN+2];
unsigned char RxBufferLength = 0;
const unsigned char TxBuffer[PACKET_LEN]= {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
unsigned char buttonPressed = 0;
unsigned int i = 0; 

unsigned char transmitting = 0; 
unsigned char receiving = 0; 

char str[100];

void main( void )
{  
  // Stop watchdog timer to prevent time out reset 
  WDTCTL = WDTPW + WDTHOLD; 

  // Increase PMMCOREV level to 2 for proper radio operation
  SetVCore(2);

  /////////////////////////////////////////////////////////////////////////////////////////
  // SPI INIT
  
  PMAPPWD = 0x02D52;                        // Get write-access to port mapping regs
  //P2MAP0 = PM_UCA0SIMO;                     // Map UCA0SIMO output to P2.0
  //P2MAP2 = PM_UCA0SOMI;                     // Map UCA0SOMI output to P2.2
  //P2MAP4 = PM_UCA0CLK;                      // Map UCA0CLK output to P2.4

  P1MAP4 = PM_UCB0CLK;
  P1MAP3 = PM_UCB0SIMO;
  P1MAP2 = PM_UCB0SOMI;

  PMAPPWD = 0;                              // Lock port mapping registers

  P2OUT |= BIT6 + BIT7;                            // Set P1.0 for LED
                                            // Set P1.2 for slave reset
  P2DIR |= BIT6 + BIT7;                     // Set P1.0, P1.2 to output direction
  P1DIR |= BIT2 + BIT3 + BIT4;              // ACLK, MCLK, SMCLK set out to pins
  P1SEL |= BIT2 + BIT3 + BIT4;              // P2.0,2,4 for debugging purposes.

  UCB0CTL1 |= UCSWRST;                      // **Put state machine in reset**
//  UCB0CTL0 |= UCMST+UCSYNC+UCCKPL+UCMSB;    // 3-pin, 8-bit SPI master
  UCB0CTL0 |= UCSYNC+UCCKPL+UCMSB;    // 3-pin, 8-bit SPI slave
                                            // Clock polarity high, MSB
  UCB0CTL1 |= UCSSEL_2;                     // SMCLK
  UCB0BR0 = 0x02;                           // /2
  UCB0BR1 = 0;                              //
  UCA0MCTL = 0;                             // No modulation
  UCB0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
  UCB0IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt

//  P1OUT &= ~0x02;                           // Now with SPI signals initialized,
//  P1OUT |= 0x02;                            // reset slave

  __delay_cycles(100);                      // Wait for slave to initialize

///////////////////////////////////////////////////////////////////////////////////////////////////
  ResetRadioCore();     
  InitRadio();
  InitButtonLeds();
  
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

  ReceiveOn(); 
  receiving = 1; 
    
  while (1)
  { 
    __bis_SR_register( LPM3_bits + GIE );   
    __no_operation(); 
    
    if (buttonPressed)                      // Process a button press->transmit 
    {
      P2OUT |= BIT7;                        // Pulse LED during Transmit
      buttonPressed = 0; 
      P1IFG = 0; 
      
      ReceiveOff();
      receiving = 0; 
      Transmit( (unsigned char*)TxBuffer, sizeof TxBuffer);         
      transmitting = 1;
       
      P1IE |= BIT0;                         // Re-enable button press
    }
    else if(!transmitting)
    {
      ReceiveOn();      
      receiving = 1; 
    }
  }
}

void InitButtonLeds(void)
{
  // Set up the button as interruptible 
  P1DIR &= ~BIT0;
  P1REN |= BIT0;
  P1IES &= BIT0;
  P1IFG = 0;
  P1OUT |= BIT0;
  P1IE  |= BIT0;

  // Initialize Port J
  PJOUT = 0x00;
  PJDIR = 0xFF; 

  // Set up LEDs 
  P2OUT &= ~BIT6;
  P2DIR |= BIT6;
  P2OUT &= ~BIT7;
  P2DIR |= BIT7;
}

void InitRadio(void)
{
  // Set the High-Power Mode Request Enable bit so LPM3 can be entered
  // with active radio enabled 
  PMMCTL0_H = 0xA5;
  PMMCTL0_L |= PMMHPMRE_L; 
  PMMCTL0_H = 0x00; 
  
  WriteRfSettings(&rfSettings);
  
  WriteSinglePATable(PATABLE_VAL);
}

#pragma vector=PORT1_VECTOR
__interrupt void PORT1_ISR(void)
{
  switch(__even_in_range(P1IV, 16))
  {
    case  0: break;
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
      break;
  }
}

void Transmit(unsigned char *buffer, unsigned char length)
{
  RF1AIES |= BIT9;                          
  RF1AIFG &= ~BIT9;                         // Clear pending interrupts
  RF1AIE |= BIT9;                           // Enable TX end-of-packet interrupt
  
  WriteBurstReg(RF_TXFIFOWR, buffer, length);     
  
  Strobe( RF_STX );                         // Strobe STX   
}

void ReceiveOn(void)
{  
  RF1AIES |= BIT9;                          // Falling edge of RFIFG9
  RF1AIFG &= ~BIT9;                         // Clear a pending interrupt
  RF1AIE  |= BIT9;                          // Enable the interrupt 
  
  // Radio is in IDLE following a TX, so strobe SRX to enter Receive Mode
  Strobe( RF_SRX );                      
}

void ReceiveOff(void)
{
  RF1AIE &= ~BIT9;                          // Disable RX interrupts
  RF1AIFG &= ~BIT9;                         // Clear pending IFG

  // It is possible that ReceiveOff is called while radio is receiving a packet.
  // Therefore, it is necessary to flush the RX FIFO after issuing IDLE strobe 
  // such that the RXFIFO is empty prior to receiving a packet.
  Strobe( RF_SIDLE );
  Strobe( RF_SFRX  );                       
}

#pragma vector=CC1101_VECTOR
__interrupt void CC1101_ISR(void)
{
  switch(__even_in_range(RF1AIV,32))        // Prioritizing Radio Core Interrupt 
  {
    case  0: break;                         // No RF core interrupt pending                                            
    case  2: break;                         // RFIFG0 
    case  4: break;                         // RFIFG1
    case  6: break;                         // RFIFG2
    case  8: break;                         // RFIFG3
    case 10: break;                         // RFIFG4
    case 12: break;                         // RFIFG5
    case 14: break;                         // RFIFG6          
    case 16: break;                         // RFIFG7
    case 18: break;                         // RFIFG8
    case 20:                                // RFIFG9
      if(receiving)         // RX end of packet
      {
        // Read the length byte from the FIFO       
        RxBufferLength = ReadSingleReg( RXBYTES );               
        ReadBurstReg(RF_RXFIFORD, RxBuffer, RxBufferLength); 
        
        // Stop here to see contents of RxBuffer
        __no_operation();

       // sprintf(str, "Active: %x\n\r", RxBuffer[PACKET_LEN-2]);
        //sprintf(str, "x value: %d    y value: %d    z value: %d    flex: %d    EMG: %d\n\r", results[0], results[1], results[3], results[2], results[4]);
       // uart_puts(str);
        
        // Check the CRC results
        if(RxBuffer[CRC_LQI_IDX] & CRC_OK)  
          P2OUT ^= BIT6;                    // Toggle LED1
      }
      else if(transmitting)       // TX end of packet
      {
        RF1AIE &= ~BIT9;                    // Disable TX end-of-packet interrupt
        P2OUT &= ~BIT7;                     // Turn off LED after Transmit
        transmitting = 0; 
      }
      else while(1);          // trap 
      break;
    case 22: break;                         // RFIFG10
    case 24: break;                         // RFIFG11
    case 26: break;                         // RFIFG12
    case 28: break;                         // RFIFG13
    case 30: break;                         // RFIFG14
    case 32: break;                         // RFIFG15
  }  
  __bic_SR_register_on_exit(LPM3_bits);     
}

#pragma vector=USCI_B0_VECTOR
__interrupt void USCI_B0_ISR(void)
{
  switch(__even_in_range(UCB0IV,4))
  {
    case 0: break;                          // Vector 0 - no interrupt
    case 2:                                 // Vector 2 - RXIFG
      while (!(UCB0IFG&UCTXIFG));           // USCI_A0 TX buffer ready?

//      if (UCB0RXBUF==SLV_Data)              // Test for correct character RX'd
//        P2OUT |= BIT6;                      // If correct, light LED
//      else
//        P2OUT &= ~BIT6;                     // If incorrect, clear LED

     //MST_Data = 0x04;                      // Increment data
     // SLV_Data++;
      UCB0TXBUF = RxBuffer[PACKET_LEN-2];                 // Send next value

      // Send 0x12 == 0001 0010  Receive 0x24 == 0010 0100
      // Send 0x24 == 0010 0100  Receive 0x84 == 1000 0100
      // Send 0x01 == 0000 0001  Receive 0x04 == 0000 0100
      // Send 0x02 == 0000 0010  Receive 0x08 == 0000 1000
      // Second Try
          // Send 0x02 == 0000 0010 Receive 0x80 == 1000 0000
      // Send 0x03 == 0000 0011  Receive 0x60 == 0110 0000
      // send 0x10 == 0001 0000  Receive 0x44 == 0100 0100
      // Send 0x20 == 0010 0000  Receive 0x02 == 0000 0010
      // Send 0x30 == 0011 0000  Receive 0x06 == 0000 0110


      __delay_cycles(40);                   // Add time between transmissions to
                                            // make sure slave can process information
      break;
    case 4: break;                          // Vector 4 - TXIFG
    default: break;
  }
}
