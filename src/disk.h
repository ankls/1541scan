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
typedef enum { SF_SectorRead     = 0x01, // Does the descriptor contain any usable info?
               SF_WeakContents   = 0x02, // Does the block data change during reads?
               SF_Busy           = 0x04, // Are we busy updating the descriptor's info?
               SF_Allocated      = 0x08, // Marked as occuped in BAM
               SF_File           = 0x10, // Occupied by a file (found by following file block chains)
               SF_BAM            = 0x20, // Occupied by BAM (found by reading BAM)
               SF_Directory      = 0x40, // Occupied by directory (found by reading directory)
              } SectorFlags;

typedef struct {
    ubyte                   flags;
    DOSErrorCode            latest_dos_error;
    ubyte                   checksum;
    ubyte                   file_table_idx;
} SectorDescriptor;


/////////////////////////////////////////////////////////////////////////////////
// Track-related definitions

// These start with 1
typedef ubyte TrackNr;
typedef ubyte TrackSectorIndex; // The index of the sector within the track, starting with 1

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
    ubyte     directory_trackNr;          // expect:0x12
    ubyte     directory_sectorIdx;        // expect:0x01
    ubyte     format_indicator;           // expect: 0x41, "A"
    ubyte     double_sided;               // no=0x00, yes=0x80
    TrackBAM  track_bam[35];
    char      disk_name[16];              // padded with 160 (0xa0, shift-space)
    ubyte     pad1[2];                    // 160 (0xa0, shift-space)
    char      disk_id[2];
    ubyte     pad2[1];                    // 160 (0xa0, shift-space)
    ubyte     dos_version;                // 2 = CBM DOS 2.6
    ubyte     format_indicator_copy;      // expect: "A" for 4040 format 
    ubyte     floppy_mode[3];             // 0x00==1541, 0xa0==1571
    ubyte     reserved1[41];
    ubyte     blocks1571[35];             // unused on 1541
} BAM;

/////////////////////////////////////////////////////////////////////////////////
// Directory

enum { FILE_STATUS_MASK                = 0xf0,
       FILE_STATUS_NORMAL              = 0x00,
       FILE_STATUS_DELETED             = 0x80,
       FILE_STATUE_DELETED_REPLACEMENT = 0xa0,
       FILE_STATUS_LOCKED              = 0xC0};
enum { FILE_TYPE_MASK                  = 0x0f,
       FILE_TYPE_DELETED               = 0x00,
       FILE_TYPE_SEQUENTIAL            = 0x01,
       FILE_TYPE_PROGRAM               = 0x02,
       FILE_TYPE_USER                  = 0x03,
       FILE_TYPE_RELATIVE              = 0x04};

typedef struct
{
    ubyte chainNextTrackNr;           // Only filled in the 1st entry per directory block,
    ubyte chainNextSectorIdx;         // and they point to the next directory block. Else 0x00.

    ubyte file_type;                  //  FILE_STATUS_MASK / FILE_TYPE_MASK
    ubyte file_data_start_track_nr;
    ubyte file_data_start_sector_idx;
    char  file_name[16];              // filled to end with 0xa0 (shift-space)
    ubyte file_rel_sectors[2];        // only for REL files
    ubyte file_rel_record_size;       // only for REL files
    ubyte pad1[4];                    // are to be always 0x00
    ubyte dos_tmp_replace[2];         // temporary space for DOS and its replace operation
    u16   file_size_in_blocks;
} FileEntry;

enum {NO_MORE_DIRECTORY_TRACK  = 0x00,
      NO_MORE_DIRECTORY_SECTOR = 0xff,
      NO_MORE_FILE_TRACK       = 0x00 };
enum { MAX_DIRECTORY_SECTORS = 18, // Track 18 has 19 dectors but one is occupied by the BAM
       MAX_FILES_PER_DIRECTORY_SECTOR = 8,
       MAX_FILES_PER_DISK = MAX_DIRECTORY_SECTORS * MAX_FILES_PER_DIRECTORY_SECTOR // 144
     };

typedef struct
{
    FileEntry entries[MAX_FILES_PER_DIRECTORY_SECTOR];
} DirectoryBlock;


/////////////////////////////////////////////////////////////////////////////////
// Disk-related definitions

#define TRACKS_PER_DISK 35
#define SECTORS_PER_DISK 683

typedef struct
{
    SectorDescriptor descriptor[SECTORS_PER_DISK];
    char             disk_name[16];              // padded with 160 (0xa0, shift-space)
    ubyte            pad1[2];                    // zero to terminate disk name
    char             disk_id[2];                 // disk ID
    ubyte            pad2[2];                    // zero to terminate disk name
    FileEntry        files[MAX_FILES_PER_DISK];
    bool             bam_was_read;               // flag, if BAM was read from disk
    bool             dir_was_read;               // flag, if directory was read from disk
    ubyte            num_files_found;
} DiskDescriptor;

// The indexing starts with 0, unlike tracks and sectors
typedef u16 DiskSectorIndex;

extern DiskDescriptor g_disk_descriptor;


/////////////////////////////////////////////////////////////////////////////////
// Functions

// Geometry
ubyte            numSectorsInTrackNr(TrackNr trackNr);
DiskSectorIndex  trackAndSectorToDiskSectorIndex(TrackNr track_nr, TrackSectorIndex sector_nr);
TrackNr          diskSectorIndexToTrackNr(DiskSectorIndex disk_sector_index);
TrackSectorIndex diskSectorIndexToSectorNr(DiskSectorIndex disk_sector_index);

// Disk
void             clearDiskDescriptor(DiskDescriptor * const disk_descriptor);

// BAM
void             addBAMToDescriptor(BAM const * bam, DiskDescriptor * const diskDescriptor);

// Directory


#endif // DISK_H
