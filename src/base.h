#ifndef BASE_H
#define BASE_H

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#endif

typedef unsigned char ubyte;
typedef unsigned int  u16;

#ifndef bool
#define bool ubyte
#define false 0
#define true 1
#endif

#define PRINT_FUNCTION_NAMES false
#define PRINT_IO false

#endif // BASE_H
