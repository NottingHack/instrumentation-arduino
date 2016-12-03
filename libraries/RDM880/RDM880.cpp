/*
 *  RDM880.cpp
 *  Nottingham Hackspace Instrumentaion
 *
 *  Created by Matt Lloyd on 2016.
 *  Copyright Matt Lloyd. All rights reserved.
 *
 */

#include <Arduino.h>
#include "RDM880.h"
// #include <string.h>

RDM880::RDM880(Stream *stream)
{
    _myStream = stream;
    _stationId = 0x00;
}

RDM880::RDM880(Stream *stream, uint8_t stationId)
{
    _myStream = stream;
    _stationId = stationId;
}

// uint8_t RDM880::mfRead(MIFARE_Packet *packet, uint8_t *readData) 
// {
//     sendCommand(MI_CMD_Read, packet, sizeof(MIFARE_Packet));

//     if (!readResponse()) {
//         return -1;
//     }

//     if (_responseBuf[STATUS_BYTE] == STATUS_CMD_OK) {
//         memcpy(readData, _responseBuf[DATA_START_BYTE+4], _responseBuf[LEN_BYTE]-4);
//     }
//     return _responseBuf[STATUS_BYTE];
// }

// uint8_t RDM880::mfWrite(MIFARE_Packet *packet, uint8_t *dataToWrite)
// {
//     sendCommand(MI_CMD_WRITE, packet, sizeof(MIFARE_Packet), dataToWrite, packet.blockCount*16);

//     if (!readResponse()) {
//         return -1;
//     }

//     return _responseBuf[STATUS_BYTE];
// }

uint8_t RDM880::mfGetSerial(uint8_t mode, uint8_t halt)
{
    // uint8_t *dataBuffer[1];
    // dataBuffer[0] = 0x26;
    // sendCommand(0x26, dataBuffer, 1);
    uint8_t data[2];
    data[0] = mode;
    data[1] = halt;
    sendCommand(MI_CMD_GetSNR, data, 2);

    // get the serial number
    uint8_t ret = readResponse();
    if (ret != 6) {
        // carp back packet
        return ret;
    }

    // check the status flag
    if (_responseBuf[STATUS_BYTE] != STATUS_CMD_OK) {
        return 7;
    }
    // get data based on length
    if (_responseBuf[LEN_BYTE] < 6){
        // note enough data
        return 8;
    }

    uint8_t offset = (_responseBuf[LEN_BYTE] == 6) ? 2 : 1;
    uid.size = _responseBuf[LEN_BYTE] - offset;
    memcpy(uid.uidByte, &_responseBuf[DATA_START_BYTE + offset - 1], uid.size);

    return 9;
}

void RDM880::sendCommand(uint8_t cmd, uint8_t* data, uint8_t dataLen)
{
    // [STX][STATION ID][DATA LEN][CMD][DATAn][BCC][ETX]
    uint8_t checksum;
    _myStream->write(STX);
    _myStream->write(_stationId);

    dataLen++;  //add in the command character
    _myStream->write(dataLen);
    _myStream->write(cmd);

    checksum = (_stationId ^ dataLen ^ cmd);

    for (uint8_t i=0; i<dataLen-1; i++) {
      _myStream->write(data[i]);
      checksum = checksum ^ data[i];
    } // end for

    _myStream->write(checksum);
    _myStream->write(ETX);
}

void RDM880::sendCommand(uint8_t cmd, uint8_t* data, uint8_t dataLen, uint8_t* buffer, uint8_t bufferLen)
{
    // [STX][STATION ID][DATA LEN][CMD][DATAn][BCC][ETX]
    flushOutput();
    uint8_t checksum;
    _myStream->write(STX);
    _myStream->write(_stationId);

    dataLen++;  //add in the command character
    _myStream->write(dataLen + bufferLen);
    _myStream->write(cmd);

    checksum = (_stationId ^ dataLen ^ cmd);

    for (uint8_t i=0; i<dataLen-1; i++) {
      _myStream->write(data[i]);
      checksum = checksum ^ data[i];
    } // end for

    for (uint8_t i=0; i<bufferLen; i++) {
      _myStream->write(buffer[i]);
      checksum = checksum ^ buffer[i];
    } // end for

    _myStream->write(checksum);
    _myStream->write(ETX);

}

uint8_t RDM880::readResponse()
{
    byte incomingByte;
    byte responsePtr =0;
    unsigned long time = millis();
    while (millis() - time < 20) {
        // read the incoming byte:
        if (_myStream->available() > 0) {
            incomingByte = (char)_myStream->read();
            _responseBuf[responsePtr++]=incomingByte;
            time = millis();
        }
    } // end while
    _responseBuf[responsePtr]=0;
    return 6;
    // // [STX][STATION ID][DATA LEN][STATUS][DATAn][BCC][ETX]
    // uint8_t checksum;
    // uint8_t responsePtr = 0;
    
    // uint8_t c;
    // while( _myStream->available() < 1) {
    //     delay(1);
    // }
    // uint32_t time = millis();
    
    // do{ 
    //     c = _myStream->read();
    //     // Serial.println(c, HEX);
    //     if ((millis() - time) > 1000) {
    //         // time out, failed to find start of packet
    //         flushOutput();
    //         return 1;
    //     }
    //     delay(20); // small wait
    // } while (c != STX);

    // _responseBuf[responsePtr++] = STX;
    // _responseBuf[responsePtr++] = _myStream->read(); // grab the stationId
    // if (_responseBuf[1] != _stationId) {
    //     // stationId did not match
    //     flushOutput();
    //     return 2;
    // }

    // _responseBuf[responsePtr++] = _myStream->read(); // grab the dataLen

    // checksum = (_responseBuf[STATION_BYTE] ^ _responseBuf[LEN_BYTE]); 

    // for (uint8_t i; i<_responseBuf[2]; i++) {
    //     if (_myStream->available() > 0) {
    //         _responseBuf[responsePtr] = _myStream->read(); // grab the data
    //         checksum = checksum ^ _responseBuf[responsePtr++]; // add it into our checksum calc
    //     } else {
    //         // hmm we should still have data to get, lets bail
    //         flushOutput();
    //         return 3;
    //     }
    // }

    // _responseBuf[responsePtr++] = _myStream->read(); // grab the crc
    // // check the CRC
    // if (checksum != _responseBuf[responsePtr-1]) {
    //     flushOutput();
    //     return 4;
    // }
    // _responseBuf[responsePtr] = _myStream->read(); // this should be the ETX

    // if (_responseBuf[responsePtr] != ETX) {
    //     // ext didnt match
    //     flushOutput();
    //     return 5;
    // }
    // flushOutput();
    // return 6;
} 

void RDM880::flushOutput()
{
    uint8_t devnull;
    while( _myStream->available() != 0) {
        devnull = _myStream->read();
    }
}