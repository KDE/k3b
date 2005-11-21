/*
 * Firmware flash utility for BTC DRW1008 DVD+/-RW recorder
 * Version 2004/04/29
 * By David Huang <khym@azeotrope.org>
 * This work is dedicated to the public domain
 *
 * This utility may also work with other BTC DVD recorders, such as
 * the DRW1004 and DRW1108, but they have not been tested.
 *
 * USE AT YOUR OWN RISK! 
 * btcflash is provided AS IS, with NO WARRANTY, either expressed or implied.
 *
 * Requires "transport.hxx" from Andy Polyakov's DVD+RW tools:
 * http://fy.chalmers.se/~appro/linux/DVD+RW/tools/
 * If obtained as part of dvd+rw-tools it can be built with
 * 'make +btcflash'.
 *
 * Firmware files may be obtained by running BTC's Windows flash
 * utility, then searching in the WINDOWS\TEMP or WINNT\TEMP directory
 * for a *.HEX file. It will probably be in a subdirectory named
 * PAC*.tmp.DIR, and the HEX file will be named Vnnnn.HEX, where nnnn
 * is the firmware version number. You'll also find IDEFLASH.EXE or
 * BTCFLASH.EXE in the same directory.
 *
 * This utility will also accept firmware files in ".BIN" format.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "transport.hxx"

const unsigned int FLASHSIZE = 0x100000;	/* BTC flash is 1MB */

unsigned char *loadfirmware(const char *);
int getbyte(char *&);
unsigned short calcsum(unsigned char *);

unsigned char *
loadfirmware(const char *firmware)
{
	FILE *f;
	char line[80], *p;
	unsigned char *fwbuf;
	int bank, length, offset, type, hexsum;
	int i, b;

	fwbuf = new unsigned char[FLASHSIZE];
	if (!fwbuf) {
		fprintf(stderr, "Could not allocate memory for firmware\n");
		return NULL;
	}

	f = fopen(firmware, "r");
	if (!f) {
		fprintf(stderr, "%s: Unable to open: ", firmware);
		perror(NULL);
		return NULL;
	}

	// Get length of file. If it's exactly FLASHSIZE, assume it's a
	// .bin file. Otherwise, try to read it as a .hex file.
	fseek(f, 0, SEEK_END);
	if (ftell(f) == FLASHSIZE) {
		rewind(f);
		if (fread(fwbuf, 1, FLASHSIZE, f) != FLASHSIZE) {
			fprintf(stderr, "%s: Short read\n", firmware);
			return NULL;
		}
		fclose(f);
		return fwbuf;
	}

	rewind(f);
	memset(fwbuf, 0xff, FLASHSIZE);

	bank = 0;
	while (fgets(line, sizeof(line), f)) {
		if (line[0] != ':')
			continue;

		p = line + 1;

		length = getbyte(p);
		offset = getbyte(p) << 8 | getbyte(p);
		type = getbyte(p);
		if (length < 0 || offset < 0 || type < 0 ||
		    (type != 0 && length != 0)) {
			fprintf(stderr, "Malformed line: %s", line);
			return NULL;
		} else if (length == 0) {
			if (strncmp(line, ":00000155AA", 11) == 0) {
				if (++bank >= 16) {
					fprintf(stderr,
					    "Firmware file larger than 1MB\n");
					return NULL;
				}
				continue;
			} else if (strncmp(line, ":00000001FF", 11) == 0)
				break;
			else {
				fprintf(stderr, "Malformed line: %s", line);
				return NULL;
			}
		}

		hexsum = (length + (offset >> 8) + (offset & 0xff)) & 0xff;
		for (i = 0; i < length; i++, offset++) {
			b = getbyte(p);
			hexsum = (hexsum + b) & 0xff;
			if (b < 0) {
				fprintf(stderr, "Short line: %s", line);
				return NULL;
			}
			fwbuf[(bank << 16) | offset] = (char)b;
		}
		hexsum = (0x100 - hexsum) & 0xff;
		if (hexsum != getbyte(p)) {
			fprintf(stderr, "Checksum mismatch: %s", line);
			return NULL;
		}
			
	}

	fclose(f);

	if (bank != 15) {
		fprintf(stderr, "Firmware file too small\n");
		return NULL;
	}

	return fwbuf;
}

int
getbyte(char *&p)
{
	int h, l;

	h = *p;
	if (h >= '0' && h <= '9')
		h -= '0';
	else if (h >= 'A' && h <= 'F')
		h -= 'A' - 10;
	else if (h >= 'a' && h <= 'f')
		h -= 'a' - 10;
	else
		return -1;

	l = *(p+1);
	if (l >= '0' && l <= '9')
		l -= '0';
	else if (l >= 'A' && l <= 'F')
		l -= 'A' - 10;
	else if (l >= 'a' && l <= 'f')
		l -= 'a' - 10;
	else
		return -1;

	p += 2;
	return (h << 4) | l;
}

unsigned short
calcsum(unsigned char *fwbuf)
{
	unsigned int flashsum, i;
		    
	for(flashsum = 0, i = 0; i < FLASHSIZE; i++)
		flashsum += fwbuf[i];

	return (flashsum & 0xffff);
}

int main(int argc, char *argv[])
{
	const char *fwfile;
	char confirm[5];
	unsigned char *fwbuf, inq[128], csbuf[32];
	unsigned short checksum;
	Scsi_Command  cmd;
	int err;
	unsigned int offset;

	if (argc < 3) {
		fprintf(stderr, "Usage: %s /dev/cdrom firmware\n",
		    argv[0]);
		return 1;
	}

	printf("BTC DVD+/-RW firmware flash utility 2004/04/29\n");
	printf("USE AT YOUR OWN RISK!\n\n");

	if (!cmd.associate(argv[1])) {
		fprintf(stderr, "%s: unable to open: ", argv[1]);
		perror (NULL);
		return 1;
	}

	fwfile = argv[2];

	if (!(fwbuf = loadfirmware(fwfile)))
		return 1;

	checksum = calcsum(fwbuf);

	printf("Loaded firmware from %s\nFirmware checksum is %04X\n",
	    fwfile, checksum);

	cmd[0] = 0x12;	// INQUIRY
	cmd[4] = 36;
	cmd[5] = 0;
	if (err = cmd.transport(READ, inq, 36)) {
		sperror("INQUIRY", err);
		return 1;
	}

	printf("Drive is currently:     [%.8s][%.16s][%.4s]\n",
	    inq+8, inq+16, inq+32);
	printf("Firmware appears to be: [%.8s][%.16s][%.4s]\n\n",
	    fwbuf+0x40bc, fwbuf+0x40c4, fwbuf+0x40d4);

	if (strncmp((char*)inq + 8, (char*)fwbuf + 0x40bc, 24) != 0)
		printf(
		    "***********************************************"
		    "***********\n"
		    "WARNING! THIS FIRMWARE DOES NOT SEEM TO BE FOR "
		    "THIS DRIVE!\n"
		    "***********************************************"
		    "***********\n");

	printf("Type \"YES\" to proceed with flash: ");
	fflush(stdout);
	fgets(confirm, sizeof(confirm), stdin);
	if (strcmp(confirm, "YES\n") != 0) {
		printf("\nFlash canceled.\n");
		return 0;
	}

	printf("\nUploading firmware...\n");

	// Upload firmware
	for (offset = 0; offset < FLASHSIZE; offset += 0x1000) {
		cmd[0] = 0x3B;	// WRITE BUFFER
		cmd[1] = 6;	// Download Microcode with Offsets
		cmd[2] = 0;	// Buffer ID 0
		cmd[3] = (offset >> 16) & 0xff;
		cmd[4] = (offset >> 8) & 0xff;
		cmd[5] = 0x20;
		cmd[6] = 0;	// Length 0x1000
		cmd[7] = 0x10;
		cmd[8] = 0;
		cmd[9] = 0;
		if (err = cmd.transport(WRITE, fwbuf + offset, 0x1000)) {
			sperror("WRITE BUFFER[1]", err);
			return 1;
		}
	}

	// Upload checksum
	memset(csbuf, 0, 32);
	csbuf[30] = (checksum >> 8);
	csbuf[31] = (checksum & 0xff);
	cmd[0] = 0x3B;	// WRITE BUFFER
	cmd[1] = 6;	// Download Microcode with Offsets
	cmd[2] = 0;	// Buffer ID 0
	cmd[3] = 0;	// Offset 0
	cmd[4] = 0;
	cmd[5] = 0;
	cmd[6] = 0;	// Length 0x20
	cmd[7] = 0;
	cmd[8] = 0x20;
	cmd[9] = 0;
	if (err = cmd.transport(WRITE, csbuf, 0x20)) {
		sperror("WRITE BUFFER[2]", err);
		return 1;
	}

	printf("Flashing drive...\n");

	// Firmware uploaded; now flash it!
	cmd[0] = 0x3B;	// WRITE BUFFER
	cmd[1] = 7;	// Download Microcode with Offsets and Save
	cmd[2] = 0;	// Buffer ID 0
	cmd[3] = 0;	// Offset 0
	cmd[4] = 0;
	cmd[5] = 0;
	cmd[6] = 0;	// Length 0
	cmd[7] = 0;
	cmd[8] = 0;
	cmd[9] = 0;
	if (err = cmd.transport()) {
		sperror("WRITE BUFFER[3]", err);
		return 1;
	}

	sleep(50);	// Let drive sit for a while before bothering it

	while (1) {
		sleep(1);
		cmd[0] = 0;	// TEST UNIT READY
		cmd[5] = 0;
		err = cmd.transport();

		// Wait until it returns either ready or
		// not ready/medium not present
		if ((err == 0) || (SK(err) == 2 && ASC(err) == 0x3A))
			break;
	}

	cmd[0] = 0x12;	// INQUIRY
	cmd[4] = 36;
	cmd[5] = 0;
	if (err = cmd.transport(READ, inq, 36)) {
		sperror("INQUIRY[2]", err);
		return 1;
	}

	printf("Drive is now:           [%.8s][%.16s][%.4s]\n\n",
	    inq+8, inq+16, inq+32);
	printf("Please reboot before using the drive.\n");

	return 0;
}
