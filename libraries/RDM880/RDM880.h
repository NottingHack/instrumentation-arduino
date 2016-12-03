/*
 *  RDM880.h
 *  Nottingham Hackspace Instrumentaion
 *
 *  Created by Matt Lloyd on 2016.
 *  Copyright Matt Lloyd. All rights reserved.
 *
 */

#include <Arduino.h>

#ifndef RDM880_h
#define RDM880_h

class RDM880 
{
private:
    enum {
        RDM880_MAX_RESPONSE_LEN     = 60,
        MF_KEY_SIZE                 = 6
    };

    Stream *_myStream;
    uint8_t _stationId;

    uint8_t _responseBuf[RDM880_MAX_RESPONSE_LEN];
    void sendCommand(uint8_t cmd, uint8_t* data, uint8_t dataLen);
    void sendCommand(uint8_t cmd, uint8_t* data, uint8_t dataLen, uint8_t* buffer, uint8_t bufferLen);
    uint8_t readResponse(); // returns false on timeout invalid packet
    void flushOutput();

public:
    // A struct used for passing the UID.
    typedef struct {
        uint8_t        size;           // Number of bytes in the UID. 4, 7 or 10.
        uint8_t        uidByte[10];
    } Uid;

    typedef struct {
        uint8_t mode;
        uint8_t blockCount;
        uint8_t startAddress;
        uint8_t keyByte[MF_KEY_SIZE];
    } MIFARE_Packet;

    typedef struct {
        uint8_t mode;
        uint8_t sector;
        uint8_t keyByte[MF_KEY_SIZE];
        uint8_t value[2];
    } MIFARE_Value;

    enum PACKET {
        STX                         = 0xAA,
        ETX                         = 0xBB,
        STATION_BYTE                = 1,
        LEN_BYTE                    = 2,
        STATUS_BYTE                  = 3,
        DATA_START_BYTE             = 4
    };

    enum RMD_BAUD {
        BAUD_9600                   = 0x00,
        BUAD_19200                  = 0x01,
        BAUD_38400                  = 0x02,
        BAUD_57600                  = 0x05,
        BAUD_115200                 = 0x04
    };

    enum ISO14443A {
        ISO14443A_CMD_Request       = 0x03, // takes data[0] REQUEST_MODE. response date[0-1] two byte ATQ
        ISO14443A_CMD_Anticollision = 0x04, // response data[0] ISO14443A_MULTI_CARD_FLAG, data[1-4?11] uid
        ISO14443A_CMD_Select        = 0x05, // takes uid. response uid
        ISO14443A_CMD_Halt          = 0x06, // response data[0] 0x80
        ISO14443A_CMD_Transfer      = 0x82  // takes data[0] CRC_FLAG, data[1] dataLen, data[2-n] data to send, repose data[0-n] from card
    };
    
    enum ISO14443A_MULTI_CARD_FLAG {
        ISO14443A_MULTI_FLAG_ONE              = 0x26,
        ISO14443A_MULTI_FLAG_MANY             = 0x52
    };

    // enum ISO14443B {
    //     ISO14443B_CMD_Request       = 0x09,
    //     ISO14443B_CMD_Anticollision = 0x0A,
    //     ISO14443B_CMD_Attrib        = 0x0B,
    //     ISO14443B_CMD_Rst           = 0x0C,
    //     ISO14443B_CMD_Transfer      = 0x0D
    // };

    enum MIFARE {
        MI_CMD_Read             = 0x20, // takes data[0-8] MIFARE_PACKET. response data[0-3] serial, data[4-n] read
        MI_CMD_Write            = 0x21, // takes data[0-8] MIFARE_PACKET, data[9-24] 16byte to write
        MI_CMD_InitVal          = 0x22, // takes data[0-8] MIFARE_PACKET, ??
        MI_CMD_Decrement        = 0x23, // takes data[0-8] MIFARE_PACKET, ??
        MI_CMD_Increment        = 0x24, // takes data[0-8] MIFARE_PACKET, ??
        MI_CMD_GetSNR           = 0x25  // takes data[0] REQUEST_MODE, data[1] halt, response data[0] MI_MULTI_CARD_FLAG, data[1-n] uid
    };

    enum MIFARE_MODE_CONTROL {
        MI_REQ_IDLE                 = (0x00 << 0),
        MI_REQ_ALL                  = (0x01 << 0),
        MI_USE_KEYA                 = (0x00 << 1),
        MI_USE_KEYB                 = (0x01 << 1)
    };

    enum REQUEST_MODE {
        REQ_MODE_IDLE               = 0x26,
        REQ_MODE_ALL                = 0x52
    };

    enum MI_MULTI_CARD_FLAG {
        MI_MULTI_FLAG_ONE              = 0x00,
        MI_MULTI_FLAG_MANY             = 0x01
    };

    enum SYSTEM {
        SYSTEM_CMD_SetAddress       = 0x80, // takes 1 byte
        SYSTEM_CMD_SetBaudrate      = 0x81, // takes 1 bye form BAUD
        SYSTEM_CMD_SetSerialNumber  = 0x82, // takes 8 bytes
        SYSTEM_CMD_GetSerialNumber  = 0x83, // response data[0] address, data[1-9] serial
        SYSTEM_CMD_Write_UserInfo   = 0x84, // takes data[0] area number {0-3}, data[1] len to write, data[2-n] data to write
        SYSTEM_CMD_Read_UserInfo    = 0x85, // takes data[0] area number {0-3}, data[1] len to read. response data[0-n] returned data
        SYSTEM_CMD_Get_VersionNum   = 0x86, // returns 6 or more
        SYSTEM_CMD_Control_Led1     = 0x87, // takes data[0] nx20ms time, data[1] no cycles
        SYSTEM_CMD_Control_Led2     = 0x88, // takes data[0] nx20ms time, data[1] no cycles
        SYSTEM_CMD_Control_Buzzer   = 0x89  // takes data[0] nx20ms time, data[1] no cycles
    };

    // enum ISO15693 {
    //     ISO15693_CMD_Inventory                      = 0x10,
    //     ISO15693_CMD_Read                           = 0x11,
    //     ISO15693_CMD_Write                          = 0x12,
    //     ISO15693_CMD_Lockblock                      = 0x13,
    //     ISO15693_CMD_StayQuiet                      = 0x14,
    //     ISO15693_CMD_Select                         = 0x15,
    //     ISO15693_CMD_Resetready                     = 0x16,
    //     ISO15693_CMD_Write_AFI                      = 0x17,
    //     ISO15693_CMD_Lock_AFI                       = 0x18,
    //     ISO15693_CMD_Write_DSFID                    = 0x19,
    //     ISO15693_CMD_Lock_DSFID                     = 0x1A,
    //     ISO15693_CMD_Get_Information                = 0x1B,
    //     ISO15693_CMD_Get_Multiple_Block_Security    = 0x1C,
    //     ISO15693_CMD_Transfer                       = 0x1D
    // };

    // enum ISO15693_FLAGS {
    //     ISO15693_FLAG_SUB_CARRIER                   = (1 << 0),
    //     ISO15693_FLAG_DATA_RATE                     = (1 << 1),
    //     ISO15693_FLAG_INVONTERY                     = (1 << 2),
    //     ISO15693_FLAG_PROTOCOL_EXT                  = (1 << 3),
    //     ISO15693_FLAG_SELECT                        = (1 << 4),
    //     ISO15693_FLAG_ADDRESS                       = (1 << 5),
    //     ISO15693_FLAG_OPTION                        = (1 << 6),
    //     ISO15693_FLAG_RFU                           = (1 << 7)
    // };

    enum Status {
        STATUS_CMD_OK                           = 0x00,  // Command OK
        STATUS_CMD_FAILED                       = 0x01,  // Command failed
        STATUS_SET_OK                           = 0x80,  // Set OK
        STATUS_SET_FAILED                       = 0x81,  // Set failed
        STATUS_TIMEOUT                          = 0x82,  // Reader reply timeout
        STATUS_NO_CARD                          = 0x83,  // Card does not exist
        STATUS_DATA_ERROR                       = 0x84,  // The data response from the card is error
        STATUS_INVALID_PARA                     = 0x85,  // Invalid command parameter
        STATUS_UNKOWN_ERROR                     = 0x87,  // Unknown internal error
        STATUS_UNKOWN_CMD                       = 0x8F,  // Reader received unknown command
        STATUS_ISO14443_INIT_FAIL               = 0x8A,  // ISO14443: Error in InitVal process
        STATUS_ISO14443_BAD_SNR                 = 0x8B,  // ISO14443: Wrong SNR during anticollision loop
        STATUS_ISO14443_AUTH_FAIL               = 0x8C,  // ISO14443: Authentication failure
        STATUS_ISO15693_NOT_SUPPORTED           = 0x90,  // ISO15693: The card does not support this command
        STATUS_ISO15693_BAD_FORMAT              = 0x91,  // ISO15693: Invalid command format
        STATUS_ISO15693_OPTION_NOT_SUPPORTED    = 0x92,  // ISO15693: Do not support option mode
        STATUS_ISO15693_BLOCK_DOES_NOT_EXIST    = 0x93,  // ISO15693: Block does not exist
        STATUS_ISO15693_OBJECT_LOCKED           = 0x94,  // ISO15693: The object has been locked
        STATUS_ISO15693_LOCK_FAILED             = 0x95,  // ISO15693: Lock operation failed
        STATUS_ISO15693_OPERAITION_FAILED       = 0x96   // ISO15693: Operation failed
    };

    Uid uid;

    RDM880(Stream *serial); 
    RDM880(Stream *serial, uint8_t stationId); 

    // isobRequest();
    // isobAnticollision();
    // isobAttrib();
    // isobRst();
    // isobTransfer();


    uint8_t mfRead(MIFARE_Packet *packet, uint8_t *readData);
    uint8_t mfWrite(MIFARE_Packet *packet, uint8_t *dataToWrite);
    // mfInitVal(MIFARE_Packet *packet, uint8_t *value);
    // mfDecrement(MIFARE_Packet *packet, uint8_t *value);
    // mfIncrement(MIFARE_Packet *packet, uint8_t *value);
    uint8_t mfGetSerial(uint8_t mode = REQ_MODE_IDLE, uint8_t halt = 0);


};

#endif