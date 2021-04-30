#ifndef UTILS_H_
#define UTILS_H_

#include <USBAPI.h>

#define DEBUG_PRINTING

template <typename T>
void
DebugPrint(const T& str)
{
#ifdef DEBUG_PRINTING
    Serial.print(str);
#endif
}

template <typename T>
void
DebugPrintln(const T& str)
{
#ifdef DEBUG_PRINTING
    Serial.println(str);
#endif
}

#endif  // UTILS_H_
