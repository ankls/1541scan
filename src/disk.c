#include "disk.h"

// Note: Index 0 has the value for track 1
const ubyte SECTORS_PER_TRACK_IDX[TRACKS_PER_DISK] = { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, // tracks 1..17
                                                   19, 19, 19, 19, 19, 19, 19, // tracks 18..24
                                                   18, 18, 18, 18, 18, 18, // tracks 25..30
                                                   17, 17, 17, 17, 17 }; // tracks 31..35
// Note: Index 0 has the value for track 1
const DiskSectorIndex START_SECTOR_INDEX_PER_TRACK_IDX[TRACKS_PER_DISK]
                                               = { 0, 21, 42, 63, 84, 105, 126, 147, 168, 189, 210, 231, 252, 273, 294, 315, 336, // tracks 1..17
                                                   357, 376, 395, 414, 433, 452, 471, // tracks 18..24
                                                   490, 508, 526, 544, 562, 580, // tracks 25..30
                                                   598, 615, 632, 649, 666 }; // tracks 31..35

ubyte numSectorsInTrackNr(TrackNr trackNr)
{
    return SECTORS_PER_TRACK_IDX[trackNr-1]; // Our array starts with track 1 at index 0, so we need to subtract 1 from the track number to get the correct index.
}

DiskSectorIndex firstDiskSectorIndexForTrackNr(TrackNr trackNr)
{
    return START_SECTOR_INDEX_PER_TRACK_IDX[trackNr-1]; // Our array starts with track 1 at index 0, so we need to subtract 1 from the track number to get the correct index.
}

DiskSectorIndex trackAndSectorToDiskSectorIndex(TrackNr track_nr, TrackSectorIndex sector_nr)
{
    return firstDiskSectorIndexForTrackNr(track_nr) + sector_nr;
}

TrackNr diskSectorIndexToTrackNr(DiskSectorIndex const disk_sector_index)
{
    // This can be optimized by precomputing the starting disk sector index for each track, but this is good enough for now.
    ubyte track_nr;
    for (track_nr = 1; track_nr <= TRACKS_PER_DISK; ++track_nr)
    {
        if (disk_sector_index < firstDiskSectorIndexForTrackNr(track_nr))
        { return track_nr - 1; }
    }
    return TRACKS_PER_DISK; // If the disk sector index is greater than the starting index of the last track, it belongs to the last track.
}

TrackSectorIndex diskSectorIndexToTrackSectorIndex(DiskSectorIndex const disk_sector_index)
{
    TrackNr track_nr;
    track_nr = diskSectorIndexToTrackNr(disk_sector_index);
    return disk_sector_index - firstDiskSectorIndexForTrackNr(track_nr);
}

void clearDiskDescriptor(DiskDescriptor * const disk_descriptor)
{
    unsigned i;
    for (i = 0; i < SECTORS_PER_DISK; ++i)
    {
        disk_descriptor->descriptor[i].flags = 0;
        disk_descriptor->descriptor[i].latest_dos_error = DOS_EC_OK;
    }
}

// Computes the checksum byte of a block
ubyte calculateBlockChecksum(BlockData const * const block_data)
{
    ubyte checksum = 0;
    unsigned i;
    for (i = 0; i < 256; ++i)
    {
        checksum ^= block_data->data[i];
    }
    return checksum;
}

