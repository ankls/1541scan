#include "display_aux.h"
#include "errors.h"
#include "dos_io.h" // for g_block_buffer
#include "keyboard.h"

#include <c64.h>
#include <conio.h>
#include <string.h>
#include <peekpoke.h>
#include <stdio.h>

void sectorDescriptorToCharAndColor(SectorDescriptor const * const sector_descriptor, char * c, ubyte * color)
{
    if (sector_descriptor->flags & SF_Busy)
    {
        *color = COLOR_WHITE;
        *c = 105;
    }
    else if ((sector_descriptor->flags & SF_SectorRead) == 0x00)
    {
        *color = COLOR_GRAY1;
        *c = '?';
    }
    else
    {
        if (sector_descriptor->flags & SF_WeakContents)
        {
            // Sector has different contents in each read, thus is "weak"
            *c = 'W';
        }
        else
        {
            bool allocated;
            bool part_of_file;
            
            dosErrorCharAndColor(sector_descriptor->latest_dos_error, c, color);

            if (DOS_EC_OK == sector_descriptor->latest_dos_error)
            {
                allocated    = (0 != (sector_descriptor->flags & SF_Allocated)) ? true : false;
                part_of_file = (0 != (sector_descriptor->flags & SF_File)) ?      true : false;

                if (allocated != part_of_file)
                { *color = COLOR_YELLOW; }
            }
        }
    }
    
    if ((sector_descriptor->flags & SF_Allocated) == 0x00)
    {
        // sector is free
        // clear high bit
        // to get the screen code from inverted to non-inverted.
        *c &= 0x7f;
    }
    else
    {
        // sector allocated
        // we set the high bit
        // make it inverted
        *c |= 0x80;
    }
}

void dosErrorCharAndColor(const DOS_ERROR_CODE dec, char * c, ubyte * front)
{
    // front colors
    // light green: All fine
    // light red: Some reading failed (actually something with the disk)
    // lighty yellow: Something with the reading process (disk ID, BAM, drive, ...)
    // purple: Syntax stuff, file stuff, writing stuff (should never happen), misc

    switch(dec)
    {
        case DOS_EC_OK:                             *c = 250; *front = COLOR_GREEN;      break; // 250 = checkmark
        case DOS_EC_FILES_SCRATCHED:                *c = 'R'; *front = COLOR_PURPLE;     break; 
        case DOS_EC_READ_ERROR_20:                  *c = 'D'; *front = COLOR_LIGHTRED;   break; // read error (data block header not found)
        case DOS_EC_READ_ERROR_21:                  *c = 'S'; *front = COLOR_LIGHTRED;   break; // read error (SYNC not found)
        case DOS_EC_READ_ERROR_22:                  *c = 'H'; *front = COLOR_LIGHTRED;   break; // read error (checksum in data block header)
        case DOS_EC_READ_ERROR_23:                  *c = 'B'; *front = COLOR_LIGHTRED;   break; // read error (checksum in data block body)
        case DOS_EC_READ_ERROR_24:                  *c = 'C'; *front = COLOR_LIGHTRED;   break; // read error (checksum)
        case DOS_EC_WRITE_ERROR_DATA:               *c = 'W'; *front = COLOR_PURPLE;     break; // write error (data block)
        case DOS_EC_WRITE_PROTECT_ON:               *c = 'P'; *front = COLOR_PURPLE;     break; // disk write protected
        case DOS_EC_READ_ERROR_27:                  *c = 'R'; *front = COLOR_LIGHTRED;   break; // read error
        case DOS_EC_WRITE_ERROR_SYNC:               *c = 'S'; *front = COLOR_PURPLE;     break; // write error (SYNC not found)
        case DOS_EC_ID_MISMATCH:                    *c = 'I'; *front = COLOR_YELLOW;     break; // disk ID mismatch
        case DOS_EC_SYNTAX_ERROR_MALFORMED:         *c = 'M'; *front = COLOR_PURPLE;     break; // syntax error (malformed command)
        case DOS_EC_SYNTAX_ERROR_UNKNOWN_COMMAND:   *c = 'C'; *front = COLOR_PURPLE;     break; // syntax error (unknown command)
        case DOS_EC_SYNTAX_ERRPR_COMMAND_TOO_LONG:  *c = 'L'; *front = COLOR_PURPLE;     break; // syntax error (command too long)
        case DOS_EC_SYNTAX_ERROR_ILLEGAL_JOKER:     *c = 'J'; *front = COLOR_PURPLE;     break; // syntax error (illegal joker)
        case DOS_EC_SYNTAX_ERROR_NO_FILENAME:       *c = 'N'; *front = COLOR_PURPLE;     break; // syntax error (filename not found)
        case DOS_EC_FILE_NOT_FOUND_USR:             *c = 'U'; *front = COLOR_PURPLE;     break; // file not found (USR)
        case DOS_EC_RECORD_NOT_PRESENT:             *c = 'P'; *front = COLOR_PURPLE;     break; // record not present
        case DOS_EC_OVERFLOW_IN_RECORD:             *c = 'Z'; *front = COLOR_PURPLE;     break; // overflow in record
        case DOS_EC_FILE_TOO_LARGE:                 *c = 'G'; *front = COLOR_PURPLE;     break; // file too large
        case DOS_EC_WRITE_FILE_OPEN:                *c = 'O'; *front = COLOR_PURPLE;     break; // write file open (not closed)
        case DOS_EC_FILE_NOT_OPEN:                  *c = 'X'; *front = COLOR_PURPLE;     break; // file not open
        case DOS_EC_FILE_NOT_FOUND:                 *c = 'Y'; *front = COLOR_PURPLE;     break; // file not found
        case DOS_EC_FILE_EXISTS:                    *c = 'E'; *front = COLOR_PURPLE;     break; // file exists
        case DOS_EC_FILE_TYPE_MISMATCH:             *c = 'T'; *front = COLOR_PURPLE;     break; // file type mismatch
        case DOS_EC_NO_BLOCK:                       *c = 'B'; *front = COLOR_YELLOW;     break; // no block (already occupied)
        case DOS_EC_ILLEGAL_TOS_DISK:               *c = 'D'; *front = COLOR_YELLOW;     break; // illegal track or sector (disk)
        case DOS_EC_ILLEGAL_TOS_FILE:               *c = 'F'; *front = COLOR_YELLOW;     break; // illegal track or sector (file)
        case DOS_EC_NO_CHANNEL:                     *c = 'A'; *front = COLOR_PURPLE;     break; // no channel
        case DOS_EC_DIR_ERROR:                      *c = 'D'; *front = COLOR_YELLOW;     break; // dir error (DOS vs BAM mismatch)
        case DOS_EC_DISK_FULL:                      *c = 'F'; *front = COLOR_PURPLE;     break; // disk full
        case DOS_EC_VERSION:                        *c = 'V'; *front = COLOR_PURPLE;     break; // CBM DOS version report or mismatch
        case DOS_EC_DRIVE_NOT_READY:                *c = 'D'; *front = COLOR_PURPLE;     break; // drive not ready
        default:                                    *c = 'R'; *front = COLOR_YELLOW;     break; // undefined DOS code
    }
}

void clearRow(ubyte row)
{
    u16 i;
    for (i = 40*row; i < (40*row+40); ++i)
    {
        POKE(0x400 + i, ' '); // Print a space at each position on the screen using direct memory access
        POKE(COLOR_RAM + i, COLOR_LIGHTBLUE);
    }
}

void clearScreen()
{
    // Clear the screen by printing 25 lines of spaces.
    u16 i;
    for (i = 0; i < (40*25); ++i)
    {
        POKE(0x400 + i, ' '); // Print a space at each position on the screen using direct memory access
        POKE(COLOR_RAM + i, COLOR_LIGHTBLUE);
    }
}


//////////////////////////////////////////////////////////////////////////
// Screen layout "block hex display"

void displayBlockData(BlockData const * const block_data)
{
    enum { X_OFFSET = 3, Y_OFFSET = 2 };

    ubyte value;
    ubyte row;
    ubyte column;

    unsigned idx;

    char hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

    clearScreen();
    gotoxy(0,0);
    printf("   .0.1.2.3.4.5.6.7.8.9.a.b.c.d.e.f\n");
    printf("\n");
    printf("0.\n");
    printf("1.\n");
    printf("2.\n");
    printf("3.\n");
    printf("4.\n");
    printf("5.\n");
    printf("6.\n");
    printf("7.\n");
    printf("8.\n");
    printf("9.\n");
    printf("a.\n");
    printf("b.\n");
    printf("c.\n");
    printf("d.\n");
    printf("e.\n");
    printf("f.\n");

    // 16 bytes per line displayed
    for (row = 0; row < (BLOCK_SIZE >> 4); ++row)
    {
        gotoxy(X_OFFSET, Y_OFFSET + row);
        for (column = 0; column < 16; ++column)
        {
            idx = row;
            idx <<= 4;
            idx += column;
            value = block_data->data[idx];
            printf("%c%c", hex[value >> 4], hex[value & 0x0f]);
        }
    }
}


//////////////////////////////////////////////////////////////////////////
// Screen layout "disk block health map"

void displaySectorDescriptor(TrackNr const track_nr, TrackSectorIndex const sector_idx, SectorDescriptor const * const sd)
{
    char c;
    ubyte color;

    u16 offset;
    ubyte x_offset;
    ubyte y_offset;

    x_offset = 2;
    y_offset = 2;
    offset = (sector_idx + y_offset) * 40 + (track_nr - 1 + x_offset);
    
    sectorDescriptorToCharAndColor(sd, &c, &color);

    // Print the character at the correct position on the screen using direct memory access
    POKE(0x400 + offset, c);
    POKE(0xD800 + offset, color);
}

void displayTrackAndSectorRulers()
{
    ubyte i;

    gotoxy(0,0);
    printf("  Track    11111111112222222222333333");
    gotoxy(0,1);
    printf("  12345678901234567890123456789012345");
    for (i = 0; i < numSectorsInTrackNr(1); ++i)
    {
        gotoxy(0,2+i);
        printf("%2d\n", i); // Print the track number at the beginning of each line
    }
    gotoxy(0,2); printf("S");
    gotoxy(0,3); printf("e");
    gotoxy(0,4); printf("c");
    gotoxy(0,5); printf("t");
    gotoxy(0,6); printf("o");
    gotoxy(0,7); printf("r");
}

void displayDiskDescriptor(DiskDescriptor const * const disk_descriptor)
{
    // Display the disk in text ascii art.
    {
        // Print the tracks in Y-direction and sectors in X-direction, and the character for the sector descriptor as the content of the cell.
        ubyte x_offset;
        ubyte y_offset;

        TrackNr track_nr;
        TrackSectorIndex sector_idx;

        x_offset = 2;
        y_offset = 2;

        for (track_nr = 1; track_nr <= TRACKS_PER_DISK; ++track_nr)
        {
            for (sector_idx = 0; sector_idx < numSectorsInTrackNr(track_nr); ++sector_idx)
            {
                DiskSectorIndex disk_sector_index = trackAndSectorToDiskSectorIndex(track_nr, sector_idx);
                SectorDescriptor const * const sector_descriptor = &disk_descriptor->descriptor[disk_sector_index];
                displaySectorDescriptor(track_nr, sector_idx, sector_descriptor);
            }
        }
    }

    gotoxy(20,21); printf("Disk name");
    gotoxy(38,21); printf("ID");
    gotoxy(20,22); printf("%s",   disk_descriptor->disk_name);
    gotoxy(38,22); printf("%c%c", disk_descriptor->disk_id[0], disk_descriptor->disk_id[1]);
}

void displayStatus(const char * status)
{
    clearRow(23);
    gotoxy(0,23);
    printf("Status:%s", status);
}

void clearStatus()
{
    ubyte col;
    for (col = 0; col < 40; ++col)
    { POKE(0x400 + 40*23 + col, ' '); }
}

void displayMenu(const char * menu)
{
    clearRow(24);
    gotoxy(0,24);
    printf("%s", menu);
}

void clearMenu()
{
    ubyte col;
    for (col = 0; col < 40; ++col)
    { POKE(0x400 + 40*24 + col, ' '); }
}

