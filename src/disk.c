#include "disk.h"

DiskDescriptor g_disk_descriptor;

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
    // TODO replace some clearing by memset()
    {
        unsigned idx16;
        ubyte * ptr;

        for (idx16 = 0; idx16 < SECTORS_PER_DISK; ++idx16)
        {
            disk_descriptor->descriptor[idx16].flags            = 0x00;
            disk_descriptor->descriptor[idx16].latest_dos_error = DOS_EC_OK;
            disk_descriptor->descriptor[idx16].file_table_idx   = 0;
            disk_descriptor->descriptor[idx16].checksum         = 0;
            disk_descriptor->descriptor[idx16].file_successor_track_nr   = NO_MORE_FILE_TRACK;
            disk_descriptor->descriptor[idx16].file_successor_sector_idx = 0;
        }

        ptr = (ubyte *) &(disk_descriptor->files);
        for (idx16 = 0; idx16 < sizeof(FileEntry) * MAX_FILES_PER_DISK; ++idx16)
        { *(ptr++) = 0; }
    }

    {
        ubyte idx8;
        for (idx8 = 0; idx8<16; ++idx8)
        { disk_descriptor->disk_name[idx8] = ' '; }
        disk_descriptor->pad1[0] = '\0';
        disk_descriptor->pad1[1] = '\0';
        disk_descriptor->disk_id[0] = ' ';
        disk_descriptor->disk_id[1] = ' ';
        disk_descriptor->pad2[0] = '\0';
        disk_descriptor->pad2[1] = '\0';
    }

    disk_descriptor->bam_was_read = false;
    disk_descriptor->dir_was_read = false;
    disk_descriptor->num_files_found = 0;
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

void updateSectorDescriptor(SectorDescriptor * const sd, BlockData const * const block_data, DOSErrorCode dosec)
{
    // We just read a sector, so mark this
    sd->flags |= SF_SectorRead;
    // Now update the record of the latest DOS error code
    sd->latest_dos_error = dosec;

    // Now for the detailed flag computation.

    if (0 == (sd->flags & SF_SectorRead))
    {
        // Case A: The sector was never read before.
        if (DOS_EC_OK == dosec)
        {
            // Case AA: The sector was now read OK
            sd->flags    = sd->flags | SF_ChecksumOK; // Mark the sector as read and clear the checksum mismatch flag
            sd->checksum = calculateBlockChecksum(block_data);
        }
        else
        {
            // Case AB: The sector was now read with an error
            sd->flags    = (sd->flags | SF_TroubleReading) & ~SF_ChecksumOK; // Mark the sector as read and set the checksum mismatch and trouble reading flags
            // We still want to update the checksum, because we want to have a record of what the checksum was when we read the sector, even if it was read with an error.
            // If a sector yields different checksums on different reads, then it is a weak sector and we will mark it as such.
            sd->checksum = calculateBlockChecksum(block_data);
        }
    }    
    else
    {
        // Case B: The sector was read before.
        if (0 == (sd->flags & SF_TroubleReading))
        {
            // Case BA: The sector was OK before.
            if (DOS_EC_OK == dosec)
            {
                // Case BAA: The sector was read OK again.
                // In this case, we have no new information to update the descriptor with, so we don't change anything.
            }
            else
            {
                // Case BAB: The sector was read with an error now but was OK before.

                // We're having trouble reading the secotr now and this wasn't the case before, so we mark this as weak contents now.
                sd->flags |= (sd->flags | SF_TroubleReading | SF_WeakContents);
                if (SF_ChecksumOK == (sd->flags & SF_ChecksumOK))
                {
                    // If the checksum was OK before, then we don't update it, since we learned an accepted value before.
                    // This read was obviously worse than before.
                }
                else
                {
                    // If the checksum was not OK before, then we don't have any reliable data to compute a checksum from.
                    // If SF_ChecksumOK was already cleared, then the checksum was already marked as not OK, so we don't need to change it.
                    const ubyte new_checksum = calculateBlockChecksum(block_data);
                    if (new_checksum != sd->checksum)
                    {
                        // The checksum is different from what we had before, so we mark this as weak contents now.
                        sd->flags |= SF_WeakContents;
                    }
                }
            }
        }
        else
        {
            // Case BAB: The sector was read before and it had an error before.
            // In this case, there is nothing to do
        }

    }
}

bool isSectorFixedByLatestRead(SectorDescriptor const * const sd)
{
    return (  (SF_SectorRead     == (sd->flags & SF_SectorRead))
           && (SF_TroubleReading == (sd->flags & SF_TroubleReading))
           && (SF_ChecksumOK     == (sd->flags & SF_ChecksumOK)));
}



void addBAMToDescriptor(BAM const * bam, DiskDescriptor * const disk_descriptor)
{
    TrackNr track_nr;
    TrackSectorIndex sector_idx;
    SectorDescriptor * sd;

    // copy allocation table
    for (track_nr = 1; track_nr <= TRACKS_PER_DISK; ++track_nr)
    {
        TrackBAM * trk_bam_ptr = &(bam->track_bam[track_nr-1]);
        for (sector_idx = 0; sector_idx < numSectorsInTrackNr(track_nr); ++sector_idx)
        {
            ubyte * bam_byte;
            ubyte   bam_bit;

            bam_byte = &(trk_bam_ptr->allocation_bits_0_to_7) + (sector_idx >> 3);
            bam_bit  = 1 << (sector_idx & 0x07);

            sd = &(disk_descriptor->descriptor[trackAndSectorToDiskSectorIndex(track_nr, sector_idx)]);

            if  (((*bam_byte) & bam_bit) == 0x00)
            {
                // allocated
                sd->flags |= SF_Allocated;
            }
            else
            {
                // not allocated
                sd->flags &= ~SF_Allocated;
            }
        }
    }

    // copy disk name
    {
        ubyte idx;
        for (idx = 0; idx < sizeof(bam->disk_name); ++idx)
        { disk_descriptor->disk_name[idx] = bam->disk_name[idx]; }
    }

    // copy disk ID
    disk_descriptor->disk_id[0] = bam->disk_id[0];
    disk_descriptor->disk_id[1] = bam->disk_id[1];

    disk_descriptor->bam_was_read = true;
}

const char * fileTypeToString(ubyte file_type)
{
    switch (file_type & FILE_TYPE_MASK)
    {
        case FILE_TYPE_DELETED:               return "DEL";
        case FILE_TYPE_SEQUENTIAL:            return "SEQ";
        case FILE_TYPE_PROGRAM:               return "PRG";
        case FILE_TYPE_USER:                  return "USR";
        case FILE_TYPE_RELATIVE:              return "REL";
        default:                              return "???";
    }
}

const char * fileFlagsToString(ubyte file_type)
{
    switch (file_type & (FILE_FLAG_SAVING | FILE_FLAG_LOCKED | FILE_FLAG_CLOSED))
    {
        case FILE_FLAG_SAVING:                                       return "@ *";
        case FILE_FLAG_LOCKED:                                       return " >*";
        case FILE_FLAG_CLOSED:                                       return "OK ";
        case FILE_FLAG_SAVING | FILE_FLAG_LOCKED:                    return "@>*";
        case FILE_FLAG_SAVING | FILE_FLAG_CLOSED:                    return "@  ";
        case FILE_FLAG_LOCKED | FILE_FLAG_CLOSED:                    return " > ";
        case FILE_FLAG_SAVING | FILE_FLAG_LOCKED | FILE_FLAG_CLOSED: return "@> ";
        default:                                                     return "???";
    }
}
