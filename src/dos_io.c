#include "dos_io.h"
#include "kernal_io.h"
#include "opencbm_io.h"
#include "channel.h"

#include <stddef.h>
#include <string.h>
#include <stdio.h>

// We can select the backend for I/O

//#define SEND_TO_DRIVE opencbmio_sendToDrive
#define SEND_TO_DRIVE kio_sendToDrive
//#define READ_FROM_DRIVE opencbmio_readFromDrive
#define READ_FROM_DRIVE kio_readFromDrive

// g_ prefix for global data
BlockData g_block_buffer;
Command   g_command_buffer;
Status    g_status_buffer;

bool sendResetDrive()
{
    u16 bytes_written;

#if PRINT_FUNCTION_NAMES
    printf(__func__ "()\n");
#endif
    g_command_buffer.data[0] = 85; // PETSCII 'u'
    g_command_buffer.data[1] = 73; // PETSCII 'i'
    g_command_buffer.data[2] = 48; // PETSCII '0'
    return (  SEND_TO_DRIVE(&g_channel_data, &(g_command_buffer.data[0]), 3, &bytes_written)
           && (bytes_written == 3))
           ? true : false;
}

bool sendInitializeDrive()
{
    u16 bytes_written;

#if PRINT_FUNCTION_NAMES
    printf(__func__ "()\n");
#endif
    g_command_buffer.data[0] = 73; // PETSCII 'i'
    g_command_buffer.data[1] = 48; // PETSCII '0'
    return (  SEND_TO_DRIVE(&g_channel_data, &(g_command_buffer.data[0]), 2, &bytes_written)
           && (bytes_written == 2))
           ? true : false;
}

// Read a block into floppy's memory at address 0x0300 using U1 command
bool sendDirectCommandU1(TrackNr track_nr, TrackSectorIndex sector_idx)
{
    u16 bytes_written;

#if PRINT_FUNCTION_NAMES
    printf(__func__ "()\n");
#endif

    g_command_buffer.data[0] =  85; // PETSCII 'u'
    g_command_buffer.data[1] =  49; // PETSCII '1'
    g_command_buffer.data[2] =  32; // PETSCII ' '
    g_command_buffer.data[3] =  48 + g_channel_data.drive_channel; // PETSCII secondary channel
    g_command_buffer.data[4] =  32; // PETSCII ' '
    g_command_buffer.data[5] =  48; // PETSCII '0' Drive number in logical unit, starting with 0. 1541 has only one disk drive in its unit, hence always 0.
    g_command_buffer.data[6] =  32; // PETSCII ' '
    g_command_buffer.data[7] =  48 + (track_nr / 10); // PETSCII '0' + (track div 10)
    g_command_buffer.data[8] =  48 + (track_nr % 10); // PETSCII '0' + (track mod 10)
    g_command_buffer.data[9] =  32; // PETSCII ' '
    g_command_buffer.data[10] =  48 + (sector_idx / 10); // PETSCII '0' + (sector div 10)
    g_command_buffer.data[11] =  48 + (sector_idx % 10);// PETSCII '0' + (sector mod 10)

    return (SEND_TO_DRIVE(&g_channel_command, &(g_command_buffer.data[0]), 12, &bytes_written)
           && (bytes_written == 12))
           ? true : false;
}

// Position the read-pointer in floppy's memory to a desired offset
bool sendDirectCommandBP(ubyte offset)
{
    u16 bytes_written;

#if PRINT_FUNCTION_NAMES
    printf(__func__ "()\n");
#endif

    g_command_buffer.data[0] = 66; // PETSCII 'b'
    g_command_buffer.data[1] = 45; // PETSCII '-'
    g_command_buffer.data[2] = 80; // PETSCII 'p'
    g_command_buffer.data[3] = 58; // PETSCII ':'
    g_command_buffer.data[4] = 48 + g_channel_data.drive_channel; // PETSCII '0' + drive channel
    g_command_buffer.data[5] = offset; // Docs unclear: PETSCII or binary

    return (SEND_TO_DRIVE(&g_channel_command, &(g_command_buffer.data[0]), 6, &bytes_written)
           && (bytes_written == 6))
           ? true : false;
}

bool sendDirectCommandMR(u16 floppy_memory_address, u16 len)
{
    u16 bytes_written;

#if PRINT_FUNCTION_NAMES
    printf(__func__ "()\n");
#endif

    g_command_buffer.data[0] = 77; // PETSCII 'm'
    g_command_buffer.data[1] = 45; // PETSCII '-'
    g_command_buffer.data[2] = 82; // PETSCII 'r'
    g_command_buffer.data[3] = floppy_memory_address % 0xff; // low address byte
    g_command_buffer.data[4] = floppy_memory_address >> 8;   // high address byte
    g_command_buffer.data[5] = len;                          // amount of bytes

    return (SEND_TO_DRIVE(&g_channel_command, &(g_command_buffer.data[0]), 6, &bytes_written)
           && (bytes_written == 6))
           ? true : false;
}

bool readFromDrive(ubyte * const buffer_address, u16 const buffer_len, u16 * const bytes_read)
{
    return (DOS_EC_OK == READ_FROM_DRIVE(&g_channel_data, buffer_address, buffer_len, bytes_read) ? true : false);
}

DOS_ERROR_CODE readDriveErrorCode()
{
    DOS_ERROR_CODE dosec;
    u16 bytes_read;

    g_status_buffer.data[0] = '\0'; // To kill previous contents
    g_status_buffer.data[1] = '\0'; // To kill previous contents

    READ_FROM_DRIVE(&g_channel_command, &(g_status_buffer.data[0]), sizeof(g_status_buffer.data), &bytes_read);

    dosec = (g_status_buffer.data[0] - '0') * 10;    // Convert ASCII digit to number
    dosec = dosec + (g_status_buffer.data[1] - '0'); // Convert second ASCII digit and add to the first

    return dosec;
}
