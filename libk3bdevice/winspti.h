#ifndef WINSPTI_H
#define WINSPTI_H
/*
 *
 * winspti.h
 * SPDX-FileCopyrightText: 2007 Jeremy C. Andrus <jeremy@jeremya.com>
 *
 * This file is part of the K3b project.
 * SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * Parts of this file are inspired (and copied) from various source
 * files in the cdrdao project (C) J. Schilling, Andreas Mueller
 * and many others.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * See the file "COPYING" for the exact licensing terms.
 */

/*------------------------------------------------------------------------------
 winspti.h
------------------------------------------------------------------------------*/
#include <windows.h>

#define	SENSE_LEN_SPTI          36	/* Sense length for ASPI is only 14 */
#define	NUM_MAX_NTSCSI_DRIVES   26	/* a: ... z:			*/
#define	NUM_FLOPPY_DRIVES       2
#define	NUM_MAX_NTSCSI_HA       NUM_MAX_NTSCSI_DRIVES

#define	NTSCSI_HA_INQUIRY_SIZE  36

#define	SCSI_CMD_INQUIRY        0x12

typedef struct
{
  BYTE    ha;           /* SCSI Bus #			*/
  BYTE    tgt;          /* SCSI Target #		*/
  BYTE    lun;          /* SCSI Lun #			*/
  BYTE    PortNumber;   /* SCSI Card # (\\.\SCSI%d)	*/
  BYTE    PathId;       /* SCSI Bus/Channel # on card n	*/
  BYTE    driveLetter;  /* Win32 drive letter (e.g. c:)	*/
  BOOL    bUsed;        /* Win32 drive letter is used	*/
  HANDLE  hDevice;      /* Win32 handle for ioctl()	*/
  BYTE    inqData[NTSCSI_HA_INQUIRY_SIZE];
} SPTI_Drive;

typedef struct {
	USHORT		Length;
	UCHAR		ScsiStatus;
	UCHAR		PathId;
	UCHAR		TargetId;
	UCHAR		Lun;
	UCHAR		CdbLength;
	UCHAR		SenseInfoLength;
	UCHAR		DataIn;
	ULONG		DataTransferLength;
	ULONG		TimeOutValue;
	ULONG		DataBufferOffset;
	ULONG		SenseInfoOffset;
	UCHAR		Cdb[16];
} SCSI_PASS_THROUGH, *PSCSI_PASS_THROUGH;


typedef struct {
	USHORT		Length;
	UCHAR		ScsiStatus;
	UCHAR		PathId;
	UCHAR		TargetId;
	UCHAR		Lun;
	UCHAR		CdbLength;
	UCHAR		SenseInfoLength;
	UCHAR		DataIn;
	ULONG		DataTransferLength;
	ULONG		TimeOutValue;
	PVOID		DataBuffer;
	ULONG		SenseInfoOffset;
	UCHAR		Cdb[16];
} SCSI_PASS_THROUGH_DIRECT, *PSCSI_PASS_THROUGH_DIRECT;


typedef struct {
	SCSI_PASS_THROUGH spt;
	ULONG		Filler;
	UCHAR		ucSenseBuf[SENSE_LEN_SPTI];
	UCHAR		ucDataBuf[512];
} SCSI_PASS_THROUGH_WITH_BUFFERS, *PSCSI_PASS_THROUGH_WITH_BUFFERS;


typedef struct {
	SCSI_PASS_THROUGH_DIRECT spt;
	ULONG		Filler;
	UCHAR		ucSenseBuf[SENSE_LEN_SPTI];
} SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, *PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER;



typedef struct {
	UCHAR		NumberOfLogicalUnits;
	UCHAR		InitiatorBusId;
	ULONG		InquiryDataOffset;
} SCSI_BUS_DATA, *PSCSI_BUS_DATA;


typedef struct {
	BYTE        SD_Error;
	BYTE        SD_Segment;
	BYTE        SD_SenseKey;
	DWORD       SD_Information;
	BYTE        SD_AS_Length;
	DWORD       SD_CommandInformation;
	BYTE        SD_ASC;
	BYTE        SD_ASCQ;
	BYTE        SD_FieldReplaceableUnit;
	BYTE        SD_SpecificKey1;
	BYTE        SD_SpecificKey2;
	BYTE        SD_SpecificKey3;
    BYTE        SD_Data[14];
} SCSI_SENSE_DATA, *PSCSI_SENSE_DATA;



typedef struct {
	UCHAR		NumberOfBusses;
	SCSI_BUS_DATA	BusData[1];
} SCSI_ADAPTER_BUS_INFO, *PSCSI_ADAPTER_BUS_INFO;


typedef struct {
	UCHAR		PathId;
	UCHAR		TargetId;
	UCHAR		Lun;
	BOOLEAN 	DeviceClaimed;
	ULONG		InquiryDataLength;
	ULONG		NextInquiryDataOffset;
	UCHAR		InquiryData[1];
} SCSI_INQUIRY_DATA, *PSCSI_INQUIRY_DATA;


typedef struct {
	ULONG		Length;
	UCHAR		PortNumber;
	UCHAR		PathId;
	UCHAR		TargetId;
	UCHAR		Lun;
} SCSI_ADDRESS, *PSCSI_ADDRESS;


/*
 * method codes
 */
#define	METHOD_BUFFERED		0
#define	METHOD_IN_DIRECT	1
#define	METHOD_OUT_DIRECT	2
#define	METHOD_NEITHER		3

/*
 * file access values
 */
#define	FILE_ANY_ACCESS		0
#define	FILE_READ_ACCESS	0x0001
#define	FILE_WRITE_ACCESS	0x0002


#define	IOCTL_SCSI_BASE    0x00000004

/*
 * constants for DataIn member of SCSI_PASS_THROUGH* structures
 */
#define	SCSI_IOCTL_DATA_OUT		0
#define	SCSI_IOCTL_DATA_IN		1
#define	SCSI_IOCTL_DATA_UNSPECIFIED	2

/*
 * Standard IOCTL define
 */
#define	CTL_CODE(DevType, Function, Method, Access) 		\
	(((DevType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))

#define	IOCTL_SCSI_PASS_THROUGH 	CTL_CODE(IOCTL_SCSI_BASE, 0x0401, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define	IOCTL_SCSI_MINIPORT		CTL_CODE(IOCTL_SCSI_BASE, 0x0402, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define	IOCTL_SCSI_GET_INQUIRY_DATA	CTL_CODE(IOCTL_SCSI_BASE, 0x0403, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	IOCTL_SCSI_GET_CAPABILITIES	CTL_CODE(IOCTL_SCSI_BASE, 0x0404, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	IOCTL_SCSI_PASS_THROUGH_DIRECT	CTL_CODE(IOCTL_SCSI_BASE, 0x0405, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define	IOCTL_SCSI_GET_ADDRESS		CTL_CODE(IOCTL_SCSI_BASE, 0x0406, METHOD_BUFFERED, FILE_ANY_ACCESS)

#endif // WINSPTI_H
