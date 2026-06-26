
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
                sd->latest_dos_error = dosec;
                displayStatus((char const * const) &(getLastDriveStatusString()->data[0]));

                if (DOS_EC_OK != dosec)
                { sd->flags |= SF_ChecksumMismatch; }

                sd->flags |= SF_SectorRead;
                sd->flags &= ~SF_Busy;
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
                if ((SF_SectorRead == (sd->flags & SF_SectorRead)) && (SF_ChecksumMismatch != (sd->flags & SF_ChecksumMismatch)))
                { continue; }

                sd->flags |= SF_Busy;
                displaySectorDescriptor(track_nr, sector_idx, sd);

                dosec = readSector(track_nr, sector_idx, &g_block_buffer);
                sd->latest_dos_error = dosec;
                if (DOS_EC_OK != dosec) // We keep the status on display until next error
                { displayStatus((char const * const) &(getLastDriveStatusString()->data[0])); }
                else 
                {
                    // The checksum is now OK, thus the checksum error flag is cleared.
                    sd->flags &= ~SF_ChecksumMismatch;
                }

                {
                    ubyte checksum;
                    checksum = calculateBlockChecksum(&g_block_buffer);
                    // If we now computed a different checksum than the one we computed before, then the block is weak.
                    // This flag must be set regardless if the checksum is OK now or not, since any further reading
                    // may yield a different checksum again.
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
            DOS_ERROR_CODE dosec;
            // 18,0 is the BAM location on the disk
            sd = &(g_disk_descriptor.descriptor[trackAndSectorToDiskSectorIndex(18, 0)]);

            sd->flags |= SF_Busy;
            displaySectorDescriptor(18, 0, sd);

            dosec = readSector(18, 0, &g_block_buffer);
            sd->latest_dos_error = dosec;

            sd->flags |= (SF_SectorRead | SF_Allocated | SF_BAM);
            if (DOS_EC_OK != dosec)
            { sd->flags |= SF_ChecksumMismatch; }
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
        g_disk_descriptor.num_files_found = 0;
        {
            bool                   end_of_dir_reached;
            DirectoryBlock const * dir_block_ptr;
            ubyte                  num_dir_sectors_read;
            DOS_ERROR_CODE         dosec;

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
                dosec = readSector(track_nr, sector_idx, &g_block_buffer);
                sd->flags |= (SF_SectorRead | SF_Allocated | SF_Directory);
                sd->latest_dos_error = dosec;

                sd->flags &= ~SF_Busy;
                displaySectorDescriptor(track_nr, sector_idx, sd);

                // Ok, we got one in our buffer, count it
                ++num_dir_sectors_read;

                for (file_in_sector_idx = 0; file_in_sector_idx < MAX_FILES_PER_DIRECTORY_SECTOR; ++file_in_sector_idx)
                {
                    fe_ptr = &(dir_block_ptr->entries[file_in_sector_idx]);
                    // Check, if the entry is empty
                    if (FILE_TYPE_DELETED != fe_ptr->file_type)
                    {
                        // This entry contains useful data
                        memcpy(&(g_disk_descriptor.files[g_disk_descriptor.num_files_found]),
                               fe_ptr,
                               sizeof(FileEntry)); 
                        ++(g_disk_descriptor.num_files_found);
                    }
                    // Note: We skip over empty directories and do not enter
                    // these in the disk descriptor. So the indices in the descriptor
                    // will not match the ones found in the chain of files on the disk
                    // as soon as we hit one empty file slot.
                }

                // We increment the track/sector values for the next while loop iteration
                track_nr   = dir_block_ptr->entries[0].chainNextTrackNr;
                sector_idx = dir_block_ptr->entries[0].chainNextSectorIdx;

                if (true == keyb_userHoldsAbortKey())
                { abort_operation = true; } // abort on left arrow char

                // We stop reading directory blocks when the next pointer targets track 0,
                // which we don't have on a disk. The second condition is a protection to
                // protect against corrupted or protected disks and stops us from infinite reads.
                end_of_dir_reached = (NO_MORE_DIRECTORY_TRACK == track_nr)
                                  || (MAX_DIRECTORY_SECTORS == num_dir_sectors_read)
                                  || (DOS_EC_OK != dosec)
                                  || (true == abort_operation);
            }

            g_disk_descriptor.dir_was_read = (false == abort_operation);
        }
    }

    kio_closeChannel(&g_channel_data);
    kio_closeChannel(&g_channel_command);

    return abort_operation;
}

bool readFiles()
{
    bool user_abort = false;
    bool error_abort = false;
    ubyte file_idx;

    if (false == g_disk_descriptor.dir_was_read)
    { readBAMAndDirectory(); }

    kio_openChannel(&g_channel_command);
    kio_openChannel(&g_channel_data);

    displayMenu("<- Abort. Reading all files.");

    for (file_idx = 0; (file_idx < g_disk_descriptor.num_files_found) && (false == user_abort); ++file_idx)
    {
        FileEntry *      fe_ptr;
        TrackNr          track_nr;
        TrackSectorIndex sector_idx;
        u16              block_in_file;

        fe_ptr = &(g_disk_descriptor.files[file_idx]);

        // The beginning of the block chain is in the fe_ptr
        track_nr = fe_ptr->file_data_start_track_nr;
        sector_idx = fe_ptr->file_data_start_sector_idx;

        block_in_file = 0;
        error_abort = false;

        while (  (NO_MORE_FILE_TRACK != track_nr) // This is the exit condition (track 0) for a sane disk
              && (TRACKS_PER_DISK >= track_nr)    // Maybe due to corruption or copy protection
              && (numSectorsInTrackNr(track_nr) > sector_idx) // same as above
              && (false == user_abort)
              && (false == error_abort) )
        {
            SectorDescriptor * sd;

            if (true == keyb_userHoldsAbortKey())
            { user_abort = true; } // abort on left arrow char

            sd = &(g_disk_descriptor.descriptor[trackAndSectorToDiskSectorIndex(track_nr, sector_idx)]);

            // Display as busy
            sd->flags |= SF_Busy;
            displaySectorDescriptor(track_nr, sector_idx, sd);

            sd->latest_dos_error = readSector(track_nr, sector_idx, &g_block_buffer);
            ++block_in_file;
            if (DOS_EC_OK != sd->latest_dos_error)
            { displayStatus((char const * const) &(getLastDriveStatusString()->data[0])); }
            else
            { displayStatusAndNr((char const * const) fe_ptr->file_name, block_in_file); }

            // If we didn't read this sector already in another pass,
            // then mark this as read now. And, while we're at it,
            // compute the checksum.
            if (0 == (sd->flags & SF_SectorRead))
            {
                sd->flags |= SF_SectorRead;
                sd->checksum = calculateBlockChecksum(&g_block_buffer);
            }

            if (0 == (sd->flags & SF_File))
            {
                // Mark that we found file data in this sector.
                sd->flags          |= SF_File;
                sd->file_table_idx =  file_idx;
                sd->file_successor_track_nr   = g_block_buffer.data[0];
                sd->file_successor_sector_idx = g_block_buffer.data[1];
            }
            else
            {
                // We were here already with this or another file,
                // so we're in a loop. This can be due to copy protection or disk corruption,
                // so we break out of the loop to avoid an infinite loop.
                error_abort = true;
            }
            
            // Remove busy marker
            sd->flags &= ~SF_Busy;
            displaySectorDescriptor(track_nr, sector_idx, sd);

            // Follow the chain (won't matter if we abort)
            track_nr   = g_block_buffer.data[0];
            sector_idx = g_block_buffer.data[1];
        }

    }

    clearMenu();

    kio_closeChannel(&g_channel_data);
    kio_closeChannel(&g_channel_command);

    return user_abort;
}

static void displaySectorMetadata(TrackNr t, TrackSectorIndex s, SectorDescriptor const * const sd_m)
{
    gotoxy(0,19);
    printf("Track %d Sector %d", t, s);
    gotoxy(0,20);
    printf("DOS error: %s", getLastDriveStatusString()->data);
    gotoxy(0,21);
    printf("Flags: %s%s%s%s%s%s%s%s",
           (0 != (sd_m->flags & SF_SectorRead))       ? "Read "   : "",
           (0 != (sd_m->flags & SF_WeakContents))     ? "Weak "   : "",
           (0 != (sd_m->flags & SF_ChecksumMismatch)) ? "ChkErr " : "",
           (0 != (sd_m->flags & SF_Busy))             ? "Busy "   : "",
           (0 != (sd_m->flags & SF_Allocated))        ? "Alloc "  : "",
           (0 != (sd_m->flags & SF_File))             ? "File "   : "",
           (0 != (sd_m->flags & SF_BAM))              ? "BAM "    : "",
           (0 != (sd_m->flags & SF_Directory))        ? "Dir "    : "");
    gotoxy(0,22);
    printf("Checksum: 0x%02x", sd_m->checksum);
    gotoxy(0,23);
    printf("file: %s", (sd_m->flags & SF_File) ? g_disk_descriptor.files[sd_m->file_table_idx].file_name : "n/a");
}

void inspectSector(TrackNr track_nr, TrackSectorIndex sector_idx, SectorDescriptor * sd, bool show_as_hex)
{
    bool hex;
    const char * menu_text;
    bool needs_redraw;
    bool has_status_override;
    char status_override[41];
    bool bufferContainsFixedData = false;

    /* Declarations done; init variables */
    hex = show_as_hex;
    needs_redraw = true; /* initial draw */
    has_status_override = false;

    clearScreen();
    kio_openChannel(&g_channel_command);
    kio_openChannel(&g_channel_data);
    (void) readSector(track_nr, sector_idx, &g_block_buffer);
    kio_closeChannel(&g_channel_data);
    kio_closeChannel(&g_channel_command);

    /* Wait for user input: F1 = re-read weak sector; F3 = toggle display; left arrow/ESC (0x5f) = exit */
    for (;;)
    {
        char c;
        /* Centralized redraw logic */
        if (needs_redraw)
        {
            if (hex)
            { displayBlockDataAsHex(&g_block_buffer); }
            else // PETSCII
            { displayBlockDataAsPetscii(&g_block_buffer); }

            /* Print metadata using helper to avoid duplication */
            displaySectorMetadata(track_nr, sector_idx, sd);

            /* Menu text depends on current sector flags */
            if ((0 != (sd->flags & SF_WeakContents)) && (0 != (sd->flags & SF_ChecksumMismatch)))
            { menu_text = "F1=Reread F3=Hex/PETSCII <-=Exit"; }
            else if (bufferContainsFixedData)
            { menu_text = "F3=Hex/PETSCII F5=Write <-=Exit"; }
            else
            { menu_text = "F3=Hex/PETSCII <-=Exit"; }
            displayMenu(menu_text);

            /* Status line: either override or drive status */
            clearStatus();
            if (has_status_override)
            { displayStatus(status_override); }
            else
            { displayStatus((char const * const) &(getLastDriveStatusString()->data[0])); }

            needs_redraw = false;
        }

        keyb_clearBufferedChars();
        c = keyb_readChar_blocking();
        if (c == 0x5f) /* Arrow left / ESC used as abort */
        {
            clearMenu();
            clearStatus();
            return;
        }
        else if (c == CH_F1)
        {
            /* Only perform re-read when sector marked as weak and has wrong checksum or, if it wasn't read before */
            if (  (0 == (sd->flags & SF_WeakContents))
               || (0 == (sd->flags & SF_ChecksumMismatch)))
            {
                /* ignore */ ;
            }
            else
            {
                DOS_ERROR_CODE dosec;
                bufferContainsFixedData = false;

                displaySectorDescriptor(track_nr, sector_idx, sd);

                kio_openChannel(&g_channel_command);
                kio_openChannel(&g_channel_data);
                dosec = readSector(track_nr, sector_idx, &g_block_buffer);
                sd->latest_dos_error = dosec;
                kio_closeChannel(&g_channel_data);
                kio_closeChannel(&g_channel_command);

                sd->flags |= SF_SectorRead;

                if (DOS_EC_OK == dosec)
                {
                    bufferContainsFixedData = true;
                    sd->flags &= ~SF_ChecksumMismatch;
                }
                else
                {
                    sd->flags |= SF_ChecksumMismatch;
                }

                sd->checksum = calculateBlockChecksum(&g_block_buffer);

                displaySectorDescriptor(track_nr, sector_idx, sd);

                /* Prepare status override and request redraw */
                if (bufferContainsFixedData)
                { strcpy(status_override, "Correct reading found!"); }
                else
                { strcpy(status_override, "Read unsuccessful."); }
                has_status_override = true;
                needs_redraw = true;
            }
        }
        else if (c == CH_F3)
        {
            /* Toggle display mode and request redraw; clear any status override */
            hex = !hex;
            has_status_override = false;
            needs_redraw = true;
        }
        else if (c == CH_F5)
        {
            /* Only perform write when sector marked as weak and has correct checksum */
            if (false == bufferContainsFixedData)
            {
                /* ignore */ ;
            }
            else
            {
                DOS_ERROR_CODE dosec;

                displaySectorDescriptor(track_nr, sector_idx, sd);

                kio_openChannel(&g_channel_command);
                kio_openChannel(&g_channel_data);
                dosec = writeSector(track_nr, sector_idx, &g_block_buffer);
                sd->latest_dos_error = dosec;
                kio_closeChannel(&g_channel_data);
                kio_closeChannel(&g_channel_command);

                if (DOS_EC_OK == dosec)
                {
                    sd->flags &= ~SF_WeakContents;
                    sd->flags &= ~SF_ChecksumMismatch;
                    strcpy(status_override, "Write successful!");
                }
                else
                {
                    strcpy(status_override, "Write failed.");
                }

                displaySectorDescriptor(track_nr, sector_idx, sd);

                /* Request redraw */
                has_status_override = true;
                needs_redraw = true;
            }
        }
        else
        {
            /* ignore other keys and continue to wait */
        }
    }
}

void selectSector()
{
    TrackNr track_nr;
    TrackSectorIndex sector_idx;

    track_nr = 1;
    sector_idx = 0;

    displayMenu("Abort=<- Cursor=Move F1=ShowHex F3=ShowPetscii");

    do
    {
        SectorDescriptor * sd;
        char c;

        sd = &(g_disk_descriptor.descriptor[trackAndSectorToDiskSectorIndex(track_nr, sector_idx)]);
        sd->flags |= SF_Busy;
        displaySectorDescriptor(track_nr, sector_idx, sd);

        keyb_clearBufferedChars();
        c = keyb_readChar_blocking(); // getchar();

        sd->flags &= ~SF_Busy;
        displaySectorDescriptor(track_nr, sector_idx, sd);

        switch (c)
        {
            case 0x91: // up arrow char
                if (sector_idx > 0)
                { --sector_idx; }
                break;
            case 0x11: // down arrow char
                if (sector_idx < numSectorsInTrackNr(track_nr) - 1)
                { ++sector_idx; }
                break;
            case 0x9d: // left arrow char
                if (track_nr > 1)
                { --track_nr; }
                break;
            case 0x1d: // right arrow char
                if (track_nr < TRACKS_PER_DISK)
                { ++track_nr; }
                break;
            case 0x5f: // Arrow left / ESC char, used as abort key in other places, so we use it here as well for consistency
                return;
            case CH_F1: // F1 key
            case CH_F3: // F3 key
                inspectSector(track_nr, sector_idx, sd, CH_F1 == c /* show_as_hex */);
                return;
            default:
                break;
        }
    }
    while (true);
}

const char * fileIndexToHealthString(ubyte file_index)
{
    // We rely on the file with the proper index to be present in our structures,
    // so ensure that first before calling this function.
    // We walk the file blocks and see that all blocks are readable and not weak.
    // If so, we return "OK ", else "DMG".
    FileEntry * fe_ptr;
    SectorDescriptor * sd;
    TrackNr track_nr;
    TrackSectorIndex sector_idx;
    u16 counter; // to catch circles in successor chain, which can be due to copy protection or disk corruption

    fe_ptr = &(g_disk_descriptor.files[file_index]);
    track_nr = fe_ptr->file_data_start_track_nr;
    sector_idx = fe_ptr->file_data_start_sector_idx;
    counter = 0;

    while (counter < SECTORS_PER_DISK)
    {
        sd = &(g_disk_descriptor.descriptor[trackAndSectorToDiskSectorIndex(track_nr, sector_idx)]);

        // check if this block is ok
        if (  (0 == (sd->flags & SF_SectorRead))
           || (0 != (sd->flags & SF_ChecksumMismatch))
           || (0 != (sd->flags & SF_WeakContents))
           || (FILE_TYPE_DELETED == fe_ptr->file_type))
        { return "DMG"; }

        // See if there is a successor block in the file, if not, we're done and can return OK
        if (NO_MORE_FILE_TRACK == sd->file_successor_track_nr)
        { return "OK "; }

        track_nr = sd->file_successor_track_nr;
        sector_idx = sd->file_successor_sector_idx;

        ++counter;
    }

    return "DMG";
}

void inspectDirectory()
{
    clearScreen();
    // Leverage the fact that we read the directory already in the disk descriptor.

    {
        ubyte file_idx;
        ubyte file_display_offset = 0;
        const ubyte display_lines = 20; // number of file lines we show at once (rows 1..20)

        // Interaction loop: allow scrolling with cursor keys and exit with left-arrow (abort)
        do
        {
            // Header
            gotoxy(0,0);
            //      0123456789012345678901234567890123456789
            printf("Nr Name             Tp. Sta Siz Chk");

            // Clear the file area first
            {
                ubyte r;
                for (r = 1; r <= display_lines; ++r)
                { clearRow(r); }
            }

            // Draw visible files
            for (file_idx = file_display_offset; (file_idx < g_disk_descriptor.num_files_found) && (file_idx < file_display_offset + display_lines); ++file_idx)
            {
                FileEntry * fe_ptr;

                fe_ptr = &(g_disk_descriptor.files[file_idx]);

                gotoxy(0, 1 + file_idx - file_display_offset);
                printf("%2d %16s %3s %3s %03d %3s",
                       file_idx + 1,
                       fe_ptr->file_name,
                       fileTypeToString(fe_ptr->file_type),
                       fileFlagsToString(fe_ptr->file_type),
                       fe_ptr->file_size_in_blocks,
                       fileIndexToHealthString(file_idx));
            }

            // Menu line with scrolling markers
            {
                char menu_buf[41];
                int i;
                const char * hint = "Cursor=Move <-=Exit";

                for (i = 0; i < 40; ++i) menu_buf[i] = ' ';
                menu_buf[40] = '\0';

                // base hint
                for (i = 0; (hint[i] != '\0') && (i < 40); ++i) menu_buf[i] = hint[i];

                // show marker if there are files above
                if (file_display_offset > 0)
                { menu_buf[36] = '^'; }

                // show marker if there are files below
                if ((u16)file_display_offset + display_lines < (u16)g_disk_descriptor.num_files_found)
                { menu_buf[39] = 'v'; }

                displayMenu(menu_buf);
            }

            // status line
            clearStatus();
            gotoxy(0,23);
            printf("Files %d..%d of %d", file_display_offset + 1,
                   (file_display_offset + display_lines < g_disk_descriptor.num_files_found) ? (file_display_offset + display_lines) : g_disk_descriptor.num_files_found,
                   g_disk_descriptor.num_files_found);

            // wait for user input for navigation
            {
                char c;
                keyb_clearBufferedChars();
                c = keyb_readChar_blocking();

                switch (c)
                {
                    case 0x91: // up arrow
                        if (file_display_offset > 0)
                        { --file_display_offset; }
                        break;
                    case 0x11: // down arrow
                        if ((u16)file_display_offset + display_lines < (u16)g_disk_descriptor.num_files_found)
                        { ++file_display_offset; }
                        break;
                    case 0x5f: // left arrow / abort -> exit directory view
                        clearMenu();
                        clearStatus();
                        return;
                    /*
                    case CH_F5:
                    {
                        // If user pressed F5 while in directory, select currently highlighted file (first visible)
                        selectSector();
                        clearScreen();
                        displayTrackAndSectorRulers();
                        displayDiskDescriptor(&g_disk_descriptor);
                        clearMenu();
                        return;
                    }
                    */
                    default:
                        // ignore other keys and redraw
                        clearStatus();
                        clearRow(23);
                        gotoxy(0,23);
                        printf("unknown key 0x%02x", c);
                        break;
                }
            }
        } while (true);
    }
}


int main(void)
{
    ubyte menu_page;
    bool show_drive_status;

    menu_page = 0;
    show_drive_status = false;

    initChannels();
    
    clearDiskDescriptor(&g_disk_descriptor);

    clearScreen();
    displayTrackAndSectorRulers();
    displayDiskDescriptor(&g_disk_descriptor);
    displayStatus("github.com/ankls/1541scan 2026-06-28");

    while (true)
    {
        char selection;

        textcolor(TGI_COLOR_GRAY3);
        bgcolor(COLOR_BLACK);
        bordercolor(COLOR_GRAY1);

        if (true == show_drive_status)
        { displayStatus((char const * const) &(getLastDriveStatusString()->data[0])); }

        switch(menu_page)
        {
            default:
                menu_page = 0;
                // intentional fall-through
                
                //               Line length
                //               0123456789012345678901234567890123456789
            case 0: displayMenu("Press space bar to cycle menu."); break;
            case 1: displayMenu("F1=New F3=Weak F5=Block F7=Files"); break;
            case 2: displayMenu("F2=Clear F4=Blks F6=BAM+Dir F8=RdFiles"); break;
        }

        keyb_clearBufferedChars();
        selection = keyb_readChar_blocking(); // getchar();
        clearMenu();

        show_drive_status = true; // reset to default after menu selection, some operations disable it

        switch (selection)
        {
            case CH_F1:
            {
                clearScreen();
                displayTrackAndSectorRulers();
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
                clearScreen();
                displayTrackAndSectorRulers();
                clearDiskDescriptor(&g_disk_descriptor);
                displayDiskDescriptor(&g_disk_descriptor);
            }
            break;
            case CH_F3:
            {
                checkForWeakBlocks();
            }
            break;
            case CH_F4:
            {
                readDisk();
            }
            break;
            case CH_F5:
            {
                selectSector();
                clearScreen();
                displayTrackAndSectorRulers();
                displayDiskDescriptor(&g_disk_descriptor);
            }
            break;
            case CH_F6:
            {
                readBAMAndDirectory();
            }
            break;
            case CH_F7:
            {
                if (false == g_disk_descriptor.dir_was_read)
                {
                    if (true == readBAMAndDirectory())
                    { break; }
                    if (true == readFiles())
                    { break; }
                }

                inspectDirectory();
                clearScreen();
                displayTrackAndSectorRulers();
                displayDiskDescriptor(&g_disk_descriptor);
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
              break;
            default:
            {
                clearStatus();
                clearRow(23);
                gotoxy(0,23);
                printf("unknown key 0x%02x", selection);
                show_drive_status = false;
            }
            break;
        }
    }
}
