#include "opencbm_io.h"

#include <cbm.h>
#include <stdio.h>
#include <errno.h>

KERNAL_ST_SERIAL_STATUS opencbmio_openChannel(Channel const * const channel)
{
#if PRINT_FUNCTION_NAMES
    printf(__func__ "(channel lfn=%d)\n", channel->logical_file_number);
#endif
    // Try to open the command channel
    return cbm_open(channel->logical_file_number, channel->drive_nr, channel->drive_channel, channel->file_name);
}

void opencbmio_closeChannel(Channel const * const channel)
{
#if PRINT_FUNCTION_NAMES
    printf(__func__ "(channel lfn=%d)\n", channel->logical_file_number);
#endif
    cbm_close(channel->logical_file_number);
}

bool opencbmio_sendToDrive(Channel const  * const channel, ubyte const * const data, const u16 length, u16 * const bytes_written)
{
#if PRINT_FUNCTION_NAMES
    printf(__func__ "(channel lfn %d, %d bytes)\n", channel->logical_file_number, length);
#endif

    // Send command
    *bytes_written = cbm_write(channel->logical_file_number, data, length);

#if PRINT_IO
     {
        ubyte idx;
        printf("TX to lfn %d:", channel->drive_channel);
        for (idx = 0; idx < *bytes_written; ++idx)
        {
            printf(" 0x%02x", data[idx]);
        }
        printf("\n");
    }
#endif

    return (length == *bytes_written) ? true : false;
}

KERNAL_ST_SERIAL_STATUS opencbmio_readFromDrive(Channel const * const channel, ubyte * const data, const u16 buffer_length, u16 * const bytes_read)
{
   int rc;
#if PRINT_FUNCTION_NAMES
    printf(__func__ "(%d, 0x%04x, %d, *bytes)=", *channel, data, buffer_length);
#endif

     // Send command
    rc = cbm_read(channel->logical_file_number, data, buffer_length);

#if PRINT_IO
    printf("%d", rc);
#endif

    if (rc >= 0)
    {
        *bytes_read = rc;
#if PRINT_IO
        {
            u16 idx;
            printf("RX from lfn %d:", channel->drive_channel);
            for (idx = 0; idx < *bytes_read; ++idx)
            {
                printf(" 0x%02x", data[idx]);
            }
            printf("\n");
        }
#endif
        return KERNAL_ST_SERIAL_OK;
    }
    else
    {
        return __oserror;
    }
}

