#ifndef DOS_IO_H
#define DOS_IO_H

#include "base.h"
#include "errors.h"
#include "channel.h"
#include "disk.h"

#define MAX_COMMAND_LENGTH 8 // Arbitrary decision, extend if needed
#define MAX_STATUS_LENGTH 40 // Seen in other programs as a limit

typedef struct { ubyte data[MAX_COMMAND_LENGTH]; } Command;
typedef struct { ubyte data[MAX_STATUS_LENGTH]; } Status;

// g_ prefix for global data
extern BlockData g_block_buffer;
extern Command   g_command_buffer;
extern Status    g_status_buffer;


bool sendResetDrive();
bool sendInitializeDrive();
bool sendDirectCommandU1(TrackNr track_nr, TrackSectorIndex sector_nr);
bool sendDirectCommandBP(ubyte offset);
bool sendDirectCommandMR(u16 floppy_memory_address, u16 len);
bool readFromDrive(ubyte * const buffer_address, u16 const buffer_len, u16 * const bytes_read);
DOS_ERROR_CODE readDriveErrorCode();
Status const * getLastDriveStatusString();
DOS_ERROR_CODE readSector(TrackNr track_nr, TrackSectorIndex sector_idx, BlockData * const block_data);

#endif // DOS_IO_H
