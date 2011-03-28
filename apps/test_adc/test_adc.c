#include <wixel.h>
#include <usb.h>
#include <usb_com.h>
#include <stdio.h>

// TODO: use VT100 commands to make a cool bar graph display
// TODO: add VDD readings

int32 CODE param_report_period_ms = 40;

int32 CODE param_input_mode = 0;

int32 CODE param_use_vt100 = 1;

uint8 XDATA report[1024];
uint16 DATA reportLength = 0;
uint16 DATA reportBytesSent = 0;

void updateLeds()
{
    usbShowStatusWithGreenLed();
    LED_YELLOW(0);
    LED_RED(0);
}

// This gets called by puts and printf, to populate the report buffer.
// The result is sent to USB later.
void putchar(char c)
{
    report[reportLength] = c;
    reportLength++;
}

// value should be between 0 and 63 inclusive.
void printBar(const char * name, uint16 adcResult)
{
    uint8 i, width;
    printf("%-4s %4d |", name, adcResult);
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

    if (getMs() - lastReport >= param_report_period_ms && reportLength == 0)
    {
        lastReport = getMs();
        reportBytesSent = 0;

        for(i = 0; i < 6; i++)
        {
            result[i] = adcRead(i);
        }

        if (param_use_vt100)
        {
            printf("\x1B[0;0H");  // VT100 command for "go to 0,0"
            printBar("P0_0", result[0]);
            printBar("P0_1", result[1]);
            printBar("P0_2", result[2]);
            printBar("P0_3", result[3]);
            printBar("P0_4", result[4]);
            printBar("P0_5", result[5]);
        }
        else
        {
            reportLength = sprintf(report, "%4d, %4d, %4d, %4d, %4d, %4d\r\n",
                    result[0], result[1], result[2], result[3], result[4], result[5]);
        }
    }

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
