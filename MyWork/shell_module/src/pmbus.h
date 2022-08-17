#ifndef __PMBUS_H
#define __PMBUS_H

#include <zephyr/sys/util_macro.h>


enum pmbus_registers {
	PMBUS_PAGE                      = 0x00, /* R/W byte */
	PMBUS_OPERATION                 = 0x01, /* R/W byte */
    PMBUS_ON_OFF_CONFIG             = 0x02, /* R/W byte */
    PMBUS_CLEAR_FAULTS              = 0x03, /* Send Byte */
    PMBUS_PHASE                     = 0x04, /* R/W byte */
    PMBUS_PAGE_PLUS_WRITE           = 0x05, /* Block Write-only */
    PMBUS_PAGE_PLUS_READ            = 0x06, /* Block Read-only */
    PMBUS_WRITE_PROTECT             = 0x10, /* R/W byte */
    PMBUS_STORE_DEFAULT_ALL         = 0x11, /* Send Byte */
    PMBUS_RESTORE_DEFAULT_ALL       = 0x12, /* Send Byte */
    PMBUS_STORE_DEFAULT_CODE        = 0x13, /* Write-only Byte */
    PMBUS_RESTORE_DEFAULT_CODE      = 0x14, /* Write-only Byte */
    PMBUS_STORE_USER_ALL            = 0x15, /* Send Byte */
    PMBUS_RESTORE_USER_ALL          = 0x16, /* Send Byte */
    PMBUS_STORE_USER_CODE           = 0x17, /* Write-only Byte */
    PMBUS_RESTORE_USER_CODE         = 0x18, /* Write-only Byte */
    PMBUS_CAPABILITY                = 0x19, /* Read-Only byte */
    PMBUS_QUERY                     = 0x1A, /* Write-Only */
    PMBUS_SMBALERT_MASK             = 0x1B, /* Block read, Word write */
    PMBUS_VOUT_MODE                 = 0x20, /* R/W byte */
    PMBUS_VOUT_COMMAND              = 0x21, /* R/W word */
    PMBUS_VOUT_TRIM                 = 0x22, /* R/W word */
    PMBUS_VOUT_CAL_OFFSET           = 0x23, /* R/W word */
    PMBUS_VOUT_MAX                  = 0x24, /* R/W word */
    PMBUS_VOUT_MARGIN_HIGH          = 0x25, /* R/W word */
    PMBUS_VOUT_MARGIN_LOW           = 0x26, /* R/W word */
    PMBUS_VOUT_TRANSITION_RATE      = 0x27, /* R/W word */
    PMBUS_VOUT_DROOP                = 0x28, /* R/W word */
    PMBUS_VOUT_SCALE_LOOP           = 0x29, /* R/W word */
    PMBUS_VOUT_SCALE_MONITOR        = 0x2A, /* R/W word */
    PMBUS_COEFFICIENTS              = 0x30, /* Read-only block 5 bytes */
    PMBUS_POUT_MAX                  = 0x31, /* R/W word */
    PMBUS_MAX_DUTY                  = 0x32, /* R/W word */
    PMBUS_FREQUENCY_SWITCH          = 0x33, /* R/W word */
    PMBUS_VIN_ON                    = 0x35, /* R/W word */
    PMBUS_VIN_OFF                   = 0x36, /* R/W word */
    PMBUS_INTERLEAVE                = 0x37, /* R/W word */
    PMBUS_IOUT_CAL_GAIN             = 0x38, /* R/W word */
    PMBUS_IOUT_CAL_OFFSET           = 0x39, /* R/W word */
    PMBUS_FAN_CONFIG_1_2            = 0x3A, /* R/W byte */
    PMBUS_FAN_COMMAND_1             = 0x3B, /* R/W word */
    PMBUS_FAN_COMMAND_2             = 0x3C, /* R/W word */
    PMBUS_FAN_CONFIG_3_4            = 0x3D, /* R/W byte */
    PMBUS_FAN_COMMAND_3             = 0x3E, /* R/W word */
    PMBUS_FAN_COMMAND_4             = 0x3F, /* R/W word */
    PMBUS_VOUT_OV_FAULT_LIMIT       = 0x40, /* R/W word */
    PMBUS_VOUT_OV_FAULT_RESPONSE    = 0x41, /* R/W byte */
    PMBUS_VOUT_OV_WARN_LIMIT        = 0x42, /* R/W word */
    PMBUS_VOUT_UV_WARN_LIMIT        = 0x43, /* R/W word */
    PMBUS_VOUT_UV_FAULT_LIMIT       = 0x44, /* R/W word */
    PMBUS_VOUT_UV_FAULT_RESPONSE    = 0x45, /* R/W byte */
    PMBUS_IOUT_OC_FAULT_LIMIT       = 0x46, /* R/W word */
    PMBUS_IOUT_OC_FAULT_RESPONSE    = 0x47, /* R/W byte */
    PMBUS_IOUT_OC_LV_FAULT_LIMIT    = 0x48, /* R/W word */
    PMBUS_IOUT_OC_LV_FAULT_RESPONSE = 0x49, /* R/W byte */
    PMBUS_IOUT_OC_WARN_LIMIT        = 0x4A, /* R/W word */
    PMBUS_IOUT_UC_FAULT_LIMIT       = 0x4B, /* R/W word */
    PMBUS_IOUT_UC_FAULT_RESPONSE    = 0x4C, /* R/W byte */
    PMBUS_OT_FAULT_LIMIT            = 0x4F, /* R/W word */
    PMBUS_OT_FAULT_RESPONSE         = 0x50, /* R/W byte */
    PMBUS_OT_WARN_LIMIT             = 0x51, /* R/W word */
    PMBUS_UT_WARN_LIMIT             = 0x52, /* R/W word */
    PMBUS_UT_FAULT_LIMIT            = 0x53, /* R/W word */
    PMBUS_UT_FAULT_RESPONSE         = 0x54, /* R/W byte */
    PMBUS_VIN_OV_FAULT_LIMIT        = 0x55, /* R/W word */
    PMBUS_VIN_OV_FAULT_RESPONSE     = 0x56, /* R/W byte */
    PMBUS_VIN_OV_WARN_LIMIT         = 0x57, /* R/W word */
    PMBUS_VIN_UV_WARN_LIMIT         = 0x58, /* R/W word */
    PMBUS_VIN_UV_FAULT_LIMIT        = 0x59, /* R/W word */
    PMBUS_VIN_UV_FAULT_RESPONSE     = 0x5A, /* R/W byte */
    PMBUS_IIN_OC_FAULT_LIMIT        = 0x5B, /* R/W word */
    PMBUS_IIN_OC_FAULT_RESPONSE     = 0x5C, /* R/W byte */
    PMBUS_IIN_OC_WARN_LIMIT         = 0x5D, /* R/W word */
    PMBUS_POWER_GOOD_ON             = 0x5E, /* R/W word */
    PMBUS_POWER_GOOD_OFF            = 0x5F, /* R/W word */
    PMBUS_TON_DELAY                 = 0x60, /* R/W word */
    PMBUS_TON_RISE                  = 0x61, /* R/W word */
    PMBUS_TON_MAX_FAULT_LIMIT       = 0x62, /* R/W word */
    PMBUS_TON_MAX_FAULT_RESPONSE    = 0x63, /* R/W byte */
    PMBUS_TOFF_DELAY                = 0x64, /* R/W word */
    PMBUS_TOFF_FALL                 = 0x65, /* R/W word */
    PMBUS_TOFF_MAX_WARN_LIMIT       = 0x66, /* R/W word */
    PMBUS_POUT_OP_FAULT_LIMIT       = 0x68, /* R/W word */
    PMBUS_POUT_OP_FAULT_RESPONSE    = 0x69, /* R/W byte */
    PMBUS_POUT_OP_WARN_LIMIT        = 0x6A, /* R/W word */
    PMBUS_PIN_OP_WARN_LIMIT         = 0x6B, /* R/W word */
    PMBUS_STATUS_BYTE               = 0x78, /* R/W byte */
    PMBUS_STATUS_WORD               = 0x79, /* R/W word */
    PMBUS_STATUS_VOUT               = 0x7A, /* R/W byte */
    PMBUS_STATUS_IOUT               = 0x7B, /* R/W byte */
    PMBUS_STATUS_INPUT              = 0x7C, /* R/W byte */
    PMBUS_STATUS_TEMPERATURE        = 0x7D, /* R/W byte */
    PMBUS_STATUS_CML                = 0x7E, /* R/W byte */
    PMBUS_STATUS_OTHER              = 0x7F, /* R/W byte */
    PMBUS_STATUS_MFR_SPECIFIC       = 0x80, /* R/W byte */
    PMBUS_STATUS_FANS_1_2           = 0x81, /* R/W byte */
    PMBUS_STATUS_FANS_3_4           = 0x82, /* R/W byte */
    PMBUS_READ_EIN                  = 0x86, /* Read-Only block 5 bytes */
    PMBUS_READ_EOUT                 = 0x87, /* Read-Only block 5 bytes */
    PMBUS_READ_VIN                  = 0x88, /* Read-Only word */
    PMBUS_READ_IIN                  = 0x89, /* Read-Only word */
    PMBUS_READ_VCAP                 = 0x8A, /* Read-Only word */
    PMBUS_READ_VOUT                 = 0x8B, /* Read-Only word */
    PMBUS_READ_IOUT                 = 0x8C, /* Read-Only word */
    PMBUS_READ_TEMPERATURE_1        = 0x8D, /* Read-Only word */
    PMBUS_READ_TEMPERATURE_2        = 0x8E, /* Read-Only word */
    PMBUS_READ_TEMPERATURE_3        = 0x8F, /* Read-Only word */
    PMBUS_READ_FAN_SPEED_1          = 0x90, /* Read-Only word */
    PMBUS_READ_FAN_SPEED_2          = 0x91, /* Read-Only word */
    PMBUS_READ_FAN_SPEED_3          = 0x92, /* Read-Only word */
    PMBUS_READ_FAN_SPEED_4          = 0x93, /* Read-Only word */
    PMBUS_READ_DUTY_CYCLE           = 0x94, /* Read-Only word */
    PMBUS_READ_FREQUENCY            = 0x95, /* Read-Only word */
    PMBUS_READ_POUT                 = 0x96, /* Read-Only word */
    PMBUS_READ_PIN                  = 0x97, /* Read-Only word */
    PMBUS_REVISION                  = 0x98, /* Read-Only byte */
    PMBUS_MFR_ID                    = 0x99, /* R/W block */
    PMBUS_MFR_MODEL                 = 0x9A, /* R/W block */
    PMBUS_MFR_REVISION              = 0x9B, /* R/W block */
    PMBUS_MFR_LOCATION              = 0x9C, /* R/W block */
    PMBUS_MFR_DATE                  = 0x9D, /* R/W block */
    PMBUS_MFR_SERIAL                = 0x9E, /* R/W block */
    PMBUS_APP_PROFILE_SUPPORT       = 0x9F, /* Read-Only block-read */
    PMBUS_MFR_VIN_MIN               = 0xA0, /* Read-Only word */
    PMBUS_MFR_VIN_MAX               = 0xA1, /* Read-Only word */
    PMBUS_MFR_IIN_MAX               = 0xA2, /* Read-Only word */
    PMBUS_MFR_PIN_MAX               = 0xA3, /* Read-Only word */
    PMBUS_MFR_VOUT_MIN              = 0xA4, /* Read-Only word */
    PMBUS_MFR_VOUT_MAX              = 0xA5, /* Read-Only word */
    PMBUS_MFR_IOUT_MAX              = 0xA6, /* Read-Only word */
    PMBUS_MFR_POUT_MAX              = 0xA7, /* Read-Only word */
    PMBUS_MFR_TAMBIENT_MAX          = 0xA8, /* Read-Only word */
    PMBUS_MFR_TAMBIENT_MIN          = 0xA9, /* Read-Only word */
    PMBUS_MFR_EFFICIENCY_LL         = 0xAA, /* Read-Only block 14 bytes */
    PMBUS_MFR_EFFICIENCY_HL         = 0xAB, /* Read-Only block 14 bytes */
    PMBUS_MFR_PIN_ACCURACY          = 0xAC, /* Read-Only byte */
    PMBUS_IC_DEVICE_ID              = 0xAD, /* Read-Only block-read */
    PMBUS_IC_DEVICE_REV             = 0xAE, /* Read-Only block-read */
};

/* STATUS_WORD */
#define PB_STATUS_VOUT           BIT(15)
#define PB_STATUS_IOUT_POUT      BIT(14)
#define PB_STATUS_INPUT          BIT(13)
#define PB_STATUS_WORD_MFR       BIT(12)
#define PB_STATUS_POWER_GOOD_N   BIT(11)
#define PB_STATUS_FAN            BIT(10)
#define PB_STATUS_OTHER          BIT(9)
#define PB_STATUS_UNKNOWN        BIT(8)
/* STATUS_BYTE */
#define PB_STATUS_BUSY           BIT(7)
#define PB_STATUS_OFF            BIT(6)
#define PB_STATUS_VOUT_OV        BIT(5)
#define PB_STATUS_IOUT_OC        BIT(4)
#define PB_STATUS_VIN_UV         BIT(3)
#define PB_STATUS_TEMPERATURE    BIT(2)
#define PB_STATUS_CML            BIT(1)
#define PB_STATUS_NONE_ABOVE     BIT(0)

/* STATUS_VOUT */
#define PB_STATUS_VOUT_OV_FAULT         BIT(7) /* Output Overvoltage Fault */
#define PB_STATUS_VOUT_OV_WARN          BIT(6) /* Output Overvoltage Warning */
#define PB_STATUS_VOUT_UV_WARN          BIT(5) /* Output Undervoltage Warning */
#define PB_STATUS_VOUT_UV_FAULT         BIT(4) /* Output Undervoltage Fault */
#define PB_STATUS_VOUT_MAX              BIT(3)
#define PB_STATUS_VOUT_TON_MAX_FAULT    BIT(2)
#define PB_STATUS_VOUT_TOFF_MAX_WARN    BIT(1)

/* STATUS_IOUT */
#define PB_STATUS_IOUT_OC_FAULT    BIT(7) /* Output Overcurrent Fault */
#define PB_STATUS_IOUT_OC_LV_FAULT BIT(6) /* Output OC And Low Voltage Fault */
#define PB_STATUS_IOUT_OC_WARN     BIT(5) /* Output Overcurrent Warning */
#define PB_STATUS_IOUT_UC_FAULT    BIT(4) /* Output Undercurrent Fault */
#define PB_STATUS_CURR_SHARE       BIT(3) /* Current Share Fault */
#define PB_STATUS_PWR_LIM_MODE     BIT(2) /* In Power Limiting Mode */
#define PB_STATUS_POUT_OP_FAULT    BIT(1) /* Output Overpower Fault */
#define PB_STATUS_POUT_OP_WARN     BIT(0) /* Output Overpower Warning */

/* STATUS_INPUT */
#define PB_STATUS_INPUT_VIN_OV_FAULT    BIT(7) /* Input Overvoltage Fault */
#define PB_STATUS_INPUT_VIN_OV_WARN     BIT(6) /* Input Overvoltage Warning */
#define PB_STATUS_INPUT_VIN_UV_WARN     BIT(5) /* Input Undervoltage Warning */
#define PB_STATUS_INPUT_VIN_UV_FAULT    BIT(4) /* Input Undervoltage Fault */
#define PB_STATUS_INPUT_IIN_OC_FAULT    BIT(2) /* Input Overcurrent Fault */
#define PB_STATUS_INPUT_IIN_OC_WARN     BIT(1) /* Input Overcurrent Warning */
#define PB_STATUS_INPUT_PIN_OP_WARN     BIT(0) /* Input Overpower Warning */

/* STATUS_TEMPERATURE */
#define PB_STATUS_OT_FAULT              BIT(7) /* Overtemperature Fault */
#define PB_STATUS_OT_WARN               BIT(6) /* Overtemperature Warning */
#define PB_STATUS_UT_WARN               BIT(5) /* Undertemperature Warning */
#define PB_STATUS_UT_FAULT              BIT(4) /* Undertemperature Fault */

/* STATUS_CML */
#define PB_CML_FAULT_INVALID_CMD     BIT(7) /* Invalid/Unsupported Command */
#define PB_CML_FAULT_INVALID_DATA    BIT(6) /* Invalid/Unsupported Data  */
#define PB_CML_FAULT_PEC             BIT(5) /* Packet Error Check Failed */
#define PB_CML_FAULT_MEMORY          BIT(4) /* Memory Fault Detected */
#define PB_CML_FAULT_PROCESSOR       BIT(3) /* Processor Fault Detected */
#define PB_CML_FAULT_OTHER_COMM      BIT(1) /* Other communication fault */
#define PB_CML_FAULT_OTHER_MEM_LOGIC BIT(0) /* Other Memory Or Logic Fault */

/* OPERATION*/
#define PB_OP_ON                BIT(7) /* PSU is switched on */
#define PB_OP_MARGIN_HIGH       BIT(5) /* PSU vout is set to margin high */
#define PB_OP_MARGIN_LOW        BIT(4) /* PSU vout is set to margin low */

/* PAGES */
#define PB_MAX_PAGES            0x1F
#define PB_ALL_PAGES            0xFF

#define BIT_ULL(x)  (1ULL<<(x))
/* flags */
#define PB_HAS_COEFFICIENTS        BIT_ULL(9)
#define PB_HAS_VIN                 BIT_ULL(10)
#define PB_HAS_VOUT                BIT_ULL(11)
#define PB_HAS_VOUT_MARGIN         BIT_ULL(12)
#define PB_HAS_VIN_RATING          BIT_ULL(13)
#define PB_HAS_VOUT_RATING         BIT_ULL(14)
#define PB_HAS_VOUT_MODE           BIT_ULL(15)
#define PB_HAS_IOUT                BIT_ULL(21)
#define PB_HAS_IIN                 BIT_ULL(22)
#define PB_HAS_IOUT_RATING         BIT_ULL(23)
#define PB_HAS_IIN_RATING          BIT_ULL(24)
#define PB_HAS_IOUT_GAIN           BIT_ULL(25)
#define PB_HAS_POUT                BIT_ULL(30)
#define PB_HAS_PIN                 BIT_ULL(31)
#define PB_HAS_EIN                 BIT_ULL(32)
#define PB_HAS_EOUT                BIT_ULL(33)
#define PB_HAS_POUT_RATING         BIT_ULL(34)
#define PB_HAS_PIN_RATING          BIT_ULL(35)
#define PB_HAS_TEMPERATURE         BIT_ULL(40)
#define PB_HAS_TEMP2               BIT_ULL(41)
#define PB_HAS_TEMP3               BIT_ULL(42)
#define PB_HAS_TEMP_RATING         BIT_ULL(43)
#define PB_HAS_MFR_INFO            BIT_ULL(50)


extern int pmset_page(int, unsigned char);
extern int pmget_mfr_id(int);
extern int pmbus_write(int, int, int, unsigned char *);
extern int pmbus_read(int, int, int, unsigned char *);
extern unsigned short toshort(unsigned char *);
extern int pmset_op(int, int, int);
extern double pmread_vout(int, int);

#endif /* __PMBUS_H */
