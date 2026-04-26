#include "kernal_io.h"

#include <string.h>
#include <stdio.h>

// Auxilliary variables for inline assembly, to make variable data accessible via global symbols, so %v can be used.
// Place these in the zero page so that indirect addressing modes like
// "(%v,x)" or "(%v),y" are legal.  cc65 requires the pointer to reside
// in the zero page for those modes.
static ubyte g_logical_file_nr;
static ubyte g_drive_channel;
static ubyte g_file_name_ptr_low_byte;
static ubyte g_file_name_ptr_high_byte;
static ubyte g_file_name_len;
static ubyte g_kernal_status_code;
static ubyte g_data_byte;
static ubyte g_drive_nr;

inline void KERNAL_SETLFS(ubyte drive_nr, ubyte logical_file_number, ubyte drive_channel)
{
    g_logical_file_nr = logical_file_number;
    g_drive_channel   = drive_channel;
    g_drive_nr        = drive_nr;
    __asm__ ("lda %v", g_logical_file_nr);
    __asm__ ("ldx %v", g_drive_nr);
    __asm__ ("ldy %v", g_drive_channel);
    __asm__ ("jsr $ffba"); // kernal SETLFS
}

inline void KERNAL_SETNAM(const char * file_name)
{
    g_file_name_ptr_low_byte  = (ubyte)((u16)file_name);         // low byte of file name pointer
    g_file_name_ptr_high_byte = (ubyte)(((u16)file_name) >> 8);  // high byte of file name pointer
    g_file_name_len           = strlen(file_name);               // file name length

    __asm__ ("lda %v", g_file_name_len);           // A = file name length
    __asm__ ("ldx %v", g_file_name_ptr_low_byte);  // X = low file name address byte
    __asm__ ("ldy %v", g_file_name_ptr_high_byte); // Y = high file name address byte
    __asm__ ("jsr $ffbd");                         // kernal SETNAM
}

inline KERNAL_ST_SERIAL_STATUS KERNAL_OPEN()
{
    g_kernal_status_code = KERNAL_ST_SERIAL_OK; // default: all fine
    __asm__ ("jsr $ffc0");                      // kernal OPEN
    __asm__ ("bcc %g", success);
    __asm__ ("sta %v", g_kernal_status_code);   // save error
    success:
    return g_kernal_status_code;
}

inline KERNAL_ST_SERIAL_STATUS KERNAL_READST()
{
    g_kernal_status_code = KERNAL_ST_SERIAL_OK; // default: all fine

    __asm__ ("jsr $ffb7"); // kernal READST
    __asm__ ("sta %v", g_kernal_status_code);

    return g_kernal_status_code;
}

inline void KERNAL_CLOSE(ubyte logical_file_number)
{
    g_logical_file_nr = logical_file_number;
    
    __asm__ ("lda %v", g_logical_file_nr);
    __asm__ ("jsr $ffc3");                 // kernal CLOSE
}

inline void KERNAL_CHKOUT(ubyte logical_file_number)
{
    g_logical_file_nr = logical_file_number;

    // Select channel for output
    __asm__ ("ldx %v", g_logical_file_nr);
    __asm__ ("jsr $ffc9");                 // kernal CHKOUT
}

inline void KERNAL_CHKIN(ubyte logical_file_number)
{
    g_logical_file_nr = logical_file_number;

    // Select channel for input
    __asm__ ("ldx %v", g_logical_file_nr);
    __asm__ ("jsr $ffc6");                 // kernal CHKIN
}

inline bool KERNAL_CHROUT(ubyte data)
{
    g_data_byte = data;
    g_kernal_status_code = 0;                 // this is not an official KERNAL status code, but we use it to track if the CHROUT call succeeded or not, since CHROUT itself does not return any status code. We will set it to 1 if CHROUT fails, and leave it at 0 if it succeeds.
    __asm__ ("lda %v", g_data_byte);          // data byte to send
    __asm__ ("jsr $ffd2");                    // kernal CHROUT
    __asm__ ("bcc %g", success);
    __asm__ ("lda #1");                       // flag this operation as failed
    __asm__ ("sta %v", g_kernal_status_code);
    success:

    return ((g_kernal_status_code == 0)? true : false);
}

inline void KERNAL_CHRIN(ubyte * data)
{
    __asm__ ("jsr $ffcf");           // kernal CHRIN (aka BASIN)
    __asm__ ("sta %v", g_data_byte); // save the read byte
    
    *data = g_data_byte;
}

inline void KERNAL_CLRCHN()
{
    __asm__ ("jsr $ffcc");            // kernal CLRCHN
}

inline void MUTE_INTERRUPTS()
{
    __asm__ ("sei");
}

inline void UNMUTE_INTERRUPTS()
{
    __asm__ ("cli");
}

inline void KERNAL_GETIN(char * data)
{
    loop:
    __asm__ ("jsr $ffe4");           // kernal GETIN
    __asm__ ("cmp #0");
    __asm__ ("beq %g", loop);
    __asm__ ("sta %v", g_data_byte); // save the read byte
    
    *data = g_data_byte;
}

char kio_getInput()
{
    char c;
    KERNAL_GETIN(&c);
    return c;
}

KERNAL_ST_SERIAL_STATUS kio_openChannel(Channel const * const channel)
{
#if PRINT_FUNCTION_NAMES
    printf(__func__ "(channel lfn=%d)\n", channel->logical_file_number);
#endif

    (void) KERNAL_SETLFS(channel->drive_nr, channel->logical_file_number, channel->drive_channel);
    (void) KERNAL_SETNAM(channel->file_name);
    return KERNAL_OPEN();
}

void kio_closeChannel(Channel const * const channel)
{
#if PRINT_FUNCTION_NAMES
    printf(__func__ "(channel lfn=%d)\n", channel->logical_file_number);
#endif

    (void) KERNAL_CLOSE(channel->logical_file_number);
}

bool kio_sendToDrive(Channel const * const channel, ubyte const * const data, const u16 length, u16 * const bytes_written)
{
#if PRINT_FUNCTION_NAMES
    printf(__func__ "(channel lfn %d, %d bytes)\n", channel->logical_file_number, length);
#endif

#if PRINT_IO
     {
        u16 idx;
        printf("TX to lfn %d:", channel->drive_channel);
        for (idx = 0; idx < length; ++idx)
        {
            printf(" 0x%02x", data[idx]);
        }
        printf("\n");
    }
#endif

    {
        u16 idx;
        bool ok;

        ok = true;

        (void) KERNAL_CHKOUT(channel->logical_file_number); // Select channel for output
        for (idx = 0; (idx < length) && (true == ok); ++idx)
        {
            ok = KERNAL_CHROUT(data[idx]); // byte to send
            if (true != ok)
            {
                break;
            }

            /* After sending a byte, poll READST to detect timeouts or errors.
               If a timeout or other error is detected, abort the transfer. */
            {
                KERNAL_ST_SERIAL_STATUS st;
                ubyte poll_count = 0;
                do
                {
                    st = KERNAL_READST();
                    if ((st & READST_SERIAL_TIMEOUT_FLAG) != 0)
                    {
                        ok = false;
                        break;
                    }
                    ++poll_count;
                } while ((st != KERNAL_ST_SERIAL_OK) && (poll_count < 200));

                if (st != KERNAL_ST_SERIAL_OK)
                {
                    ok = false;
                    break;
                }
            }
        }
        (void) KERNAL_CLRCHN();

        *bytes_written = idx;
        return ok;
     }
}

KERNAL_ST_SERIAL_STATUS kio_readFromDrive(Channel const * const channel, ubyte * const data, const u16 buffer_length, u16 * const bytes_read)
{
    KERNAL_ST_SERIAL_STATUS st;
    u16 idx;
    (void) KERNAL_CLRCHN(); // Clear channel first to be sure we're in a clean state, and that the drive is ready to send data. This also has the effect of resetting the drive's internal state machine for the next read operation, which is important to ensure that the drive sends the expected data in response to our read requests.

    (void) KERNAL_CHKIN(channel->logical_file_number); // Select channel for input
    st = KERNAL_READST();

    for (idx = 0; (idx < buffer_length); ++idx)
    {
        // Be aware that we destroy one char in the buffer, regardless we succeed reading or not
        KERNAL_CHRIN(&(data[idx]));

        st = KERNAL_READST();

        // Handle read error
        // curious: cbm_read uses "if ((st & 0xbf) != 0)" instead
        if ((st & 0x40) != 0)
        {
            // end of data reached
            st = st & 0xbf; // mask out the end of data flag to get the actual error code
            break; // stop reading
        }
    }

    *bytes_read = (DOS_EC_OK == st) ? (idx+1) : 0;

    (void) KERNAL_CLRCHN();

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

    return st;
}

