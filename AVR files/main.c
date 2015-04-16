/* Name: main.c
 * Project: Data input based on AVR USB driver with TINY45
 * V3: 2 inputs ADC2/PB4 ADC3/PB3, pas de switch, led sur PB1 (on=0)
 * Author: Jacques Lepot
 * Creation Date: 2008-04-19
 * Copyright: (c) 2008 by Jacques LEpot
 * License: Proprietary, free under certain conditions. 

 */

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "usbdrv.h"
#include "oddebug.h"

/*
Pin assignment:
PB4 = analog input (ADC2)
PB3 = analog input (ADC3)
PB1 = LED output (active low)

PB0, PB2 = USB data lines
*/

#define BIT_LED 1
#define BIT_KEY 4


#define UTIL_BIN4(x)        (uchar)((0##x & 01000)/64 + (0##x & 0100)/16 + (0##x & 010)/4 + (0##x & 1))
#define UTIL_BIN8(hi, lo)   (uchar)(UTIL_BIN4(hi) * 16 + UTIL_BIN4(lo))

#ifndef NULL
#define NULL    ((void *)0)
#endif

/* ------------------------------------------------------------------------- */

static uchar    reportBuffer[5];    /* buffer for HID reports, type game cntrler*/
static uchar    idleRate;           /* in 4 ms units */

static uchar    adcPending;
static uchar    isRecording;
static uchar    adchanel;
static int		adcval1;
static int		adcval2;

/* ------------------------------------------------------------------------- */

PROGMEM char usbHidReportDescriptor[USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] = { /* USB report descriptor */
    0x05, 0x01,        // USAGE_PAGE (Generic Desktop = 01)
	0x09, 0x05,        // USAGE (Game Pad = 05)
	0xa1, 0x01,        // COLLECTION (Application)
	0x09, 0x01,        //   USAGE (Pointer)
	0xa1, 0x00,        //   COLLECTION (Physical)
	0x09, 0x30,        //     USAGE (X)
	0x09, 0x31,        //     USAGE (Y)
	0x15, 0x00,        //   LOGICAL_MINIMUM (0)
	0x26, 0xff, 0x00,  //     LOGICAL_MAXIMUM (255)
	0x75, 0x08,        //   REPORT_SIZE (8bits)
	0x95, 0x04,        //   REPORT_COUNT (2)
	0x81, 0x02,        //   INPUT (Data,Var,Abs)
	0xc0,              // END_COLLECTION
	
	0x05, 0x09,        // USAGE_PAGE (Button)
	0x19, 0x01,        //   USAGE_MINIMUM (Button 1)
	0x29, 0x08,        //   USAGE_MAXIMUM (Button 8)
	0x15, 0x00,        //   LOGICAL_MINIMUM (0)
	0x25, 0x01,        //   LOGICAL_MAXIMUM (1)
	0x75, 0x01,        // REPORT_SIZE (1bit)
	0x95, 0x08,        // REPORT_COUNT (8)
	0x81, 0x02,        // INPUT (Data,Var,Abs)
	0xc0               // END_COLLECTION
};

/* ------------------------------------------------------------------------- */

static void buildReport(void)
{

	reportBuffer[0] = adcval1>>8;
	reportBuffer[1] = adcval1;
	reportBuffer[2] = adcval2>>8;
	reportBuffer[3] = adcval2;
	reportBuffer[4] = 0x01;

}

/* ------------------------------------------------------------------------- */

static void setIsRecording(uchar newValue)
{
    isRecording = newValue;
    if(isRecording){
        PORTB &= ~(1 << BIT_LED);      /* LED on */
    }else{
 		PORTB |= 1 << BIT_LED;    /* LED off */
    }
}

/* ------------------------------------------------------------------------- */

static void adcPoll(void)
{int value;
    if( !(ADCSRA & (1 << ADSC))){ 		//adcPending &&
       // adcPending = 0;
        value=ADC;
		if(adchanel){
			adcval1 = value + value + (value >> 1);  /*  value * 2.5 for output in mV */
			ADMUX = UTIL_BIN8(1001, 0011);
			}
		else{
			adcval2 = value + value + (value >> 1);
			ADMUX = UTIL_BIN8(1001, 0010);
		}
		adchanel=!adchanel;
		ADCSRA |= (1 << ADSC);  /* start next conversion */
    }
}

static void timerPoll(void)
{
static uchar timerCnt;

    if(TIFR & (1 << TOV1)){
        TIFR = (1 << TOV1); /* clear overflow */
        if(adcPending == 0){
                adcPending = 1;
                ADCSRA |= (1 << ADSC);  /* start next conversion */
        }

    }
}

/* ------------------------------------------------------------------------- */

static void timerInit(void)
{
    TCCR1 = 0x0b;           /* select clock: 16.5M/1k -> overflow rate = 16.5M/256k = 62.94 Hz */
}

static void adcInit(void)
{
    ADMUX = UTIL_BIN8(1001, 0011);  /* Vref=2.56V, measure ADC0-  REFS1 REFS0 ADLAR REFS2 , MUX3 MUX2 MUX1 MUX0 -> ADC2=0010, ADC3=0011, ADC2-ADC3=0110, (ADC2-ADC3)*20=0111*/
    ADCSRA = UTIL_BIN8(1000, 0111); /* enable ADC, not free running, interrupt disable, rate = 1/128 = 129khz*/
}

/* ------------------------------------------------------------------------- */
/* ------------------------ interface to USB driver ------------------------ */
/* ------------------------------------------------------------------------- */

uchar	usbFunctionSetup(uchar data[8])
{
usbRequest_t    *rq = (void *)data;

    usbMsgPtr = reportBuffer;
    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS){    /* class request type */
        if(rq->bRequest == USBRQ_HID_GET_REPORT){  /* wValue: ReportType (highbyte), ReportID (lowbyte) */
												   /* we only have one report type, so don't look at wValue */
            buildReport();
            return sizeof(reportBuffer);
        }else if(rq->bRequest == USBRQ_HID_GET_IDLE){
            usbMsgPtr = &idleRate;
            return 1;
        }else if(rq->bRequest == USBRQ_HID_SET_IDLE){
            idleRate = rq->wValue.bytes[1];
        }
    }else{
        /* no vendor specific requests implemented */
    }
	return 0;
}

/* ------------------------------------------------------------------------- */
/* ------------------------ Oscillator Calibration ------------------------- */
/* ------------------------------------------------------------------------- */

/* Calibrate the RC oscillator to 8.25 MHz. The core clock of 16.5 MHz is
 * derived from the 66 MHz peripheral clock by dividing. Our timing reference
 * is the Start Of Frame signal (a single SE0 bit) available immediately after
 * a USB RESET. We first do a binary search for the OSCCAL value and then
 * optimize this value with a neighboorhod search.
 * This algorithm may also be used to calibrate the RC oscillator directly to
 * 12 MHz (no PLL involved, can therefore be used on almost ALL AVRs), but this
 * is wide outside the spec for the OSCCAL value and the required precision for
 * the 12 MHz clock! Use the RC oscillator calibrated to 12 MHz for
 * experimental purposes only!
 */
static void calibrateOscillator(void)
{
uchar       step = 128;
uchar       trialValue = 0, optimumValue;
int         x, optimumDev, targetValue = (unsigned)(1499 * (double)F_CPU / 10.5e6 + 0.5);

    /* do a binary search: */
    do{
        OSCCAL = trialValue + step;
        x = usbMeasureFrameLength();    /* proportional to current real frequency */
        if(x < targetValue)             /* frequency still too low */
            trialValue += step;
        step >>= 1;
    }while(step > 0);
    /* We have a precision of +/- 1 for optimum OSCCAL here */
    /* now do a neighborhood search for optimum value */
    optimumValue = trialValue;
    optimumDev = x; /* this is certainly far away from optimum */
    for(OSCCAL = trialValue - 1; OSCCAL <= trialValue + 1; OSCCAL++){
        x = usbMeasureFrameLength() - targetValue;
        if(x < 0)
            x = -x;
        if(x < optimumDev){
            optimumDev = x;
            optimumValue = OSCCAL;
        }
    }
    OSCCAL = optimumValue;
}
/*
Note: This calibration algorithm may try OSCCAL values of up to 192 even if
the optimum value is far below 192. It may therefore exceed the allowed clock
frequency of the CPU in low voltage designs!
You may replace this search algorithm with any other algorithm you like if
you have additional constraints such as a maximum CPU clock.
For version 5.x RC oscillators (those with a split range of 2x128 steps, e.g.
ATTiny25, ATTiny45, ATTiny85), it may be useful to search for the optimum in
both regions.
*/

void    usbEventResetReady(void)
{
    calibrateOscillator();
    eeprom_write_byte(0, OSCCAL);   /* store the calibrated value in EEPROM */
}

/* ------------------------------------------------------------------------- */
/* --------------------------------- main ---------------------------------- */
/* ------------------------------------------------------------------------- */

int main(void)
{
uchar   i;
uchar   calibrationValue;

    calibrationValue = eeprom_read_byte(0); /* calibration value from last time */
    if(calibrationValue != 0xff){
        OSCCAL = calibrationValue;
    }
    odDebugInit();
    usbDeviceDisconnect();
    for(i=0;i<20;i++){  /* 300 ms disconnect */
        _delay_ms(15);
    }
    usbDeviceConnect();
    DDRB |= 1 << BIT_LED;   /* output for LED */
    //PORTB |= 1 << BIT_KEY;  /* pull-up on key input */
    wdt_enable(WDTO_1S);
    //timerInit();
    adcInit();
    usbInit();
    sei();
	setIsRecording(1);
    for(;;){    /* main event loop */
        wdt_reset();
        usbPoll();
        if(usbInterruptIsReady() ){ /* we can send a report */
            buildReport();
            usbSetInterrupt(reportBuffer, sizeof(reportBuffer));

        }
        //timerPoll();
        adcPoll();
    }
    return 0;
}
