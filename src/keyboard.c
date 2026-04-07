#include "keyboard.h"
#include "kernal_io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char g_keyboard_char;

char keyb_readChar_blocking()
{
    return kio_getInput();
}

char keyb_readChar_nonblocking()
{
    __asm__ ("lda $cb"); // last pressed key
    __asm__ ("sta %v", g_keyboard_char); // save the read byte
    
    return g_keyboard_char;
}

bool keyb_userHoldsAbortKey()
{
    return (0x39 == keyb_readChar_nonblocking()); // abort on left arrow char
}

void keyb_clearBufferedChars()
{
    __asm__ ("lda #0");
    __asm__ ("sta $c6");
}

void pause()
{
    printf("Press RETURN to continue...");
    getchar();
    printf("\n");
}

