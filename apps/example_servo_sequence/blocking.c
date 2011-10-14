/** This file contains "responsible" blocking functions.
These functions may take may take an indefinitely long time to execute, but
during that time they will call frequentTasks() in a loop so that the Wixel
can maintain its various responsibilities.  If we decide we like this pattern,
then the functions in this file might eventually be moved into the
appropriate Wixel libraries. */

#include <cc2511_map.h>
#include <wixel.h>
#include <usb.h>
#include <usb_com.h>
#include <servo.h>

#include "blocking.h"

// This is a slightly inaccurate delay function.
// It will delay by at LEAST the number of milliseconds specified.
// It will delay by at MOST
//   milliseconds + 1 + (maximum time that frequentTasks() takes).
void waitMs(uint32 milliseconds)
{
     uint32 start;
     
     if (milliseconds == 0){ return; }
     
     start = getMs();
     while(getMs() - start <= milliseconds){ frequentTasks(); }
}

void servosWaitWhileMoving()
{
    while(servosMoving()){ frequentTasks(); }
}

uint8 usbComRxReceiveByteBlocking()
{
    while(usbComRxAvailable() == 0){ frequentTasks(); }
    return usbComRxReceiveByte();
}