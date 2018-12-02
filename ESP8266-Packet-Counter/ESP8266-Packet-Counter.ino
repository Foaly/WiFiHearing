////////////////////////////////////////////////////////////
/// ESP8266-Packet-Counter 
/// Counting packets on all 2.4 GHz WiFI channels
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

extern "C" {
  #include <user_interface.h>
}
#include <elapsedMillis.h>

#define DATA_LENGTH           112

#define TYPE_MANAGEMENT       0x00
#define TYPE_CONTROL          0x01
#define TYPE_DATA             0x02
#define SUBTYPE_PROBE_REQUEST 0x04

#define DISABLE 0
#define ENABLE  1

#define CHANNEL_HOP_INTERVAL_MS   1000
#define MAX_CHANNELS              14
// On a NodeMCU board the built-in led is on GPIO pin 2
#define LED_BUILTIN 2


struct RxControl {
 signed rssi:8; // signal intensity of packet
 unsigned rate:4;
 unsigned is_group:1;
 unsigned:1;
 unsigned sig_mode:2; // 0:is 11n packet; 1:is not 11n packet;
 unsigned legacy_length:12; // if not 11n packet, shows length of packet.
 unsigned damatch0:1;
 unsigned damatch1:1;
 unsigned bssidmatch0:1;
 unsigned bssidmatch1:1;
 unsigned MCS:7; // if is 11n packet, shows the modulation and code used (range from 0 to 76)
 unsigned CWB:1; // if is 11n packet, shows if is HT40 packet or not
 unsigned HT_length:16;// if is 11n packet, shows length of packet.
 unsigned Smoothing:1;
 unsigned Not_Sounding:1;
 unsigned:1;
 unsigned Aggregation:1;
 unsigned STBC:2;
 unsigned FEC_CODING:1; // if is 11n packet, shows if is LDPC packet or not.
 unsigned SGI:1;
 unsigned rxend_state:8;
 unsigned ampdu_cnt:8;
 unsigned channel:4; //which channel this packet in.
 unsigned:12;
};

struct SnifferPacket{
    struct RxControl rx_ctrl;
    uint8_t data[DATA_LENGTH];
    uint16_t cnt;
    uint16_t len;
};

static os_timer_t channelHop_timer;

uint16_t packetCount[MAX_CHANNELS + 1] = {0};
uint8_t currentChannel = 1;
elapsedMillis LEDtimer;
bool isLEDOn = false;

/**
 * Callback for promiscuous mode
 */
static void ICACHE_FLASH_ATTR sniffer_callback(uint8_t *buffer, uint16_t length) {
  struct SnifferPacket *snifferPacket = (struct SnifferPacket*) buffer;
  packetCount[currentChannel]++;
}


/**
 * Callback for channel hoping
 */
void channelHop()
{
  // start packet
  Serial.print("{");
  // send channel number as byte
  Serial.write(currentChannel);

  // send delimiter
  Serial.print(",");

  // send packet count
  // the NodeMCU is a little endian system so we send the LSB first
  uint8_t* p = reinterpret_cast<uint8_t *>( &packetCount[currentChannel] );
  Serial.write(p[0]);
  Serial.write(p[1]);

  // close packet
  Serial.print("}");
  //Serial.print("\n");

  // hop to the next higher channels
  currentChannel = wifi_get_channel() + 1;
  if (currentChannel >= MAX_CHANNELS)
    currentChannel = 1;
  wifi_set_channel(currentChannel);

  // Turn LED on
  isLEDOn = true;
  digitalWrite(LED_BUILTIN, LOW);
  LEDtimer = 0;

  packetCount[currentChannel] = 0;
}


void setup() {
  // set the WiFi chip to "promiscuous" mode aka monitor mode
  Serial.begin(115200);
  delay(10);
  wifi_set_opmode(STATION_MODE);
  wifi_set_channel(currentChannel);
  wifi_promiscuous_enable(DISABLE);
  delay(10);
  wifi_set_promiscuous_rx_cb(sniffer_callback);
  delay(10);
  wifi_promiscuous_enable(ENABLE);

  // setup the channel hoping callback timer
  os_timer_disarm(&channelHop_timer);
  os_timer_setfn(&channelHop_timer, (os_timer_func_t *) channelHop, NULL);
  os_timer_arm(&channelHop_timer, CHANNEL_HOP_INTERVAL_MS, 1);

  // Initialize the LED_BUILTIN pin as an output
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
    // turn the LED off after some time
    if (isLEDOn && LEDtimer > 10)
    {
        digitalWrite(LED_BUILTIN, HIGH);
        isLEDOn = false;
    }
    delay(10);
}
