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
    PacketParser(void (*onSuccessCallback)(unsigned int, unsigned int) );

    void parseByte(char receivedByte);

private:

    enum class Status
    {
        None,
        PaketStart,
        Channel,
        ChannelDone,
        Separator,
        Count,
        CountDone
    };

    Status m_packetStatus;
    void (*m_onSuccessCallback)(unsigned int, unsigned int);
    unsigned int m_channel;
    unsigned int m_count;
};


PacketParser::PacketParser(void (*onSuccessCallback)(unsigned int, unsigned int))
    : m_packetStatus(Status::None)
    , m_onSuccessCallback(onSuccessCallback)
    , m_channel(0)
    , m_count(0)
{
}


void PacketParser::parseByte(char receivedByte)
{
    // {"12", "1337"}
    switch(m_packetStatus)
    {
        case Status::None:
            if (receivedByte == '{')
                m_packetStatus = Status::PaketStart;
            break;
        case Status::PaketStart:
            if (receivedByte == '\"')
                m_packetStatus = Status::Channel;
            break;
        case Status::Channel:
            if (receivedByte == '\"')
                m_packetStatus = Status::ChannelDone;
            else
            {
                m_channel *= 10; // shift one decimal left
                m_channel = (receivedByte - 48) + m_channel; // convert from ASCII and add
            }
            break;
        case Status::ChannelDone:
            if (receivedByte == ',')
                m_packetStatus = Status::Separator;
            break;
        case Status::Separator:
            if (receivedByte == '\"')
                m_packetStatus = Status::Count;
            break;
        case Status::Count:
            if (receivedByte == '\"')
                m_packetStatus = Status::CountDone;
            else
            {
                m_count *= 10; // shift one decimal left
                m_count = (receivedByte - 48) + m_count; // convert from ASCII and add
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
