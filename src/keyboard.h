#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "base.h"

char  keyb_readChar_nonblocking();
char  keyb_readChar_blocking();
bool  keyb_userHoldsAbortKey();
void  keyb_clearBufferedChars();

void pause();

#endif // KEYBOARD_H
