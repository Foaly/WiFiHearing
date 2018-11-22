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

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#define CHANNELS 12

struct SoundGenerator
{
    AudioSynthWaveform waveform;
    AudioEffectEnvelope envelope;
    elapsedMillis elapsedMs;
};

SoundGenerator soundGenerators[CHANNELS];
/* Frequencies taken from https://de.wikipedia.org/wiki/Frequenzen_der_gleichstufigen_Stimmung */
                                 /*   Fis,     Gis,     Ais,     cis,     dis,     fis,     gis,     ais,   cis¹,     dis¹,    fis¹,   gis¹ */
float pentatonicScale[CHANNELS] = { 92.5f, 103.83f, 116.54f, 138.59f, 155.56f, 184.99f, 207.65f, 233.08f, 277.18f, 311.13f, 369.99f, 415.3f};

// GUItool: begin automatically generated code
/*AudioSynthWaveform       waveform7;      //xy=239,732
AudioSynthWaveform       waveform8;      //xy=240,788
AudioSynthWaveform       waveform5;      //xy=241,623
AudioSynthWaveform       waveform9;      //xy=241,846
AudioSynthWaveform       waveform6;      //xy=243,677
AudioSynthWaveform       waveform3;      //xy=244,506
AudioSynthWaveform       waveform4;      //xy=244,569
AudioSynthWaveform       waveform2;      //xy=246,440
AudioSynthWaveform       waveform1;      //xy=247,377
AudioSynthWaveform       waveform10;     //xy=245,901
AudioSynthWaveform       waveform11;     //xy=246,951
AudioSynthWaveform       waveform0;           //xy=249,310
AudioEffectEnvelope      envelope;       //xy=470,311
AudioEffectEnvelope      envelope6;      //xy=469,676
AudioEffectEnvelope      envelope7;      //xy=469,733
AudioEffectEnvelope      envelope3;      //xy=470,507
AudioEffectEnvelope      envelope8;      //xy=469,788
AudioEffectEnvelope      envelope1;      //xy=471,377
AudioEffectEnvelope      envelope4;      //xy=471,570
AudioEffectEnvelope      envelope9;      //xy=470,846
AudioEffectEnvelope      envelope5;      //xy=471,623
AudioEffectEnvelope      envelope2;      //xy=473,441
AudioEffectEnvelope      envelope10;     //xy=473,899
AudioEffectEnvelope      envelope11;     //xy=473,949*/
AudioMixer4              mixer1;         //xy=697,417
AudioMixer4              mixer2;         //xy=697,651
AudioMixer4              mixer3;         //xy=700,866
AudioMixer4              mixer4;         //xy=928,648
AudioOutputI2S           i2sOut;         //xy=1122,648
AudioConnection          patchCord12(soundGenerators[0].waveform, soundGenerators[0].envelope);
AudioConnection          patchCord9(soundGenerators[1].waveform, soundGenerators[1].envelope);
AudioConnection          patchCord8(soundGenerators[2].waveform, soundGenerators[2].envelope);
AudioConnection          patchCord6(soundGenerators[3].waveform, soundGenerators[3].envelope);
AudioConnection          patchCord7(soundGenerators[4].waveform, soundGenerators[4].envelope);
AudioConnection          patchCord3(soundGenerators[5].waveform, soundGenerators[5].envelope);
AudioConnection          patchCord5(soundGenerators[6].waveform, soundGenerators[6].envelope);
AudioConnection          patchCord1(soundGenerators[7].waveform, soundGenerators[7].envelope);
AudioConnection          patchCord2(soundGenerators[8].waveform, soundGenerators[8].envelope);
AudioConnection          patchCord4(soundGenerators[9].waveform, soundGenerators[9].envelope);
AudioConnection          patchCord10(soundGenerators[10].waveform, soundGenerators[10].envelope);
AudioConnection          patchCord11(soundGenerators[11].waveform, soundGenerators[11].envelope);
AudioConnection          patchCord13(soundGenerators[0].envelope, 0, mixer1, 0);
AudioConnection          patchCord18(soundGenerators[1].envelope, 0, mixer1, 1);
AudioConnection          patchCord22(soundGenerators[2].envelope, 0, mixer1, 2);
AudioConnection          patchCord16(soundGenerators[3].envelope, 0, mixer1, 3);
AudioConnection          patchCord19(soundGenerators[4].envelope, 0, mixer2, 0);
AudioConnection          patchCord21(soundGenerators[5].envelope, 0, mixer2, 1);
AudioConnection          patchCord14(soundGenerators[6].envelope, 0, mixer2, 2);
AudioConnection          patchCord15(soundGenerators[7].envelope, 0, mixer2, 3);
AudioConnection          patchCord17(soundGenerators[8].envelope, 0, mixer3, 0);
AudioConnection          patchCord20(soundGenerators[9].envelope, 0, mixer3, 1);
AudioConnection          patchCord23(soundGenerators[10].envelope, 0, mixer3, 2);
AudioConnection          patchCord24(soundGenerators[11].envelope, 0, mixer3, 3);
AudioConnection          patchCord25(mixer1, 0, mixer4, 0);
AudioConnection          patchCord26(mixer2, 0, mixer4, 1);
AudioConnection          patchCord27(mixer3, 0, mixer4, 2);
AudioConnection          patchCord28(mixer4, 0, i2sOut, 0);
AudioConnection          patchCord29(mixer4, 0, i2sOut, 1);
AudioControlSGTL5000     audioShield;    //xy=452,203
// GUItool: end automatically generated code


elapsedMillis elapsedMs;

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

    AudioMemory(150);
    audioShield.enable();
    audioShield.micGain(60);  //0-63
    audioShield.volume(0.8);  //0-1

    for (unsigned int i = 0; i < CHANNELS; ++i)
    {
        soundGenerators[i].waveform.begin(WAVEFORM_SINE);
        soundGenerators[i].waveform.amplitude(0.7);
        soundGenerators[i].waveform.frequency(pentatonicScale[i]);

        soundGenerators[i].envelope.attack(10.f);
        soundGenerators[i].envelope.hold(70.f);
        soundGenerators[i].envelope.delay(40.f);
        soundGenerators[i].envelope.sustain(0.f);

        soundGenerators[i].elapsedMs = 0;
    }
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
        // Serial.println(incomingByte);
    }

    if (elapsedMs >= 300)
    {
        soundGenerators[1].envelope.noteOn();
        elapsedMs = 0;
    }
}
