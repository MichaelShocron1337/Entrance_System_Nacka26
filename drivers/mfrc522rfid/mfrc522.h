#ifndef MFRC522_H
#define MFRC522_H

#include <stdint.h>
#include <stddef.h>

/* =========================
   Status codes
   ========================= */
typedef enum
{
    MFRC522_OK = 0,
    MFRC522_ERROR,
    MFRC522_TIMEOUT,
    MFRC522_NO_TAG,
    MFRC522_COLLISION
} mfrc522_status_t;

/* =========================
   PICC commands
   ========================= */
#define PICC_CMD_REQA            0x26
#define PICC_CMD_WUPA            0x52
#define PICC_CMD_CT              0x88
#define PICC_CMD_SEL_CL1         0x93
#define PICC_CMD_SEL_CL2         0x95
#define PICC_CMD_HLTA            0x50

/* =========================
   PCD commands
   ========================= */
#define PCD_IDLE                 0x00
#define PCD_MEM                  0x01
#define PCD_GENERATE_RANDOM_ID   0x02
#define PCD_CALC_CRC             0x03
#define PCD_TRANSMIT             0x04
#define PCD_NO_CMD_CHANGE        0x07
#define PCD_RECEIVE              0x08
#define PCD_TRANSCEIVE           0x0C
#define PCD_MFAUTHENT            0x0E
#define PCD_SOFTRESET            0x0F

/* =========================
   MFRC522 registers
   ========================= */
#define CommandReg               0x01
#define ComIEnReg                0x02
#define DivIEnReg                0x03
#define ComIrqReg                0x04
#define DivIrqReg                0x05
#define ErrorReg                 0x06
#define Status1Reg               0x07
#define Status2Reg               0x08
#define FIFODataReg              0x09
#define FIFOLevelReg             0x0A
#define WaterLevelReg            0x0B
#define ControlReg               0x0C
#define BitFramingReg            0x0D
#define CollReg                  0x0E

#define ModeReg                  0x11
#define TxModeReg                0x12
#define RxModeReg                0x13
#define TxControlReg             0x14
#define TxASKReg                 0x15
#define TxSelReg                 0x16
#define RxSelReg                 0x17
#define RxThresholdReg           0x18
#define DemodReg                 0x19
#define MfTxReg                  0x1C
#define MfRxReg                  0x1D
#define SerialSpeedReg           0x1F

#define CRCResultRegH            0x21
#define CRCResultRegL            0x22
#define ModWidthReg              0x24
#define RFCfgReg                 0x26
#define GsNReg                   0x27
#define CWGsPReg                 0x28
#define ModGsPReg                0x29
#define TModeReg                 0x2A
#define TPrescalerReg            0x2B
#define TReloadRegH              0x2C
#define TReloadRegL              0x2D
#define TCounterValueRegH        0x2E
#define TCounterValueRegL        0x2F

#define TestSel1Reg              0x31
#define TestSel2Reg              0x32
#define TestPinEnReg             0x33
#define TestPinValueReg          0x34
#define TestBusReg               0x35
#define AutoTestReg              0x36
#define VersionReg               0x37
#define AnalogTestReg            0x38
#define TestDAC1Reg              0x39
#define TestDAC2Reg              0x3A
#define TestADCReg               0x3B

/* =========================
   Data structures
   ========================= */
typedef struct
{
    uint8_t uid[10];
    uint8_t size;
    uint8_t sak;
} mfrc522_uid_t;

/* =========================
   Public API
   ========================= */
void mfrc522_init(void);
void mfrc522_reset(void);
uint8_t mfrc522_get_version(void);

void mfrc522_write_reg(uint8_t reg, uint8_t value);
uint8_t mfrc522_read_reg(uint8_t reg);
void mfrc522_set_bitmask(uint8_t reg, uint8_t mask);
void mfrc522_clear_bitmask(uint8_t reg, uint8_t mask);

void mfrc522_antenna_on(void);
void mfrc522_antenna_off(void);

mfrc522_status_t mfrc522_request_a(uint8_t *atqa, uint8_t *atqa_len);
mfrc522_status_t mfrc522_anticoll_select(mfrc522_uid_t *uid);
mfrc522_status_t mfrc522_halt(void);

#endif
