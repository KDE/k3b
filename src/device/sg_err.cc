#include <stdio.h>
#include <stdlib.h>
#include "sg_err.h"
#include "util.h"
#include <qobject.h>
/* This file is a huge cut, paste and hack from linux/drivers/scsi/constant.c
*  which I guess was written by:
*         Copyright (C) 1993, 1994, 1995 Eric Youngdale

* The rest of this is:
*  Copyright (C) 1999 D. Gilbert
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  ASCII values for a number of symbolic constants, printing functions, etc.
*
*  Some of the tables have been updated for SCSI 2.
*
*  Version 0.61 (990519)
*/


static const unsigned char scsi_command_size[8] = { 6, 10, 10, 12,
                                                   12, 12, 10, 10 };

#define COMMAND_SIZE(opcode) scsi_command_size[((opcode) >> 5) & 7]

static const char unknown[] = "UNKNOWN";

static const char * group_0_commands[] = {
/* 00-03 */ "Test Unit Ready", "Rezero Unit", unknown, "Request Sense",
/* 04-07 */ "Format Unit", "Read Block Limits", unknown, "Reasssign Blocks",
/* 08-0d */ "Read (6)", unknown, "Write (6)", "Seek (6)", unknown, unknown,
/* 0e-12 */ unknown, "Read Reverse", "Write Filemarks", "Space", "Inquiry",  
/* 13-16 */ "Verify", "Recover Buffered Data", "Mode Select", "Reserve",
/* 17-1b */ "Release", "Copy", "Erase", "Mode Sense", "Start/Stop Unit",
/* 1c-1d */ "Receive Diagnostic", "Send Diagnostic", 
/* 1e-1f */ "Prevent/Allow Medium Removal", unknown,
};


static const char *group_1_commands[] = {
/* 20-22 */  unknown, unknown, unknown,
/* 23-28 */ unknown, "Define window parameters", "Read Capacity", 
            unknown, unknown, "Read (10)", 
/* 29-2d */ "Read Generation", "Write (10)", "Seek (10)", "Erase", 
            "Read updated block", 
/* 2e-31 */ "Write Verify","Verify", "Search High", "Search Equal", 
/* 32-34 */ "Search Low", "Set Limits", "Prefetch or Read Position", 
/* 35-37 */ "Synchronize Cache","Lock/Unlock Cache", "Read Defect Data", 
/* 38-3c */ "Medium Scan", "Compare", "Copy Verify", "Write Buffer", 
            "Read Buffer", 
/* 3d-3f */ "Update Block", "Read Long",  "Write Long",
};

static const char *group_2_commands[] = {
/* 40-41 */ "Change Definition", "Write Same", 
/* 42-48 */ "Read sub-channel", "Read TOC", "Read header", 
            "Play audio (10)", unknown, "Play audio msf",
            "Play audio track/index", 
/* 49-4f */ "Play track relative (10)", unknown, "Pause/resume", 
            "Log Select", "Log Sense", unknown, unknown,
/* 50-55 */ unknown, unknown, unknown, unknown, unknown, "Mode Select (10)",
/* 56-5b */ unknown, unknown, unknown, unknown, "Mode Sense (10)", unknown,
/* 5c-5f */ unknown, unknown, unknown,
};


/* The following are 12 byte commands in group 5 */
static const char *group_5_commands[] = {
/* a0-a5 */ unknown, unknown, unknown, unknown, unknown,
            "Move medium/play audio(12)",
/* a6-a9 */ "Exchange medium", unknown, "Read(12)", "Play track relative(12)", 
/* aa-ae */ "Write(12)", unknown, "Erase(12)", unknown, 
            "Write and verify(12)", 
/* af-b1 */ "Verify(12)", "Search data high(12)", "Search data equal(12)",
/* b2-b4 */ "Search data low(12)", "Set limits(12)", unknown,
/* b5-b6 */ "Request volume element address", "Send volume tag",
/* b7-b9 */ "Read defect data(12)", "Read element status", unknown,
/* ba-bf */ unknown, unknown, unknown, unknown, unknown, unknown,
};




#define group(opcode) (((opcode) >> 5) & 7)

#define RESERVED_GROUP  0
#define VENDOR_GROUP    1

static const char **commands[] = {
    group_0_commands, group_1_commands, group_2_commands, 
    (const char **) RESERVED_GROUP, (const char **) RESERVED_GROUP, 
    group_5_commands, (const char **) VENDOR_GROUP, 
    (const char **) VENDOR_GROUP
};

static const char reserved[] = "RESERVED";
static const char vendor[] = "VENDOR SPECIFIC";

static void print_opcode(int opcode) {
    const char **table = commands[ group(opcode) ];
    switch ((unsigned long) table) {
    case RESERVED_GROUP:
        qDebug( "%s(0x%02x) ", reserved, opcode);
        break;
    case VENDOR_GROUP:
        qDebug( "%s(0x%02x) ", vendor, opcode);
        break;
    default:
        if (table[opcode & 0x1f] != unknown)
	    qDebug( "%s ",table[opcode & 0x1f]);
        else
            qDebug( "%s(0x%02x) ", unknown, opcode);
        break;
    }
}

void sg_print_command (const unsigned char * command) {
    int i,s;
    print_opcode(command[0]);
    for ( i = 1, s = COMMAND_SIZE(command[0]); i < s; ++i)
        qDebug( "%02x ", command[i]);
    qDebug( "");
}

static const char * statuses[] = {
/* 0-4 */ "Good", "Check Condition", "Condition Met", unknown, "Busy",
/* 5-9 */ unknown, unknown, unknown, "Intermediate", unknown,
/* a-c */ "Intermediate-Condition Met", unknown, "Reservation Conflict",
/* d-10 */ unknown, unknown, unknown, unknown,
/* 11-14 */ "Command Terminated", unknown, unknown, "Queue Full",
/* 15-1a */ unknown, unknown, unknown,  unknown, unknown, unknown,
/* 1b-1f */ unknown, unknown, unknown,  unknown, unknown,
};

void sg_print_target_status (int target_status) {
    /* status = (status >> 1) & 0xf; */ /* already done */
    qDebug( "%s ",statuses[target_status]);
}

#define D 0x001  /* DIRECT ACCESS DEVICE (disk) */
#define T 0x002  /* SEQUENTIAL ACCESS DEVICE (tape) */
#define L 0x004  /* PRINTER DEVICE */
#define P 0x008  /* PROCESSOR DEVICE */
#define W 0x010  /* WRITE ONCE READ MULTIPLE DEVICE */
#define R 0x020  /* READ ONLY (CD-ROM) DEVICE */
#define S 0x040  /* SCANNER DEVICE */
#define O 0x080  /* OPTICAL MEMORY DEVICE */
#define M 0x100  /* MEDIA CHANGER DEVICE */
#define C 0x200  /* COMMUNICATION DEVICE */

struct error_info{
    unsigned char code1, code2;
    unsigned short int devices;
    const char * text;
};

struct error_info2{
    unsigned char code1, code2_min, code2_max;
    unsigned short int devices;
    const char * text;
};

static struct error_info2 additional2[] =
{
  {0x40,0x00,0x7f,D,"Ram failure (%x)"},
  {0x40,0x80,0xff,D|T|L|P|W|R|S|O|M|C,"Diagnostic failure on component (%x)"},
  {0x41,0x00,0xff,D,"Data path failure (%x)"},
  {0x42,0x00,0xff,D,"Power-on or self-test failure (%x)"},
  {0, 0, 0, 0, NULL}
};

static struct error_info additional[] =
{
  {0x00,0x01,T,"Filemark detected"},
  {0x00,0x02,T|S,"End-of-partition/medium detected"},
  {0x00,0x03,T,"Setmark detected"},
  {0x00,0x04,T|S,"Beginning-of-partition/medium detected"},
  {0x00,0x05,T|S,"End-of-data detected"},
  {0x00,0x06,D|T|L|P|W|R|S|O|M|C,"I/O process terminated"},
  {0x00,0x11,R,"Audio play operation in progress"},
  {0x00,0x12,R,"Audio play operation paused"},
  {0x00,0x13,R,"Audio play operation successfully completed"},
  {0x00,0x14,R,"Audio play operation stopped due to error"},
  {0x00,0x15,R,"No current audio status to return"},
  {0x01,0x00,D|W|O,"No index/sector signal"},
  {0x02,0x00,D|W|R|O|M,"No seek complete"},
  {0x03,0x00,D|T|L|W|S|O,"Peripheral device write fault"},
  {0x03,0x01,T,"No write current"},
  {0x03,0x02,T,"Excessive write errors"},
  {0x04,0x00,D|T|L|P|W|R|S|O|M|C,
     "Logical unit not ready, cause not reportable"},
  {0x04,0x01,D|T|L|P|W|R|S|O|M|C,
     "Logical unit is in process of becoming ready"},
  {0x04,0x02,D|T|L|P|W|R|S|O|M|C,
     "Logical unit not ready, initializing command required"},
  {0x04,0x03,D|T|L|P|W|R|S|O|M|C,
     "Logical unit not ready, manual intervention required"},
  {0x04,0x04,D|T|L|O,"Logical unit not ready, format in progress"},
  {0x05,0x00,D|T|L|W|R|S|O|M|C,"Logical unit does not respond to selection"},
  {0x06,0x00,D|W|R|O|M,"No reference position found"},
  {0x07,0x00,D|T|L|W|R|S|O|M,"Multiple peripheral devices selected"},
  {0x08,0x00,D|T|L|W|R|S|O|M|C,"Logical unit communication failure"},
  {0x08,0x01,D|T|L|W|R|S|O|M|C,"Logical unit communication time-out"},
  {0x08,0x02,D|T|L|W|R|S|O|M|C,"Logical unit communication parity error"},
  {0x09,0x00,D|T|W|R|O,"Track following error"},
  {0x09,0x01,W|R|O,"Tracking servo failure"},
  {0x09,0x02,W|R|O,"Focus servo failure"},
  {0x09,0x03,W|R|O,"Spindle servo failure"},
  {0x0A,0x00,D|T|L|P|W|R|S|O|M|C,"Error log overflow"},
  {0x0C,0x00,T|S,"Write error"},
  {0x0C,0x01,D|W|O,"Write error recovered with auto reallocation"},
  {0x0C,0x02,D|W|O,"Write error - auto reallocation failed"},
  {0x10,0x00,D|W|O,"Id crc or ecc error"},
  {0x11,0x00,D|T|W|R|S|O,"Unrecovered read error"},
  {0x11,0x01,D|T|W|S|O,"Read retries exhausted"},
  {0x11,0x02,D|T|W|S|O,"Error too long to correct"},
  {0x11,0x03,D|T|W|S|O,"Multiple read errors"},
  {0x11,0x04,D|W|O,"Unrecovered read error - auto reallocate failed"},
  {0x11,0x05,W|R|O,"L-ec uncorrectable error"},
  {0x11,0x06,W|R|O,"Circ unrecovered error"},
  {0x11,0x07,W|O,"Data resynchronization error"},
  {0x11,0x08,T,"Incomplete block read"},
  {0x11,0x09,T,"No gap found"},
  {0x11,0x0A,D|T|O,"Miscorrected error"},
  {0x11,0x0B,D|W|O,"Unrecovered read error - recommend reassignment"},
  {0x11,0x0C,D|W|O,"Unrecovered read error - recommend rewrite the data"},
  {0x12,0x00,D|W|O,"Address mark not found for id field"},
  {0x13,0x00,D|W|O,"Address mark not found for data field"},
  {0x14,0x00,D|T|L|W|R|S|O,"Recorded entity not found"},
  {0x14,0x01,D|T|W|R|O,"Record not found"},
  {0x14,0x02,T,"Filemark or setmark not found"},
  {0x14,0x03,T,"End-of-data not found"},
  {0x14,0x04,T,"Block sequence error"},
  {0x15,0x00,D|T|L|W|R|S|O|M,"Random positioning error"},
  {0x15,0x01,D|T|L|W|R|S|O|M,"Mechanical positioning error"},
  {0x15,0x02,D|T|W|R|O,"Positioning error detected by read of medium"},
  {0x16,0x00,D|W|O,"Data synchronization mark error"},
  {0x17,0x00,D|T|W|R|S|O,"Recovered data with no error correction applied"},
  {0x17,0x01,D|T|W|R|S|O,"Recovered data with retries"},
  {0x17,0x02,D|T|W|R|O,"Recovered data with positive head offset"},
  {0x17,0x03,D|T|W|R|O,"Recovered data with negative head offset"},
  {0x17,0x04,W|R|O,"Recovered data with retries and/or circ applied"},
  {0x17,0x05,D|W|R|O,"Recovered data using previous sector id"},
  {0x17,0x06,D|W|O,"Recovered data without ecc - data auto-reallocated"},
  {0x17,0x07,D|W|O,"Recovered data without ecc - recommend reassignment"},
  {0x18,0x00,D|T|W|R|O,"Recovered data with error correction applied"},
  {0x18,0x01,D|W|R|O,"Recovered data with error correction and retries applied"},
  {0x18,0x02,D|W|R|O,"Recovered data - data auto-reallocated"},
  {0x18,0x03,R,"Recovered data with circ"},
  {0x18,0x04,R,"Recovered data with lec"},
  {0x18,0x05,D|W|R|O,"Recovered data - recommend reassignment"},
  {0x19,0x00,D|O,"Defect list error"},
  {0x19,0x01,D|O,"Defect list not available"},
  {0x19,0x02,D|O,"Defect list error in primary list"},
  {0x19,0x03,D|O,"Defect list error in grown list"},
  {0x1A,0x00,D|T|L|P|W|R|S|O|M|C,"Parameter list length error"},
  {0x1B,0x00,D|T|L|P|W|R|S|O|M|C,"Synchronous data transfer error"},
  {0x1C,0x00,D|O,"Defect list not found"},
  {0x1C,0x01,D|O,"Primary defect list not found"},
  {0x1C,0x02,D|O,"Grown defect list not found"},
  {0x1D,0x00,D|W|O,"Miscompare during verify operation"},
  {0x1E,0x00,D|W|O,"Recovered id with ecc correction"},
  {0x20,0x00,D|T|L|P|W|R|S|O|M|C,"Invalid command operation code"},
  {0x21,0x00,D|T|W|R|O|M,"Logical block address out of range"},
  {0x21,0x01,M,"Invalid element address"},
  {0x22,0x00,D,"Illegal function (should use 20 00, 24 00, or 26 00)"},
  {0x24,0x00,D|T|L|P|W|R|S|O|M|C,"Invalid field in cdb"},
  {0x25,0x00,D|T|L|P|W|R|S|O|M|C,"Logical unit not supported"},
  {0x26,0x00,D|T|L|P|W|R|S|O|M|C,"Invalid field in parameter list"},
  {0x26,0x01,D|T|L|P|W|R|S|O|M|C,"Parameter not supported"},
  {0x26,0x02,D|T|L|P|W|R|S|O|M|C,"Parameter value invalid"},
  {0x26,0x03,D|T|L|P|W|R|S|O|M|C,"Threshold parameters not supported"},
  {0x27,0x00,D|T|W|O,"Write protected"},
  {0x28,0x00,D|T|L|P|W|R|S|O|M|C,"Not ready to ready transition (medium may have changed)"},
  {0x28,0x01,M,"Import or export element accessed"},
  {0x29,0x00,D|T|L|P|W|R|S|O|M|C,"Power on, reset, or bus device reset occurred"},
  {0x2A,0x00,D|T|L|W|R|S|O|M|C,"Parameters changed"},
  {0x2A,0x01,D|T|L|W|R|S|O|M|C,"Mode parameters changed"},
  {0x2A,0x02,D|T|L|W|R|S|O|M|C,"Log parameters changed"},
  {0x2B,0x00,D|T|L|P|W|R|S|O|C,"Copy cannot execute since host cannot disconnect"},
  {0x2C,0x00,D|T|L|P|W|R|S|O|M|C,"Command sequence error"},
  {0x2C,0x01,S,"Too many windows specified"},
  {0x2C,0x02,S,"Invalid combination of windows specified"},
  {0x2D,0x00,T,"Overwrite error on update in place"},
  {0x2F,0x00,D|T|L|P|W|R|S|O|M|C,"Commands cleared by another initiator"},
  {0x30,0x00,D|T|W|R|O|M,"Incompatible medium installed"},
  {0x30,0x01,D|T|W|R|O,"Cannot read medium - unknown format"},
  {0x30,0x02,D|T|W|R|O,"Cannot read medium - incompatible format"},
  {0x30,0x03,D|T,"Cleaning cartridge installed"},
  {0x31,0x00,D|T|W|O,"Medium format corrupted"},
  {0x31,0x01,D|L|O,"Format command failed"},
  {0x32,0x00,D|W|O,"No defect spare location available"},
  {0x32,0x01,D|W|O,"Defect list update failure"},
  {0x33,0x00,T,"Tape length error"},
  {0x36,0x00,L,"Ribbon, ink, or toner failure"},
  {0x37,0x00,D|T|L|W|R|S|O|M|C,"Rounded parameter"},
  {0x39,0x00,D|T|L|W|R|S|O|M|C,"Saving parameters not supported"},
  {0x3A,0x00,D|T|L|W|R|S|O|M,"Medium not present"},
  {0x3B,0x00,T|L,"Sequential positioning error"},
  {0x3B,0x01,T,"Tape position error at beginning-of-medium"},
  {0x3B,0x02,T,"Tape position error at end-of-medium"},
  {0x3B,0x03,L,"Tape or electronic vertical forms unit not ready"},
  {0x3B,0x04,L,"Slew failure"},
  {0x3B,0x05,L,"Paper jam"},
  {0x3B,0x06,L,"Failed to sense top-of-form"},
  {0x3B,0x07,L,"Failed to sense bottom-of-form"},
  {0x3B,0x08,T,"Reposition error"},
  {0x3B,0x09,S,"Read past end of medium"},
  {0x3B,0x0A,S,"Read past beginning of medium"},
  {0x3B,0x0B,S,"Position past end of medium"},
  {0x3B,0x0C,S,"Position past beginning of medium"},
  {0x3B,0x0D,M,"Medium destination element full"},
  {0x3B,0x0E,M,"Medium source element empty"},
  {0x3D,0x00,D|T|L|P|W|R|S|O|M|C,"Invalid bits in identify message"},
  {0x3E,0x00,D|T|L|P|W|R|S|O|M|C,"Logical unit has not self-configured yet"},
  {0x3F,0x00,D|T|L|P|W|R|S|O|M|C,"Target operating conditions have changed"},
  {0x3F,0x01,D|T|L|P|W|R|S|O|M|C,"Microcode has been changed"},
  {0x3F,0x02,D|T|L|P|W|R|S|O|M|C,"Changed operating definition"},
  {0x3F,0x03,D|T|L|P|W|R|S|O|M|C,"Inquiry data has changed"},
  {0x43,0x00,D|T|L|P|W|R|S|O|M|C,"Message error"},
  {0x44,0x00,D|T|L|P|W|R|S|O|M|C,"Internal target failure"},
  {0x45,0x00,D|T|L|P|W|R|S|O|M|C,"Select or reselect failure"},
  {0x46,0x00,D|T|L|P|W|R|S|O|M|C,"Unsuccessful soft reset"},
  {0x47,0x00,D|T|L|P|W|R|S|O|M|C,"Scsi parity error"},
  {0x48,0x00,D|T|L|P|W|R|S|O|M|C,"Initiator detected error message received"},
  {0x49,0x00,D|T|L|P|W|R|S|O|M|C,"Invalid message error"},
  {0x4A,0x00,D|T|L|P|W|R|S|O|M|C,"Command phase error"},
  {0x4B,0x00,D|T|L|P|W|R|S|O|M|C,"Data phase error"},
  {0x4C,0x00,D|T|L|P|W|R|S|O|M|C,"Logical unit failed self-configuration"},
  {0x4E,0x00,D|T|L|P|W|R|S|O|M|C,"Overlapped commands attempted"},
  {0x50,0x00,T,"Write append error"},
  {0x50,0x01,T,"Write append position error"},
  {0x50,0x02,T,"Position error related to timing"},
  {0x51,0x00,T|O,"Erase failure"},
  {0x52,0x00,T,"Cartridge fault"},
  {0x53,0x00,D|T|L|W|R|S|O|M,"Media load or eject failed"},
  {0x53,0x01,T,"Unload tape failure"},
  {0x53,0x02,D|T|W|R|O|M,"Medium removal prevented"},
  {0x54,0x00,P,"Scsi to host system interface failure"},
  {0x55,0x00,P,"System resource failure"},
  {0x57,0x00,R,"Unable to recover table-of-contents"},
  {0x58,0x00,O,"Generation does not exist"},
  {0x59,0x00,O,"Updated block read"},
  {0x5A,0x00,D|T|L|P|W|R|S|O|M,"Operator request or state change input (unspecified)"},
  {0x5A,0x01,D|T|W|R|O|M,"Operator medium removal request"},
  {0x5A,0x02,D|T|W|O,"Operator selected write protect"},
  {0x5A,0x03,D|T|W|O,"Operator selected write permit"},
  {0x5B,0x00,D|T|L|P|W|R|S|O|M,"Log exception"},
  {0x5B,0x01,D|T|L|P|W|R|S|O|M,"Threshold condition met"},
  {0x5B,0x02,D|T|L|P|W|R|S|O|M,"Log counter at maximum"},
  {0x5B,0x03,D|T|L|P|W|R|S|O|M,"Log list codes exhausted"},
  {0x5C,0x00,D|O,"Rpl status change"},
  {0x5C,0x01,D|O,"Spindles synchronized"},
  {0x5C,0x02,D|O,"Spindles not synchronized"},
  {0x60,0x00,S,"Lamp failure"},
  {0x61,0x00,S,"Video acquisition error"},
  {0x61,0x01,S,"Unable to acquire video"},
  {0x61,0x02,S,"Out of focus"},
  {0x62,0x00,S,"Scan head positioning error"},
  {0x63,0x00,R,"End of user area encountered on this track"},
  {0x64,0x00,R,"Illegal mode for this track"},
  {0, 0, 0, NULL}
};

static const char *snstext[] = {
    "None",                     /* There is no sense information */
    "Recovered Error",          /* The last command completed successfully
                                   but used error correction */
    "Not Ready",                /* The addressed target is not ready */
    "Medium Error",             /* Data error detected on the medium */
    "Hardware Error",           /* Controller or device failure */
    "Illegal Request",
    "Unit Attention",           /* Removable medium was changed, or
                                   the target has been reset */
    "Data Protect",             /* Access to the data is blocked */
    "Blank Check",              /* Reached unexpected written or unwritten
                                   region of the medium */
    "Key=9",                    /* Vendor specific */
    "Copy Aborted",             /* COPY or COMPARE was aborted */
    "Aborted Command",          /* The target aborted the command */
    "Equal",                    /* A SEARCH DATA command found data equal */
    "Volume Overflow",          /* Medium full with still data to be written */
    "Miscompare",               /* Source data and data on the medium
                                   do not agree */
    "Key=15"                    /* Reserved */
};

/* Print sense information */
void sg_print_sense(const char * leadin, const unsigned char * sense_buffer)
{
    int i, s;
    int sense_class, valid, code;
    const char * error = NULL;

    sense_class = (sense_buffer[0] >> 4) & 0x07;
    code = sense_buffer[0] & 0xf;
    valid = sense_buffer[0] & 0x80;

    if (sense_class == 7) {     /* extended sense data */
        s = sense_buffer[7] + 8;
        if(s > SG_ERR_MAX_SENSE_LEN)
           s = SG_ERR_MAX_SENSE_LEN;

        if (!valid)
            qDebug( "[valid=0] ");
        qDebug( "Info fld=%d, ", (int)((sense_buffer[3] << 24) |
               (sense_buffer[4] << 16) | (sense_buffer[5] << 8) |
               sense_buffer[6]));

        if (sense_buffer[2] & 0x80)
           qDebug( "FMK ");     /* current command has read a filemark */
        if (sense_buffer[2] & 0x40)
           qDebug( "EOM ");     /* end-of-medium condition exists */
        if (sense_buffer[2] & 0x20)
           qDebug( "ILI ");     /* incorrect block length requested */

        switch (code) {
        case 0x0:
            error = "Current";  /* error concerns current command */
            break;
        case 0x1:
            error = "Deferred"; /* error concerns some earlier command */
                /* e.g., an earlier write to disk cache succeeded, but
                   now the disk discovers that it cannot write the data */
            break;
        default:
            error = "Invalid";
        }

        qDebug( "%s ", error);

        if (leadin)
            qDebug( "%s: ", leadin);
        qDebug( "sense key: 0x%02x: %s", sense_buffer[2] & 0x0f,
		snstext[sense_buffer[2] & 0x0f]);

        /* Check to see if additional sense information is available */
        if(sense_buffer[7] + 7 < 13 ||
           (sense_buffer[12] == 0  && sense_buffer[13] ==  0)) goto done;

        for(i=0; additional[i].text; i++)
            if(additional[i].code1 == sense_buffer[12] &&
               additional[i].code2 == sense_buffer[13])
                qDebug( "Additional sense indicates: %s",
                       additional[i].text);

        for(i=0; additional2[i].text; i++)
            if(additional2[i].code1 == sense_buffer[12] &&
               additional2[i].code2_min >= sense_buffer[13]  &&
               additional2[i].code2_max <= sense_buffer[13]) {
                qDebug( "Additional sense indicates: ");
                qDebug( additional2[i].text, sense_buffer[13]);
                qDebug( "");
            };
    } else {    /* non-extended sense data */

         /*
          * Standard says:
          *    sense_buffer[0] & 0200 : address valid
          *    sense_buffer[0] & 0177 : vendor-specific error code
          *    sense_buffer[1] & 0340 : vendor-specific
          *    sense_buffer[1..3] : 21-bit logical block address
          */

        if (leadin)
            qDebug( "%s: ", leadin);
        if (sense_buffer[0] < 15)
            qDebug( "old sense: key %s", snstext[sense_buffer[0] & 0x0f]);
        else
            qDebug( "sns = %2x %2x", sense_buffer[0], sense_buffer[2]);

        qDebug( "Non-extended sense class %d code 0x%0x", sense_class, code);
        s = 4;
    }

 done:
    qDebug( "Raw sense data: ");
    for (i = 0; i < s; ++i) {
        if ((i > 0) && (0 == (i % 10)))
            qDebug( "");
        qDebug( "0x%02x ", sense_buffer[i]);
    }
    qDebug( "");
    return;
}

static const char * hostbyte_table[]={
"DID_OK", "DID_NO_CONNECT", "DID_BUS_BUSY", "DID_TIME_OUT", "DID_BAD_TARGET",
"DID_ABORT", "DID_PARITY", "DID_ERROR", "DID_RESET", "DID_BAD_INTR",
"DID_PASSTHROUGH", "DID_SOFT_ERROR", NULL};

void sg_print_host_status(int host_status)
{   static int maxcode=0;
    int i;

    if(! maxcode) {
        for(i = 0; hostbyte_table[i]; i++) ;
        maxcode = i-1;
    }
    qDebug( "Host_status=0x%02x ", host_status);
    if(host_status > maxcode) {
        qDebug( "is invalid ");
        return;
    }
    qDebug( "(%s) ",hostbyte_table[host_status]);
}

static const char * driverbyte_table[]={
"DRIVER_OK", "DRIVER_BUSY", "DRIVER_SOFT",  "DRIVER_MEDIA", "DRIVER_ERROR",
"DRIVER_INVALID", "DRIVER_TIMEOUT", "DRIVER_HARD", "DRIVER_SENSE", NULL};

static const char * driversuggest_table[]={"SUGGEST_OK",
"SUGGEST_RETRY", "SUGGEST_ABORT", "SUGGEST_REMAP", "SUGGEST_DIE",
unknown,unknown,unknown, "SUGGEST_SENSE",NULL};


void sg_print_driver_status(int driver_status)
{
    static int driver_max =0 , suggest_max=0;
    int i;
    int dr = driver_status & DRIVER_MASK;
    int su = (driver_status & SUGGEST_MASK) >> 4;

    if(! driver_max) {
        for(i = 0; driverbyte_table[i]; i++) ;
        driver_max = i;
        for(i = 0; driversuggest_table[i]; i++) ;
        suggest_max = i;
    }
    qDebug( "Driver_status=0x%02x ",driver_status);
    qDebug( "(%s,%s) ",
        dr < driver_max  ? driverbyte_table[dr]:"invalid",
        su < suggest_max ? driversuggest_table[su]:"invalid");
}

int sg_chk_n_print(const char * leadin, int target_status,
                   int host_status, int driver_status,
                   const unsigned char * sense_buffer)
{
    int done_leadin = 0;
    int done_sense = 0;

    if ((0 == target_status) && (0 == host_status) && (0 == driver_status))
        return 1;       /* No problems */
    if (0 != target_status) {
        if (leadin)
            qDebug( "%s: ", leadin);
        done_leadin = 1;
        sg_print_target_status(target_status);
        qDebug( "");
        if (sense_buffer && ((target_status == CHECK_CONDITION) ||
                             (target_status == COMMAND_TERMINATED))) {
            sg_print_sense(0, sense_buffer);
            done_sense = 1;
        }
    }
    if (0 != host_status) {
        if (leadin && (! done_leadin))
            qDebug( "%s: ", leadin);
        if (done_leadin)
            qDebug( "plus...: ");
        else
            done_leadin = 1;
        sg_print_host_status(host_status);
        qDebug( "");
    }
    if (0 != driver_status) {
        if (leadin && (! done_leadin))
            qDebug( "%s: ", leadin);
        if (done_leadin)
            qDebug( "plus...: ");
        else
            done_leadin = 1;
        sg_print_driver_status(driver_status);
        qDebug( "");
        if (sense_buffer && (! done_sense) && (DRIVER_SENSE & driver_status))
            sg_print_sense(0, sense_buffer);
    }
    return 0;
}

int sg_err_category(int target_status, int host_status, int driver_status,
                    const unsigned char * sense_buffer)
{
    if ((0 == target_status) && (0 == host_status) && (0 == driver_status))
        return SG_ERR_CAT_CLEAN;
    if ((CHECK_CONDITION == target_status) || 
        (COMMAND_TERMINATED == target_status) ||
        (DRIVER_SENSE & driver_status)) {
        if (sense_buffer) {
            if(RECOVERED_ERROR == sense_buffer[2])
                return SG_ERR_CAT_RECOVERED;
            else if (UNIT_ATTENTION == (0x0f & sense_buffer[2])) {
                if (0x28 == sense_buffer[12])
                    return SG_ERR_CAT_MEDIA_CHANGED;
                if (0x29 == sense_buffer[12])
                    return SG_ERR_CAT_RESET;
            }
        }
        return SG_ERR_CAT_SENSE;
    }
    if (0 != host_status) {
        if ((DID_NO_CONNECT == host_status) ||
            (DID_BUS_BUSY == host_status) ||
            (DID_TIME_OUT == host_status))
            return SG_ERR_CAT_TIMEOUT;
    }
    if (0 != driver_status) {
        if (DRIVER_TIMEOUT == driver_status)
            return SG_ERR_CAT_TIMEOUT;
    }
    return SG_ERR_CAT_OTHER;
}

int sg_get_command_size(unsigned char opcode)
{
    return COMMAND_SIZE(opcode);
}
