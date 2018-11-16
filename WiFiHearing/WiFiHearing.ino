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

enum class Status
{
    None,
    PaketStart,
    Channel,
    ChannelDone,
    Count,
    Separator,
    CountDone
};
Status packetStatus = Status::None;
unsigned int channel = 0;
unsigned int count = 0;

void setup() {
    Serial4.begin(115200);
    Serial.begin(115200);

}

void loop() {
    if (Serial4.available() > 0)
    {
        char incomingByte = Serial4.read();

        // {"12", "300"}
        switch(packetStatus)
        {
            case Status::None:
                if (incomingByte == '{')
                    packetStatus = Status::PaketStart;
                break;
            case Status::PaketStart:
                if (incomingByte == '\"')
                    packetStatus = Status::Channel;
                break;
            case Status::Channel:
                if (incomingByte == '\"')
                    packetStatus = Status::ChannelDone;
                else
                    channel = (channel * 10) + atoi(&incomingByte);
                break;
            case Status::ChannelDone:
                if (incomingByte == ',')
                    packetStatus = Status::Separator;
                break;
            case Status::Separator:
                if (incomingByte == '\"')
                    packetStatus = Status::Count;
                break;
            case Status::Count:
                if (incomingByte == '\"')
                    packetStatus = Status::CountDone;
                else
                    count = (count * 10) + atoi(&incomingByte);
                break;
            case Status::CountDone:
                if (incomingByte == '}') {
                    Serial.print("c: ");
                    Serial.print(channel);
                    Serial.print(" = ");
                    Serial.print(count);
                    Serial.print("\n");
                    channel = 0;
                    count = 0;
                    packetStatus = Status::None;
                }
                break;
        }
        Serial.println(incomingByte);
    }
}
