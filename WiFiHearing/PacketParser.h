////////////////////////////////////////////////////////////
/// WiFi-Hearing
/// A sense for the wifi data around you.
/// Copyright (C) 2018  Maximilian Wagenbach (aka Foaly)
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU General Public License as published by
/// the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.

/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.

/// You should have received a copy of the GNU General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
////////////////////////////////////////////////////////////

#pragma once


class PacketParser {
public:
    PacketParser(void (*onSuccessCallback)(uint8_t, uint16_t) );

    void parseByte(uint8_t receivedByte);

private:

    enum class Status
    {
        None,
        Channel,
        ChannelDone,
        Count,
        CountDone
    };

    Status m_packetStatus;
    void (*m_onSuccessCallback)(uint8_t, uint16_t);
    uint8_t m_channel;
    uint16_t m_count;
    uint8_t m_byteCounter;
};


PacketParser::PacketParser(void (*onSuccessCallback)(uint8_t, uint16_t))
    : m_packetStatus(Status::None)
    , m_onSuccessCallback(onSuccessCallback)
    , m_channel(0)
    , m_count(0)
    , m_byteCounter(0)
{
}


/**
 *  Only packets with the following format will be parsed correctly:
 *  Packets start with a byte containing the character '{'
 *  Followed by a byte containing the channel number in binary. Channel counts starts at 1.
 *  A delimiter byte containing the character ',' follows.
 *  The next two bytes are a 16-bit integer containing the packet count. It is sent in little-endian order (LSB first).
 *  The last byte in a packet is a '}' character.
 */
void PacketParser::parseByte(uint8_t receivedByte)
{
    /*Serial.print("Status: ");
    Serial.println((int)m_packetStatus);
    Serial.print((char)receivedByte);
    Serial.print(" in bin: ");
    Serial.println(receivedByte, BIN);*/
  
    switch(m_packetStatus)
    {
        case Status::None:
            if (receivedByte == '{')
                m_packetStatus = Status::Channel;
            break;
        case Status::Channel:
                m_channel = receivedByte;
                m_packetStatus = Status::ChannelDone;
            break;
        case Status::ChannelDone:
            if (receivedByte == ',' || receivedByte == ' ')
                m_packetStatus = Status::Count;
            else
                m_packetStatus = Status::None;
            break;
        case Status::Count:
            // we receive the the LSB first and Teensy 3.6 is little endian
            // so we need to fill in from the left
            m_count = m_count >> 8; // shift right by 8 bit
            m_count |= (receivedByte << 8); // add received byte to the left
            m_byteCounter++;

            if (m_byteCounter > 1) {
                m_packetStatus = Status::CountDone;
                m_byteCounter = 0;
            }
            break;
        case Status::CountDone:
            if (receivedByte == '}') {
                Serial.print("c: ");
                Serial.print(m_channel);
                Serial.print(" = ");
                Serial.print(m_count);
                Serial.print("\n");

                m_onSuccessCallback(m_channel, m_count);
                
                m_channel = 0;
                m_count = 0;
                m_packetStatus = Status::None;
            }
            break;
    }
}
