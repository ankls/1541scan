#ifndef ERRORS_H
#define ERRORS_H

#include "base.h"

//////////////////////////////////////////////////////////
/////// Disk error codes returned by the 1541 jobs ///////
//////////////////////////////////////////////////////////

typedef ubyte JOB_ERROR_CODE;

typedef enum {JOB_EC_OK                             = 0x01,
              JOB_EC_HEADER_BLOCK_NOT_FOUND         = 0x02,
              JOB_EC_SYNC_NOT_FOUND                 = 0x03,
              JOB_EC_DATA_BLOCK_NOT_FOUND           = 0x04,
              JOB_EC_CHECKSUM_ERROR_IN_DATA_BLOCK   = 0x05,
              JOB_EC_VERIFY_ERROR                   = 0x07,
              JOB_EC_DISK_WRITE_PROTECTED           = 0x08,
              JOB_EC_CHECKSUM_ERROR_IN_HEADER_BLOCK = 0x09,
              JOB_EC_WRONG_ID_READ                  = 0x0B,
              JOB_EC_DISK_NOT_INSERTED              = 0x0F } JobErrorCode;

char const * jobErrorString(const JOB_ERROR_CODE jec);

//////////////////////////////////////////////////////////
//////// Disk error codes returned by the 1541 DOS ///////
//////////////////////////////////////////////////////////

typedef ubyte DOS_ERROR_CODE;

typedef enum {DOS_EC_OK                             =  0,
              DOS_EC_FILES_SCRATCHED                =  1,
              DOS_EC_READ_ERROR_20                  = 20,
              DOS_EC_READ_ERROR_21                  = 21,
              DOS_EC_READ_ERROR_22                  = 22,
              DOS_EC_READ_ERROR_23                  = 23,
              DOS_EC_READ_ERROR_24                  = 24,
              DOS_EC_WRITE_ERROR_DATA               = 25,
              DOS_EC_WRITE_PROTECT_ON               = 26,
              DOS_EC_READ_ERROR_27                  = 27,
              DOS_EC_WRITE_ERROR_SYNC               = 28,
              DOS_EC_ID_MISMATCH                    = 29,
              DOS_EC_SYNTAX_ERROR_MALFORMED         = 30,
              DOS_EC_SYNTAX_ERROR_UNKNOWN_COMMAND   = 31,
              DOS_EC_SYNTAX_ERRPR_COMMAND_TOO_LONG  = 32,
              DOS_EC_SYNTAX_ERROR_ILLEGAL_JOKER     = 33,
              DOS_EC_SYNTAX_ERROR_NO_FILENAME       = 34,
              DOS_EC_FILE_NOT_FOUND_USR             = 39,
              DOS_EC_RECORD_NOT_PRESENT             = 50,
              DOS_EC_OVERFLOW_IN_RECORD             = 51,
              DOS_EC_FILE_TOO_LARGE                 = 52,
              DOS_EC_WRITE_FILE_OPEN                = 60,
              DOS_EC_FILE_NOT_OPEN                  = 61,
              DOS_EC_FILE_NOT_FOUND                 = 62,
              DOS_EC_FILE_EXISTS                    = 63,
              DOS_EC_FILE_TYPE_MISMATCH             = 64,
              DOS_EC_NO_BLOCK                       = 65,
              DOS_EC_ILLEGAL_TOS_DISK               = 66,
              DOS_EC_ILLEGAL_TOS_FILE               = 67,
              DOS_EC_NO_CHANNEL                     = 70,
              DOS_EC_DIR_ERROR                      = 71,
              DOS_EC_DISK_FULL                      = 72,
              DOS_EC_VERSION                        = 73,
              DOS_EC_DRIVE_NOT_READY                = 74 } DOSErrorCode;

// To declare code "not available"
#define DOS_EC_NA 0xff

char const * dosErrorString(const DOS_ERROR_CODE dec);

//////////////////////////////////////////////////////////
//////// Disk error codes returned by the KERNAL ///////
//////////////////////////////////////////////////////////

typedef ubyte KERNAL_ST_SERIAL_STATUS;

typedef enum { READST_SERIAL_DEVICE_NOT_PRESENT_FLAG = 0x80,
               READST_SERIAL_END_OF_FILE_FLAG        = 0x40,
               READST_SERIAL_VERIFY_ERROR_FLAG       = 0x10,
               READST_SERIAL_TIMEOUT_FLAG            = 0x02,
               READST_SERIAL_TIMEOUT_DIRECTION_FLAG  = 0x01 } ReadStatusSerialFlag;
#define KERNAL_ST_SERIAL_OK 0x00

//////////////////////////////////////////////////////////
//////// Translation and mapping                   ///////
//////////////////////////////////////////////////////////

// job return codes are 4 bit wide, DOS codes are 8 bit wide
// Job code     Meaning                         DOS code    DOS meaning
// 0x00         N/A                             N/A         N/A
// 0x01         OK                              00          OK
// 0x02         Header block not found          20          READ ERROR
// 0x03         SYNC not found                  21          READ ERROR

// 0x04         Data block not found            22          READ ERROR
// 0x05         Checksum error in data block    23          READ ERROR
// 0x06         N/A                             N/A         N/A
// 0x07         Verify error                    25          WRITE ERROR

// 0x08         Disk write protected            26          WRITE PROTECT ON
// 0x09         Checksum error in header block  27          READ ERROR
// 0x0A         N/A                             N/A         N/A
// 0x0B         Wrong ID read                   29          ID MISMATCH

// 0x0C         N/A                             N/A         N/A
// 0x0D         N/A                             N/A         N/A
// 0x0E         N/A                             N/A         N/A
// 0x0F         Disk not inserted               74          DRIVE NOT READY


// Mapping table
extern const DOS_ERROR_CODE JOB_EC_TO_DOS_EC[16];

#endif // ERRORS_H