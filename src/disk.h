#ifndef DISK_H
#define DISK_H

#include "base.h"
#include "bam.h"
#include "errors.h"

/* Convention:

   Commodore decided to number tracks starting with 1, but sectors starting with 0.
   So the first track is track 1, and the first sector on a track is sector 0.

   For arrays and other internal data structure, it is beneficial to
   start with 0.
*/

/////////////////////////////////////////////////////////////////////////////////
// Block-related definitions
#define BLOCK_SIZE 256
typedef struct { ubyte data[BLOCK_SIZE]; } BlockData;

// Computes the checksum byte of a block
ubyte calculateBlockChecksum(BlockData const * const block_data);


/////////////////////////////////////////////////////////////////////////////////
// Sector-related definitions
typedef enum { SF_Initialized    = 0x01, // Does the descriptor contain any usable info?
               SF_WeakContents   = 0x02, // Does the block data change during reads?
               SF_Busy           = 0x04, // Are we busy updating the descriptor's info?
              } SectorFlags;

typedef struct {
    ubyte                   flags;
    DOSErrorCode            latest_dos_error;
    ubyte                   checksum;
} SectorDescriptor;


/////////////////////////////////////////////////////////////////////////////////
// Track-related definitions

// These start with 1
typedef ubyte TrackNr;
typedef ubyte TrackSectorIndex; // The index of the sector within the track, starting with 1


/////////////////////////////////////////////////////////////////////////////////
// Disk-related definitions

#define TRACKS_PER_DISK 36
#define SECTORS_PER_DISK 683

typedef struct
{
    SectorDescriptor descriptor[SECTORS_PER_DISK];
} DiskDescriptor;

// The indexing starts with 0, unlike tracks and sectors
typedef u16 DiskSectorIndex;


/////////////////////////////////////////////////////////////////////////////////
// Functions

ubyte            numSectorsInTrackNr(TrackNr trackNr);
DiskSectorIndex  trackAndSectorToDiskSectorIndex(TrackNr track_nr, TrackSectorIndex sector_nr);
TrackNr          diskSectorIndexToTrackNr(DiskSectorIndex disk_sector_index);
TrackSectorIndex diskSectorIndexToSectorNr(DiskSectorIndex disk_sector_index);
void             clearDiskDescriptor(DiskDescriptor * const disk_descriptor);

#endif // DISK_H
