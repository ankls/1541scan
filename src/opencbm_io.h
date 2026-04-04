#ifndef OPENCBM_IO
#define OPENCBM_IO

#include "base.h"
#include "channel.h"
#include "errors.h"

// public interface
KERNAL_ST_SERIAL_STATUS opencbmio_openChannel(Channel const * const channel);
void                    opencbmio_closeChannel(Channel const * const channel);
bool                    opencbmio_sendToDrive(Channel const * const channel, ubyte const * const data, const u16 length, u16 * const bytes_written);
KERNAL_ST_SERIAL_STATUS opencbmio_readFromDrive(Channel const * const channel, ubyte * const data, const u16 buffer_length, u16 * const bytes_read);

#endif // OPENCBM_IO

