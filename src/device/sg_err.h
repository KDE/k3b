#ifndef SG_ERR_H
#define SG_ERR_H
#include <linux/../scsi/scsi.h> /* cope with silly includes */

/* Feel free to copy and modify this GPL-ed code into your applications. */

/* Version 0.61 (990519) */


/* Some of the following error/status codes are exchanged between the
   various layers of the SCSI sub-system in Linux and should never
   reach the user. They are placed here for completeness. What appears
   here is copied from drivers/scsi/scsi.h which is not visible in
   the user space. */

/* The following are 'host_status' codes */
#ifndef DID_OK
#define DID_OK 0x00
#endif
#ifndef DID_NO_CONNECT
#define DID_NO_CONNECT 0x01     /* Unable to connect before timeout */
#define DID_BUS_BUSY 0x02       /* Bus remain busy until timeout */
#define DID_TIME_OUT 0x03       /* Timed out for some other reason */
#define DID_BAD_TARGET 0x04     /* Bad target (id?) */
#define DID_ABORT 0x05  /* Told to abort for some other reason */
#define DID_PARITY 0x06 /* Parity error (on SCSI bus) */
#define DID_ERROR 0x07  /* Internal error */
#define DID_RESET 0x08  /* Reset by somebody */
#define DID_BAD_INTR 0x09       /* Received an unexpected interrupt */
#define DID_PASSTHROUGH 0x0a    /* Force command past mid-level */
#define DID_SOFT_ERROR 0x0b     /* The low-level driver wants a retry */
#endif

/* The following are 'driver_status' codes */
#ifndef DRIVER_OK
#define DRIVER_OK 0x00
#endif
#ifndef DRIVER_BUSY
#define DRIVER_BUSY 0x01
#define DRIVER_SOFT 0x02
#define DRIVER_MEDIA 0x03
#define DRIVER_ERROR 0x04

#define DRIVER_INVALID 0x05
#define DRIVER_TIMEOUT 0x06
#define DRIVER_HARD 0x07
#define DRIVER_SENSE 0x08       /* Sense_buffer has been set */

/* Following "suggests" are "or-ed" with one of previous 8 entries */
#define SUGGEST_RETRY 0x10
#define SUGGEST_ABORT 0x20
#define SUGGEST_REMAP 0x30
#define SUGGEST_DIE 0x40
#define SUGGEST_SENSE 0x80
#define SUGGEST_IS_OK 0xff
#endif
#ifndef DRIVER_MASK
#define DRIVER_MASK 0x0f
#endif
#ifndef SUGGEST_MASK
#define SUGGEST_MASK 0xf0
#endif

#define SG_ERR_MAX_SENSE_LEN 16


/* The following "print" functions send ACSII to stdout */
extern void sg_print_command ( const unsigned char *command );
extern void sg_print_sense ( const char *leadin,
                             const unsigned char *sense_buffer );
extern void sg_print_target_status ( int target_status );
extern void sg_print_host_status ( int host_status );
extern void sg_print_driver_status ( int driver_status );

/* sg_chk_n_print() returns 1 quietly if there are no errors/warnings
   else it prints to standard output and returns 0. */
extern int sg_chk_n_print ( const char *leadin, int target_status,
                            int host_status, int driver_status,
                            const unsigned char *sense_buffer );


/* The following "category" function returns one of the following */
#define SG_ERR_CAT_CLEAN 0      /* No errors or other information */
#define SG_ERR_CAT_MEDIA_CHANGED 1      /* interpreted from sense buffer */
#define SG_ERR_CAT_RESET 2      /* interpreted from sense buffer */
#define SG_ERR_CAT_TIMEOUT 3
#define SG_ERR_CAT_RECOVERED 4  /* Successful command after recovered err */
#define SG_ERR_CAT_SENSE 98     /* Something else is in the sense buffer */
#define SG_ERR_CAT_OTHER 99     /* Some other error/warning has occurred */

extern int sg_err_category ( int target_status, int host_status,
                             int driver_status,
                             const unsigned char *sense_buffer );


/* Returns length of SCSI command given the opcode (first byte) */
int sg_get_command_size ( unsigned char opcode );

#endif
