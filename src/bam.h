#ifndef BAM_H
#define BAM_H

#include "base.h"

typedef struct
{
    ubyte allocation_map;
    ubyte allocation_bits_0_to_7;
    ubyte allocation_bits_8_to_15;
    ubyte allocation_bits_16_to_20;
} TrackBAM;


typedef struct
{
    ubyte     directory_track;            // expect:0x12
    ubyte     directory_sector;           // expect:0x01
    ubyte     format_indicator;           // expect: 0x41, "A"
    ubyte     double_sided;               // no=0x00, yes=0x80
    TrackBAM  track_bam[35];
    char      disk_name[16];              // padded with 160 (0xa0, shift-space)
    ubyte     pad1[2];                    // 160 (0xa0, shift-space)
    char      disk_id[2];
    ubyte     pad2[1];                    // 160 (0xa0, shift-space)
    ubyte     dos_version;                // 2 = CBM DOS 2.6
    ubyte     format_indicator_copy;      // expect: "A"
    ubyte     floppy_mode[3];             // 0x00==1541, 0xa0==1571
    ubyte     reserved1[41];
    ubyte     blocks1571[35];             // unused on 1541
} BAM;

#endif // BAM_H
