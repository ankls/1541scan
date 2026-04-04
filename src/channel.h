#ifndef CHANNEL_H
#define CHANNEL_H

#include "base.h"

typedef struct
{
    ubyte drive_nr; // 8 for 1st drive, 9 for 2nd drive
    ubyte drive_channel;
    ubyte logical_file_number;
    const char * file_name;
} Channel;

extern Channel   g_channel_data;
extern Channel   g_channel_command;

void initChannels();

#endif // CHANNEL_H
