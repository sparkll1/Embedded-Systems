/*
 *
~~~~Pin Num~~~
LED  6.5, 6.6, 6.7
Input button 3.5,  3.6, 3.7      Reset Button 3.3
LCD whole Port 4,     E 5.6, RS 5.7
ADC 5.3
Buzzer 2.4 ( cannot be changed. Assigned for TA0 CCR1 )
~~~~~~~~~~~~~~
Timer A0   - ADC interrupt. It defines the time the each round of game starts
Timer A1   - It's last 2 digits decides which color will be used in the round + speaker notes frequency
Timer A2   -
*/
#include "msp.h"
#include <stdint.h>
#include <stdbool.h>
#include "stdio.h"
#include "csHFXT.h"
#include "lcd.h"
#include "math.h"
#include "sysTickDelays.h"


//for speaker. Need to make separate header+ class for this later

#define SIXT      2048
#define OCT        4096
#define QUA   8192
#define HALF    16384
#define FULL       32768
#define THIRD      24576



uint32_t notePeriod;

volatile uint32_t g = 0;

#define G3  61224
#define G3s  12000000/207.65

#define A3  54545

#define A3s 12000000/233.08
#define B3  48595
#define C4  45866
#define D4  40864
#define E4  36404
#define E4p 12000000/311.13
#define B4p 12000000/466.16
#define F4  12000000/349.23

#define G4  QUA/392
#define G4s 12000000/415.30

#define C5  QUA/523.25

#define D5  QUA/587.33

#define E5  QUA/659.25
#define E5p QUA/622.25

#define F5  QUA/698.46

#define G5  QUA/783.99
#define G5s QUA/830.61


#define B6p QUA/932.33
#define C6  QUA/1046.5





const uint16_t winningSong[32] = {G3, C4, E4, G4, C5, E5,       G5, E5,
                                    G3s, C4, E4p, G4s, C5, E5p,   G5s, E5p,
                                    A3s, D4, F4, B4p, D5, F5, B6p, 0,  B6p,0  ,B6p,0, B6p,0, C6, 0};


const uint32_t winningDur[32] = {OCT,OCT,OCT,OCT,OCT,OCT,    THIRD,THIRD,
                                  OCT,OCT,OCT,OCT,OCT,OCT,    THIRD,THIRD,
                                  OCT,OCT,OCT,OCT,OCT,OCT,
                                  THIRD, OCT,SIXT, OCT,SIXT, OCT,SIXT, HALF, OCT};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


#define CLK_FREQUENCY       48000000    // MCLK using 48MHz HFXT
#define rButton     BIT3
#define gButton     BIT4
#define yButton     BIT5  //yellow
#define rLED        BIT5
#define gLED        BIT6
#define yLED        BIT7

const char p1[] = "Player 1: ";
const char p2[] = "Player 2: ";
const char p3[] = "PLAYER ";
const char p4[] = "WON !";

const uint8_t num[] = {5, 6, 7};
int spot = 0;

int p1Score;
int p2Score;

static volatile bool gameOver = false;
const uint8_t timeDelay = 12000;

static volatile uint16_t curADCResult;
static volatile bool resultReady = false;

void printMessage(const char* message, int msgLength);
//void updateScore(int player);
void buttonLEDPinSet(void);
/**
 * main.c
 */
void main(void)
{
    volatile int i;

    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;     // stop watchdog timer

    /*   This next section of code is made for setting
     *   up the LCD to show player 1's score on the first line
     *   and player 2's score on the second line.
     *
     */
    configHFXT();
    configLFXT();

    configLCD(CLK_FREQUENCY);
    initLCD();
  //  updateScore(3);

    /*
     *  HC06 to UART
     *
     */
    /* Configure MCLK/SMCLK source to DCO, with DCO = 12MHz */
        CS->KEY = CS_KEY_VAL;                   // Unlock CS module for register access
        CS->CTL0 = 0;                           // Reset tuning parameters
        CS->CTL0 = CS_CTL0_DCORSEL_3;           // Set DCO to 12MHz (nominal, center of 8-16MHz range)
        CS->CTL1 = CS_CTL1_SELA_2 |             // Select ACLK = REFO
                CS_CTL1_SELS_3 |                // SMCLK = DCO
                CS_CTL1_SELM_3;                 // MCLK = DCO
        CS->KEY = 0;                            // Lock CS module from unintended accesses

 //       initPWMRGBLED();                        // configure 3 RGB LED pins for PWM output

        /* Configure UART pins */
        P1->SEL0 |= BIT2 | BIT3;                // set 2-UART pins as secondary function
        P1->SEL1 &= ~(BIT2 | BIT3);

        /* Configure UART
         *  Asynchronous UART mode, 8N1 (8-bit data, no parity, 1 stop bit),
         *  LSB first, SMCLK clock source
         */
        EUSCI_A0->CTLW0 |= EUSCI_A_CTLW0_SWRST; // Put eUSCI in reset
        EUSCI_A0->CTLW0 = EUSCI_A_CTLW0_SWRST | // Remain eUSCI in reset
                EUSCI_A_CTLW0_SSEL__SMCLK;      // Configure eUSCI clock source for SMCLK

        /* Baud Rate calculation
         * Refer to Section 24.3.10 of Technical Reference manual
         * BRCLK = 12000000, Baud rate = 9600
         * N = fBRCLK / Baud rate = 12000000/9600 = 1250
         * from Technical Reference manual Table 24-5:
         *
         *
         */
        //clock prescaler in EUSCI_A2 baud rate control register
        EUSCI_A0->BRW |= 0x4E; //78
        //configure baud clock modulation in EUSCI_A2 modulation control register
        EUSCI_A0->MCTLW |= 0b100001;

        EUSCI_A0->CTLW0 = 0b0000000111000000;    // Initialize eUSCI
        EUSCI_A0->IFG &= ~EUSCI_A_IFG_RXIFG;        // Clear eUSCI RX interrupt flag
        EUSCI_A0->IE |= EUSCI_A_IE_RXIE;            // Enable USCI_A2 RX interrupt



    /*  Setting up TimerA1 to run indefinitely
     *  to a value of 12000 for an element of randomness
     *
     */


    NVIC->ISER[0] |= 0x800;

    /*  Next section for setting up ADC input
     *
     *
     *
     */


        /* Stop Watchdog timer */
        WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;

        // GPIO Setup  FOR TESTING ADC
        P1->SEL0 &= ~BIT0;                      // Set LED1 pin to GPIO function
        P1->SEL1 &= ~BIT0;
        P1->OUT &= ~BIT0;                       // Clear LED1 to start
        P1->DIR |= BIT0;                        // Set P1.0/LED1 to output

        // Configure P5.4 for ADC (tertiary module function)
        P5->SEL0 |= BIT4;
        P5->SEL1 |= BIT4;
        P5->DIR &= ~BIT4;
        P5->REN &= ~BIT4;
        P5->OUT &= ~BIT4;

        buttonLEDPinSet();

        /* Configure ADC (CTL0 and CTL1) registers for:
         *      clock source - default MODCLK, clock prescale 1:1,
         *      sample input signal (SHI) source - software controlled (ADC14SC),
         *      Pulse Sample mode with sampling period of 16 ADC14CLK cycles,
         *      Single-channel, single-conversion mode, 12-bit resolution,
         *      ADC14 conversion start address ADC14MEM1, and Low-power mode
         */
        ADC14->CTL0 = ADC14_CTL0_SHP                // Pulse Sample Mode
                        | ADC14_CTL0_SHT0__16       // 16 cycle sample-and-hold time (for ADC14MEM1)
                        | ADC14_CTL0_PDIV__1        // Predivide by 1
                        | ADC14_CTL0_DIV__1         // /1 clock divider
                        | ADC14_CTL0_SHS_0          // ADC14SC bit sample-and-hold source select
                        | ADC14_CTL0_SSEL__MODCLK   // clock source select MODCLK
                        | ADC14_CTL0_CONSEQ_0       // Single-channel, single-conversion mode
                        | ADC14_CTL0_ON;            // ADC14 on

        ADC14->CTL1 = ADC14_CTL1_RES__12BIT         // 12-bit conversion results
                | (0x1 << ADC14_CTL1_CSTARTADD_OFS) // ADC14MEM1 - conversion start address
                | ADC14_CTL1_PWRMD_2;               // Low-power mode

        // Configure ADC14MCTL1 as storage register for result
        //          Single-ended mode with Vref+ = Vcc and Vref- = Vss,
        //          Input channel - A1, and comparator window disabled

        ADC14->MCTL[1] = 0x1;

        // Enable ADC conversion complete interrupt for ADC14MEM1
        ADC14->IER0 = 0x2;


        // speaker ~~~~~~~


         P2->DIR |= BIT4;            // set pin 2.4 for speaker output
         P2->SEL0 |= BIT4;           //
         P2->SEL1 &= ~BIT4;

         TIMER_A1->CTL |= 0x0126;
         TIMER_A1->CCR[2] = notePeriod; // Set initial period in CCR2 register.
         TIMER_A1->CCTL[2] = 0x0010;   //compare with interrupt enabled

         TIMER_A1->CTL &= ~0x0020;

         // random
         TIMER_A3->CTL |= 0x0126;
         TIMER_A3->CCR[2] = notePeriod; // Set initial period in CCR2 register.
         TIMER_A3->CCTL[2] = 0x0010;   //compare with interrupt enabled



         //For Speaker frequency
         TIMER_A0->CCTL[1] |= 0x0060;
         TIMER_A0->CTL |= 0x0294;

         //Enable TA1CCR2 compare interrupt
         NVIC ->ISER[0] |= 0x800;

       // ~~~~~~~~~~~~~~~~~~~~~~~~~~~

        // Enable ADC interrupt in NVIC module
        NVIC->ISER[0] |= (1 << ADC14_IRQn);
        // Enable eUSCIA2 interrupt in NVIC module
        NVIC->ISER[0] |= (1 << EUSCIA2_IRQn );

        // Enable global interrupt
        __enable_irq();
        p1Score = 0;
        p2Score = 0;
        while (1)
        {

            // Enable and start sampling/conversion by ADC
            ADC14->CTL0 |= 0x3;

            // wait for conversion to complete
            while (!resultReady);
            resultReady = false;


            //  Next section for lighting up specific led

            uint8_t rand = ((TIMER_A3->R) & 0b11) +5 ;
            while (rand == 0x08){    // in case if number is 8
                rand = ((TIMER_A3->R) & 0b11) +5 ;
                if(rand != 0x08){
                    break;
                }
            }


            uint8_t binary = 0x01 << rand;

            int Player1input = (P3->IN & binary) >> rand;     // pressed : timerVal    not pressed : anything else


            TIMER_A1->CTL &= 0xFFCF;  // Timer A1 is halted ( it was in continuous mode)

            // need to catch the illegal input
            while(Player1input != 0)  { //button pin - 5,6,7 bit

                P6->OUT =  binary; // turn on the corresponding LED
                Player1input = (P3->IN & binary) >> rand;  // keep checking

            }

            updateScore(1);
            EUSCI_A0->TXBUF = '1';
            P6->OUT &=  ~binary; // toggle the LED with xor

            if(p1Score >= 10 || p2Score >= 10) {
                gameOver = true;




      //      TIMER_A1->CTL |= 0x0020;
            }
            if(gameOver) {

                winPhase(1);     // parameter : # of player who won

                TIMER_A1->CTL |= ~0x0020;

           //     binary = 0x01 << 7;
          //      timerVal = 7;

                int reset = P1->IN & 0b10;
                while(reset != 0){
                    TIMER_A1->CTL &= 0xFFCF;
                    reset = P1->IN & 0b10;
                }
                gameOver = false;
                p1Score = 0;
                p2Score = 0;
                updateScore(0);
            }
        }
        for (i = 1250000; i > 0; i--);        // lazy delay

}

void winPhase(int whichPlayer){
    clearDisplay();
    int i;
     for(i = 0; i < 7 ; i++) {
          printChar(p3[i]);
     }

     char a = whichPlayer + '0';
     printChar(a);

     MoveCursor(1);

     int j;
     for(j = 0; j < 4 ; j++) {
          printChar(p4[j]);
     }
}

//void winningSong(){

//}

void buttonLEDPinSet(void) {
        //Pins 3.5, 3.6, 3.7 for input buttons
      P3->SEL0 &= ~0b11100000;      // set for GPIO
      P3->SEL1 &= ~0b11100000;
      P3->DIR &= ~0b11100000;       // set as input
      P3->OUT |= 0b11100000;        // enable internal pull-up resistor on
      P3->REN |= 0b11100000;    //111000

      P1->SEL0 &= ~BIT1;      // set reset P1.1
      P1->SEL1 &= ~BIT1;
      P1->DIR &= ~BIT1;
      P1->OUT |= BIT1;
      P1->REN |= BIT1;




      //use p2->IN register for reading

      //Pins 6.4 ~ 6.7 for individual RGB leds
      P6->SEL0 &= ~0b11110000;
      P6->SEL1 &= ~0b11110000;
      P6->DIR |= 0b11110000;
      P6->OUT &= 0b00001111;


}

// ADC14 interrupt service routine
void ADC14_IRQHandler(void) {
    /*
     *  Needs to change based off of what we want to do with
     *  the ADC result that is returned. Also to do later: set up for second adc.
     *
     *
     */
    // Check if interrupt triggered by ADC14MEM1 conversion value loaded
    //  Not necessary for this example since only one ADC channel used
    if (ADC14->IFGR0 & ADC14_IFGR0_IFG1) {
        /*
         *
         *  Currently this method is only meant for testing the ADC conversion
         */
        curADCResult = ADC14->MEM[1];
  //      timeDelay = curADCResult * 10;    //temporary commentate it bc it cuase error
        resultReady = true;
        // not necessary to clear flag because reading ADC14MEMx clears flag
    }
}

void TA1_N_IRQHandler(void) {
    if(TIMER_A1->CCTL[2] & TIMER_A_CCTLN_CCIFG) {

        // Set Period in CCR register
        TIMER_A0->CCR[0] = winningSong[g];
        TIMER_A0->CCR[1] = (winningSong[g] / 2) ;
        //Increment until it reaches the end of the array. If it reaches the end, it goes back to 0.


        //clear the flag
        TIMER_A1->CCTL[2] &= 0xFFFE;


        TIMER_A1->CCR[2] += winningDur[g]  ;

        g++;
        if(g == 31){  // if the song ended, the timer halts
            g = 0;
            TIMER_A1->CTL &= 0xFFCF;
            TIMER_A0->CTL &= 0xFFCF;

        }
/*


        g++;

        notePeriod = winningDur[g];
        TIMER_A0->CCR[0] = winningSong[g] - 1;
        TIMER_A0->CCR[1] = (winningSong[g] / 2) - 1;

        P1->OUT ^= BIT0;                    // toggle LED1

        TIMER_A1->CCTL[2] = 0x10;
        // Schedule next interrupt interval
        TIMER_A1->CCR[2] = ((TIMER_A1->CCR[2]) + notePeriod);

        if(g == 31){  // if the song ended, the timer halts
            g = 0;
            TIMER_A1->CTL &= 0xFFCF;
            TIMER_A0->CTL &= 0xFFCF;

        }
*/

    }
}

void updateScore(int player) {


    /*  This method is for updating the player's score by one
     *  if they successfully tapped the button first.
     *
     *
     */


    clearDisplay();
    if(player == 1) {
        p1Score++;
    }
    else if(player == 2) {
        p2Score++;
    }


    int o;
    for(o = 0; o < 10 ; o++) {
         printChar(p1[o]);
    }

    char a = p1Score + '0';
    if(p1Score == 10) {
        a = 1 + '0';
        printChar(a);
        a = 0 +'0';
    }


    printChar(a);

    MoveCursor(1);

    int t;
    for(t = 0; t < 10 ; t++) {
         printChar(p2[t]);
    }

    char b = p2Score + '0';
    printChar(b);




}

