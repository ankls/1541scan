#include "errors.h"

char const * jobErrorString(const JOB_ERROR_CODE jec)
{
    switch(jec)
    {
        case JOB_EC_OK:                             return "0x01 OK";
        case JOB_EC_HEADER_BLOCK_NOT_FOUND:         return "0x02 header block not found";
        case JOB_EC_SYNC_NOT_FOUND:                 return "0x03 SYNC not found";
        case JOB_EC_DATA_BLOCK_NOT_FOUND:           return "0x04 data block not found";
        case JOB_EC_CHECKSUM_ERROR_IN_DATA_BLOCK:   return "0x05 checksum error in data block";
        case JOB_EC_VERIFY_ERROR:                   return "0x07 verify error";
        case JOB_EC_DISK_WRITE_PROTECTED:           return "0x08 disk write protected";
        case JOB_EC_CHECKSUM_ERROR_IN_HEADER_BLOCK: return "0x09 checksum error in header block";
        case JOB_EC_WRONG_ID_READ:                  return "0x0b wrong disk ID";
        case JOB_EC_DISK_NOT_INSERTED:              return "0x0f disk not inserted";
        default:                                    return "undefined job code";
    }
}

char const * dosErrorString(const DOS_ERROR_CODE dec)
{
    switch(dec)
    {
        case DOS_EC_OK:                             return "00 OK";
        case DOS_EC_FILES_SCRATCHED:                return "01 files scratched";
        case DOS_EC_READ_ERROR_20:                  return "20 read error (data block header not found)";
        case DOS_EC_READ_ERROR_21:                  return "21 read error (SYNC not found)";
        case DOS_EC_READ_ERROR_22:                  return "22 read error (checksum in data block header)";
        case DOS_EC_READ_ERROR_23:                  return "23 read error (checksum in data block body)";
        case DOS_EC_READ_ERROR_24:                  return "24 read error (checksum)";
        case DOS_EC_WRITE_ERROR_DATA:               return "25 write error (data block)";
        case DOS_EC_WRITE_PROTECT_ON:               return "26 disk write protected";
        case DOS_EC_READ_ERROR_27:                  return "27 read error";
        case DOS_EC_WRITE_ERROR_SYNC:               return "28 write error (SYNC not found)";
        case DOS_EC_ID_MISMATCH:                    return "29 disk ID mismatch";
        case DOS_EC_SYNTAX_ERROR_MALFORMED:         return "30 syntax error (malformed command)";
        case DOS_EC_SYNTAX_ERROR_UNKNOWN_COMMAND:   return "31 syntax error (unknown command)";
        case DOS_EC_SYNTAX_ERRPR_COMMAND_TOO_LONG:  return "32 syntax error (command too long)";
        case DOS_EC_SYNTAX_ERROR_ILLEGAL_JOKER:     return "33 syntax error (illegal joker)";
        case DOS_EC_SYNTAX_ERROR_NO_FILENAME:       return "34 syntax error (filename not found)";
        case DOS_EC_FILE_NOT_FOUND_USR:             return "39 file not found (USR)";
        case DOS_EC_RECORD_NOT_PRESENT:             return "50 record not present";
        case DOS_EC_OVERFLOW_IN_RECORD:             return "51 overflow in record";
        case DOS_EC_FILE_TOO_LARGE:                 return "52 file too large";
        case DOS_EC_WRITE_FILE_OPEN:                return "60 write file open (not closed)";
        case DOS_EC_FILE_NOT_OPEN:                  return "61 file not open";
        case DOS_EC_FILE_NOT_FOUND:                 return "62 file not found";
        case DOS_EC_FILE_EXISTS:                    return "63 file exists";
        case DOS_EC_FILE_TYPE_MISMATCH:             return "64 file type mismatch";
        case DOS_EC_NO_BLOCK:                       return "65 no block (already occupied)";
        case DOS_EC_ILLEGAL_TOS_DISK:               return "66 illegal track or sector (disk)";
        case DOS_EC_ILLEGAL_TOS_FILE:               return "67 illegal track or sector (file)";
        case DOS_EC_NO_CHANNEL:                     return "70 no channel";
        case DOS_EC_DIR_ERROR:                      return "71 dir error (DOS vs BAM mismatch)";
        case DOS_EC_DISK_FULL:                      return "72 disk full";
        case DOS_EC_VERSION:                        return "73 CBM DOS version report or mismatch";
        case DOS_EC_DRIVE_NOT_READY:                return "74 drive not ready";
        default:                                    return "undefined DOS code";
    }
}



const DOS_ERROR_CODE JOB_EC_TO_DOS_EC[16] = {DOS_EC_NA,
                                             DOS_EC_OK,
                                             DOS_EC_READ_ERROR_20,
                                             DOS_EC_READ_ERROR_21,
                                             DOS_EC_READ_ERROR_22,
                                             DOS_EC_READ_ERROR_23,
                                             DOS_EC_NA,
                                             DOS_EC_WRITE_ERROR_DATA,
                                             DOS_EC_WRITE_PROTECT_ON,
                                             DOS_EC_READ_ERROR_27,
                                             DOS_EC_NA,
                                             DOS_EC_ID_MISMATCH,
                                             DOS_EC_NA,
                                             DOS_EC_NA,
                                             DOS_EC_NA,
                                             DOS_EC_DRIVE_NOT_READY};

