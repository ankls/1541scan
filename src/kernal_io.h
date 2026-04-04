#ifndef KERNAL_IO_H
#define KERNAL_IO_H

#include "base.h"
#include "errors.h"
#include "channel.h"

// public interface
KERNAL_ST_SERIAL_STATUS kio_openChannel(Channel const * const channel);
void                    kio_closeChannel(Channel const * const channel);
bool                    kio_sendToDrive(Channel const * const channel, ubyte const * const data, const u16 length, u16 * const bytes_written);
KERNAL_ST_SERIAL_STATUS kio_readFromDrive(Channel const * const channel, ubyte * const data, const u16 buffer_length, u16 * const bytes_read);

// Helper for keyboard code
char kio_getInput();

#endif // KERNAL_IO_H
