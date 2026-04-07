
#include "base.h"
#include "errors.h"
#include "channel.h"
#include "dos_io.h"
#include "kernal_io.h"
#include "opencbm_io.h"
#include "disk.h"
#include "display_aux.h"
#include "keyboard.h"

#include <c64.h>
#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <cbm.h>
#include <conio.h>
#include <peekpoke.h>
#include <stdlib.h>
#include <string.h>

DiskDescriptor g_disk_descriptor;

// returns false if aborted, else true
bool readDisk()
{
    bool abort_operation = false;

    kio_openChannel(&g_channel_command);
    kio_openChannel(&g_channel_data);
    sendInitializeDrive();

    displayMenu("<- Abort. Reading disk.");

    {
        TrackNr track_nr;
        for (track_nr = 1; (track_nr <= TRACKS_PER_DISK) && (abort_operation == false); ++track_nr)
        {
            TrackSectorIndex sector_idx;
            for (sector_idx = 0; (sector_idx < numSectorsInTrackNr(track_nr)) && (abort_operation == false); ++sector_idx)
            {
                DOS_ERROR_CODE dosec;
                SectorDescriptor * sd;

                sd = &(g_disk_descriptor.descriptor[trackAndSectorToDiskSectorIndex(track_nr, sector_idx)]);

                // We skip over sectors we already read
                if (0 != (sd->flags & SF_SectorRead))
                { continue; }

                sd->flags |= SF_Busy;
                displaySectorDescriptor(track_nr, sector_idx, sd);

                dosec = readSector(track_nr, sector_idx, &g_block_buffer);
                displayStatus((char const * const) &(getLastDriveStatusString()->data[0]));

                sd->flags |= SF_SectorRead;
                sd->flags &= ~SF_Busy;
                sd->latest_dos_error = dosec;
                sd->checksum = calculateBlockChecksum(&g_block_buffer);
                displaySectorDescriptor(track_nr, sector_idx, sd);

                if (true == keyb_userHoldsAbortKey())
                { abort_operation = true; } // abort on left arrow char
            }
        }
    }

    clearMenu();

    kio_closeChannel(&g_channel_data);
    kio_closeChannel(&g_channel_command);

    return abort_operation;
}

// returns false if aborted, else true
bool checkForWeakBlocks()
{
    bool abort_operation;

    abort_operation = false;

    kio_openChannel(&g_channel_command);
    kio_openChannel(&g_channel_data);

    displayMenu("<- Abort. Search weak blocks.");

    {
        TrackNr track_nr;
        for (track_nr = 1; (track_nr <= TRACKS_PER_DISK) && (abort_operation == false); ++track_nr)
        {
            TrackSectorIndex sector_idx;
            for (sector_idx = 0; (sector_idx < numSectorsInTrackNr(track_nr)) && (abort_operation == false); ++sector_idx)
            {
                DOS_ERROR_CODE dosec;
                SectorDescriptor * sd;

                sd = &(g_disk_descriptor.descriptor[trackAndSectorToDiskSectorIndex(track_nr, sector_idx)]);

                // If we did read the sector OK, then its very unlikely that the block
                // is weak. The checksum must have matched by chance for that to be the case.
                if (DOS_EC_OK == sd->latest_dos_error)
                { continue; }

                sd->flags |= SF_Busy;
                displaySectorDescriptor(track_nr, sector_idx, sd);

                dosec = readSector(track_nr, sector_idx, &g_block_buffer);
                if (DOS_EC_OK != dosec) // We keep the status on display until next error
                { displayStatus((char const * const) &(getLastDriveStatusString()->data[0])); }

                {
                    ubyte checksum;
                    checksum = calculateBlockChecksum(&g_block_buffer);
                    if (sd->checksum != checksum)
                    { sd->flags |= SF_WeakContents; }
                }

                sd->flags &= ~SF_Busy;
                displaySectorDescriptor(track_nr, sector_idx, sd);

                if (true == keyb_userHoldsAbortKey())
                { abort_operation = true; } // abort on left arrow char
            }
        }

        // replace status with the one from last sector, even if OK
        displayStatus((char const * const) &(getLastDriveStatusString()->data[0]));
    }

    clearMenu();

    kio_closeChannel(&g_channel_data);
    kio_closeChannel(&g_channel_command);

    return abort_operation;
}

// returns false if aborted, else true
bool readBAMAndDirectory()
{
    bool abort_operation;

    displayMenu("Reading BAM.");

    abort_operation = false;

    kio_openChannel(&g_channel_command);
    kio_openChannel(&g_channel_data);
    sendInitializeDrive();

    {
        // We'll use these to follow the
        // chain of sectors making up
        // the directory.
        TrackNr            track_nr;
        TrackSectorIndex   sector_idx;
        SectorDescriptor * sd;

        {
            // 18,0 is the BAM location on the disk
            sd = &(g_disk_descriptor.descriptor[trackAndSectorToDiskSectorIndex(18, 0)]);

            sd->flags |= SF_Busy;
            displaySectorDescriptor(18, 0, sd);

            sd->latest_dos_error = readSector(18, 0, &g_block_buffer);

            sd->flags |= (SF_SectorRead | SF_Allocated);
            sd->flags &= ~SF_Busy;
            displaySectorDescriptor(18, 0, sd);
        }

        {
            BAM const * bam;
            bam = (BAM const *) &(g_block_buffer.data[0]);
            addBAMToDescriptor(bam, &g_disk_descriptor);

            // Initialize these vars with the
            // beginning of the chained list of directory
            // sectors.
            track_nr   = bam->directory_trackNr;
            sector_idx = bam->directory_sectorIdx;
        }

        // Display BAM markers as an intermediate step.
        displayDiskDescriptor(&g_disk_descriptor);

        displayMenu("<- Abort. Reading directory.");

        // Now read the directory chain
        {
            bool                   end_of_dir_reached;
            DirectoryBlock const * dir_block_ptr;
            ubyte                  num_dir_sectors_read;

            end_of_dir_reached = false;
            dir_block_ptr = (DirectoryBlock const *) &(g_block_buffer.data[0]);
            num_dir_sectors_read = 0;

            while (false == end_of_dir_reached)
            {
                ubyte file_in_sector_idx;
                FileEntry * fe_ptr;

                // Read directory sector
                sd = &(g_disk_descriptor.descriptor[trackAndSectorToDiskSectorIndex(track_nr, sector_idx)]);

                sd->flags |= SF_Busy;
                displaySectorDescriptor(track_nr, sector_idx, sd);

                // This fills the buffer with new data, which we access via dir_block_ptr
                sd->latest_dos_error = readSector(track_nr, sector_idx, &g_block_buffer);
                sd->flags |= (SF_SectorRead | SF_Allocated);

                sd->flags &= ~SF_Busy;
                displaySectorDescriptor(track_nr, sector_idx, sd);

                // Ok, we got one in our buffer, count it
                ++num_dir_sectors_read;

                for (file_in_sector_idx = 0; file_in_sector_idx < MAX_FILES_PER_DIRECTORY_SECTOR; ++file_in_sector_idx)
                {
                    fe_ptr = &(dir_block_ptr->entries[file_in_sector_idx]);
                    // Check, if the entry is empty
                    if ((FILE_STATUS_NORMAL | FILE_TYPE_DELETED) != fe_ptr->fileType)
                    {
                        // This entry contains useful data
                        memcpy(&(g_disk_descriptor.files[g_disk_descriptor.numFilesFound]),
                               fe_ptr,
                               sizeof(FileEntry)); 
                        ++(g_disk_descriptor.numFilesFound);
                    }
                    // Note: We skip over empty directories and do not enter
                    // these in the disk descriptor. So the indices in the descriptor
                    // will not match the ones found in the chain of files on the disk
                    // as soon as we hit one empty file slot.
                }

                // We increment the track/sector values for the next while loop iteration
                track_nr   = dir_block_ptr->nextDirectoryTrackNr;
                sector_idx = dir_block_ptr->nextDirectorySectorIdx;

                if (true == keyb_userHoldsAbortKey())
                { abort_operation = true; } // abort on left arrow char

                // We stop reading directory blocks when the next pointer targets track 0,
                // which we don't have on a disk. The second condition is a protection to
                // protect against corrupted or protected disks and stops us from infinite reads.
                end_of_dir_reached = (NO_MORE_DIRECTORY_TRACK == track_nr)
                                  || (MAX_DIRECTORY_SECTORS == num_dir_sectors_read)
                                  || (true == abort_operation);
            }

            g_disk_descriptor.dirWasRead = (false == abort_operation);
        }
    }

    kio_closeChannel(&g_channel_data);
    kio_closeChannel(&g_channel_command);

    return abort_operation;
}

bool readFiles()
{
    bool abort_operation = false;
    ubyte file_idx;

    if (false == g_disk_descriptor.dirWasRead)
    { readBAMAndDirectory(); }

    kio_openChannel(&g_channel_command);
    kio_openChannel(&g_channel_data);

    displayMenu("<- Abort. Reading all files.");

    for (file_idx = 0; file_idx < g_disk_descriptor.numFilesFound; ++file_idx)
    {
        FileEntry *      fe_ptr;
        u16              blocks_read; // Used to break loops due to protections or corruption
        TrackNr          track_nr;
        TrackSectorIndex sector_idx;

        fe_ptr = &(g_disk_descriptor.files[file_idx]);

        // The beginning of the block chain is in the fe_ptr
        track_nr = fe_ptr->fileDataStartTrackNr;
        sector_idx = fe_ptr->fileDataStartSectorIdx;

        while (  (NO_MORE_FILE_TRACK != track_nr) // This is the exit condition (track 0) for a sane disk
              && (TRACKS_PER_DISK >= track_nr)    // Maybe due to corruption or copy protection
              && (numSectorsInTrackNr(track_nr) > sector_idx)) // same as above
        {
            SectorDescriptor * sd;

            sd = &(g_disk_descriptor.descriptor[trackAndSectorToDiskSectorIndex(track_nr, sector_idx)]);

            // Display as busy
            sd->flags |= SF_Busy;
            displaySectorDescriptor(track_nr, sector_idx, sd);

            sd->latest_dos_error = readSector(track_nr, sector_idx, &g_block_buffer);
            if (DOS_EC_OK != sd->latest_dos_error)
            { displayStatus((char const * const) &(getLastDriveStatusString()->data[0])); }
            else
            { displayStatus(fe_ptr->fileName); }

            // If we didn't read this sector already in another pass,
            // then mark this as read now. And, while we're at it,
            // compute the checksum.
            if (0 == (sd->flags & SF_SectorRead))
            {
                sd->flags |= SF_SectorRead;
                sd->checksum = calculateBlockChecksum(&g_block_buffer);
            }

            sd->flags          |= SF_File;
            sd->file_table_idx =  file_idx;

            // Remove busy marker
            sd->flags &= ~SF_Busy;
            displaySectorDescriptor(track_nr, sector_idx, sd);

            // Follow the chain
            track_nr   = g_block_buffer.data[0];
            sector_idx = g_block_buffer.data[1];

            if (true == keyb_userHoldsAbortKey())
            { abort_operation = true; } // abort on left arrow char

            // Protect against loops
            ++blocks_read;
            if (SECTORS_PER_DISK == blocks_read)
            { break; }
        }

    }

    clearMenu();

    kio_closeChannel(&g_channel_data);
    kio_closeChannel(&g_channel_command);

    return abort_operation;
}


int main(void)
{
    ubyte menu_page;
    menu_page = 0;

    initChannels();

    bgcolor(COLOR_BLACK);
    bordercolor(COLOR_GRAY1);
/*
    displayBlockData(&g_block_buffer);
    pause();
*/
    clearDiskDescriptor(&g_disk_descriptor);

    clearScreen();
    displayTrackAndSectorRulers();
    displayDiskDescriptor(&g_disk_descriptor);

    while (true)
    {
        char selection;

        displayStatus((char const * const) &(getLastDriveStatusString()->data[0]));

        switch(menu_page)
        {
            default:
                menu_page = 0;
                // intentional fall-through
                
                //               Line length
                //               0123456789012345678901234567890123456789
            case 0: displayMenu("Press space bar to cycle menu."); break;
            case 1: displayMenu("F1=New F3=- F5=Weak F7=-"); break;
            case 2: displayMenu("F2=Clear F4=Blks F6=BAM+Dir F8=Files"); break;
        }

        keyb_clearBufferedChars();
        selection = keyb_readChar_blocking(); // getchar();
        clearMenu();

        switch (selection)
        {
            case CH_F1:
            {
                clearDiskDescriptor(&g_disk_descriptor);
                displayDiskDescriptor(&g_disk_descriptor);
                if (true == readBAMAndDirectory())
                { break; }
                if (true == readFiles())
                { break; }
                if (true == readDisk())
                { break; }
            }
            break;
            case CH_F2:
            {
                clearDiskDescriptor(&g_disk_descriptor);
                displayDiskDescriptor(&g_disk_descriptor);
            }
            break;
            case CH_F3:
            {
            }
            break;
            case CH_F4:
            {
                readDisk();
            }
            break;
            case CH_F5:
            {
                checkForWeakBlocks();
            }
            break;
            case CH_F6:
            {
                readBAMAndDirectory();
            }
            break;
            case CH_F7:
            {
            }
            break;
            case CH_F8:
            {
                readFiles();
            }
            break;
            case ' ':
              ++menu_page;
              if (3 == menu_page)
              { menu_page = 1; }
            default:
            {
                clearStatus();
                clearRow(23);
                gotoxy(0,23);
                printf("unknown key 0x%02x", selection);
            }
            break;
        }
    }
}
