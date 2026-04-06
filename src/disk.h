#ifndef DISK_H
#define DISK_H

#include "base.h"
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
typedef enum { SF_SectorRead    = 0x01, // Does the descriptor contain any usable info?
               SF_WeakContents   = 0x02, // Does the block data change during reads?
               SF_Busy           = 0x04, // Are we busy updating the descriptor's info?
               SF_Allocated      = 0x08, // Marked as occuped in BAM
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
    char             disk_name[16];              // padded with 160 (0xa0, shift-space)
    ubyte            pad1[2];                    // zero to terminate disk name;
    char             disk_id[2];                 // disk ID
    ubyte            pad2[2];                    // zero to terminate disk name;
} DiskDescriptor;

// The indexing starts with 0, unlike tracks and sectors
typedef u16 DiskSectorIndex;


/////////////////////////////////////////////////////////////////////////////////
// BAM

typedef struct
{
    ubyte allocation_map;
    ubyte allocation_bits_0_to_7;
    ubyte allocation_bits_8_to_15;
    ubyte allocation_bits_16_to_20;
} TrackBAM;

typedef struct
{
    ubyte     directory_tracNk;           // expect:0x12
    ubyte     directory_sectoIdxr;        // expect:0x01
    ubyte     format_indicator;           // expect: 0x41, "A"
    ubyte     double_sided;               // no=0x00, yes=0x80
    TrackBAM  track_bam[35];
    char      disk_name[16];              // padded with 160 (0xa0, shift-space)
    ubyte     pad1[2];                   // 160 (0xa0, shift-space)
    char      disk_id[2];
    ubyte     pad2[1];                   // 160 (0xa0, shift-space)
    ubyte     dos_version;                // 2 = CBM DOS 2.6
    ubyte     format_indicator_copy;       // expect: "A" for 4040 format 
    ubyte     floppy_mode[3];             // 0x00==1541, 0xa0==1571
    ubyte     reserved1[41];
    ubyte     blocks1571[35];             // unused on 1541
} BAM;


/////////////////////////////////////////////////////////////////////////////////
// Functions

ubyte            numSectorsInTrackNr(TrackNr trackNr);
DiskSectorIndex  trackAndSectorToDiskSectorIndex(TrackNr track_nr, TrackSectorIndex sector_nr);
TrackNr          diskSectorIndexToTrackNr(DiskSectorIndex disk_sector_index);
TrackSectorIndex diskSectorIndexToSectorNr(DiskSectorIndex disk_sector_index);
void             clearDiskDescriptor(DiskDescriptor * const disk_descriptor);
void             addBAMToDescriptor(BAM const * bam, DiskDescriptor * const diskDescriptor);

#endif // DISK_H
