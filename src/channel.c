#include "channel.h"

Channel   g_channel_data;
Channel   g_channel_command;

void initChannels()
{
    static char DIRECT_ACCESS_FILE[2];
    DIRECT_ACCESS_FILE[0] = 35; // '#'
    DIRECT_ACCESS_FILE[1] = 0x00; // null terminator

    g_channel_command.drive_nr = 8;
    g_channel_command.drive_channel = 15;
    g_channel_command.logical_file_number = 1;
    g_channel_command.file_name = "";

    g_channel_data.drive_nr = 8;
    g_channel_data.drive_channel = 2;
    g_channel_data.logical_file_number = 2;
    g_channel_data.file_name = &DIRECT_ACCESS_FILE[0];
}