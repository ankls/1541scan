#ifndef DISPLAY_AUX_H
#define DISPLAY_AUX_H

#include "base.h"
#include "disk.h"

void dosErrorCharAndColor(const DOS_ERROR_CODE dec, char * c, ubyte * front_color);
void sectorDescriptorToCharAndColor(SectorDescriptor const * const sector_descriptor, char * c, ubyte * color);

void clearScreen();
void clearRow(ubyte row);


//////////////////////////////////////////////////////////////////////////
// Screen layout "disk block health map" and "sector details"
void displayStatus(const char * status);
void displayMenu(const char * menu);

void clearStatus();
void clearMenu();


//////////////////////////////////////////////////////////////////////////
// Screen layout "block hex display"
void displayBlockData(BlockData const * const block_data);

//////////////////////////////////////////////////////////////////////////
// Screen layout "disk block health map"
void displayTrackAndSectorRulers();
void displayDiskDescriptor(DiskDescriptor const * const disk_descriptor);
void displaySectorDescriptor(TrackNr const track_nr, TrackSectorIndex const sector_idx, SectorDescriptor const * const sd);

#endif // DISPLAY_AUX_H
