#include <8051.h>

#define True 1
#define False 0

__bit __at (0x97) LED;
__bit t0_f;
__bit rec_f;
__bit send_f;

__xdata unsigned char *portB = (__xdata unsigned char)