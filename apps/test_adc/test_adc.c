/** test_adc app:

This app uses the CC2511's analog-to-digital converter (ADC) to read the
voltages on all 6 analog inputs and measure the voltage of the Wixel's VDD (3V3)
line.  The results are reported to the computer either in CSV format or
as a bar graph.


== Parameters ==

input_mode:  Specifies whether to enable internal pull-down resistors, 
    enable internal pull-up resistors, or just let the analog lines float.
     1 = Pull-ups
     0 = Float (default)
    -1 = Pull-downs

bar_graph:  Specifies whether to print out a bar graph or not.
    1 = Print a bar graph (default, requires you to use a terminal program that
        supports VT100 commands)
    0 = Print the 7 readings on a single line, separated by commas.

report_period_ms: Specifies the number of milliseconds to wait between
    reports to the computer.  The default is 40.
*/

#include <wixel.h>
#include <usb.h>
#include <usb_com.h>
#include <stdio.h>

/* PARAMETERS *****************************************************************/

int32 CODE param_input_mode = 0;
int32 CODE param_bar_graph = 1;
int32 CODE param_report_period_ms = 40;

/* VARIABLES ******************************************************************/

// A big buffer for holding a report.  This allows us to print more than
// 128 bytes at a time to USB.
uint8 XDATA report[1024];

// The length (in bytes) of the report currently in the report buffer.
// If zero, then there is no report in the buffer.
uint16 DATA reportLength = 0;

// The number of bytes of the current report that have already been
// send to the computer over USB.
uint16 DATA reportBytesSent = 0;

/* FUNCTIONS ******************************************************************/

void updateLeds()
{
    usbShowStatusWithGreenLed();
    LED_YELLOW(0);
    LED_RED(0);
}

// This gets called by puts, printf, and printBar to populate
// the report buffer.  The result is sent to USB later.
void putchar(char c)
{
    report[reportLength] = c;
    reportLength++;
}

// adcResult should be between 0 and 2047 inclusive.
void printBar(const char * name, uint16 adcResult)
{
    uint8 i, width;
    printf("%-4s %4d mV |", name, adcConvertToMillivolts(adcResult));
    width = adcResult >> 5;
    for(i = 0; i < width; i++){ putchar('#'); }
    for(; i < 63; i++){ putchar(' '); }
    putchar('|');
    putchar('\r');
    putchar('\n');
}

void sendReportIfNeeded()
{
    static uint32 lastReport;
    uint8 i, bytesToSend;
    uint16 result[6];
    uint16 vddMillivolts;

    // Create reports.
    if (getMs() - lastReport >= param_report_period_ms && reportLength == 0)
    {
        lastReport = getMs();
        reportBytesSent = 0;

        vddMillivolts = adcReadVddMillivolts();
        adcSetMillivoltCalibration(vddMillivolts);

        for(i = 0; i < 6; i++)
        {
            result[i] = adcRead(i);
        }

        if (param_bar_graph)
        {
            printf("\x1B[0;0H");  // VT100 command for "go to 0,0"
            printBar("P0_0", result[0]);
            printBar("P0_1", result[1]);
            printBar("P0_2", result[2]);
            printBar("P0_3", result[3]);
            printBar("P0_4", result[4]);
            printBar("P0_5", result[5]);
            printf("VDD  %4d mV", vddMillivolts);
        }
        else
        {
            printf("%4d, %4d, %4d, %4d, %4d, %4d, %4d\r\n",
                    adcConvertToMillivolts(result[0]),
                    adcConvertToMillivolts(result[1]),
                    adcConvertToMillivolts(result[2]),
                    adcConvertToMillivolts(result[3]),
                    adcConvertToMillivolts(result[4]),
                    adcConvertToMillivolts(result[5]),
                    vddMillivolts);
        }
    }

    // Send the report to USB in chunks.
    if (reportLength > 0)
    {
        bytesToSend = usbComTxAvailable();
        if (bytesToSend > reportLength - reportBytesSent)
        {
            // Send the last part of the report.
            usbComTxSend(report+reportBytesSent, reportLength - reportBytesSent);
            reportLength = 0;
        }
        else
        {
            usbComTxSend(report+reportBytesSent, bytesToSend);
            reportBytesSent += bytesToSend;
        }
    }

}

void analogInputsInit()
{
    switch(param_input_mode)
    {
    case 1: // Enable pull-up resistors for all pins on Port 0.
        // This shouldn't be necessary because the pull-ups are enabled by default.
        P2INP &= ~(1<<5);  // PDUP0 = 0: Pull-ups on Port 0.
        P0INP = 0;
        break;

    case -1: // Enable pull-down resistors for all pins on Port 0.
        P2INP |= (1<<5);   // PDUP0 = 1: Pull-downs on Port 0.
        P0INP = 0;         // This line should not be necessary because P0SEL is 0 on reset.
        break;

    default: // Disable pull-ups and pull-downs for all pins on Port 0.
        P0INP = 0x3F;
        break;
    }
}

void main()
{
    systemInit();
    usbInit();
    analogInputsInit();

    while(1)
    {
        boardService();
        updateLeds();
        usbComService();
        sendReportIfNeeded();
    }
}
