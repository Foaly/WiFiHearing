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

#include <cmath>

#define CHANNELS 13

struct SoundGenerator
{
    AudioSynthWaveform waveform;
    AudioEffectEnvelope envelope;
    elapsedMillis elapsedMs;
    unsigned int rateInMs;
};

SoundGenerator soundGenerators[CHANNELS];
/* Frequencies taken from https://de.wikipedia.org/wiki/Frequenzen_der_gleichstufigen_Stimmung */
                                 /*    Dis,   Fis,     Gis,     Ais,     cis,     dis,     fis,     gis,     ais,   cis¹,     dis¹,    fis¹,   gis¹ */
float pentatonicScale[CHANNELS] = { 77.78f, 92.5f, 103.83f, 116.54f, 138.59f, 155.56f, 184.99f, 207.65f, 233.08f, 277.18f, 311.13f, 369.99f, 415.3f };

                                         /*    Dis,     Gis,     cis,     fis,     ais,    dis¹,   gis¹,    cis²,    fis²,    ais²,     dis³,     gis³,     cis⁴*/
float pentatonicScaleCochlear[CHANNELS] = { 77.78f, 103.83f, 138.59f, 184.99f, 233.08f, 311.13f, 415.3f, 554.37f, 739.99f, 932.33f, 1244.51f, 1661.22f, 2217.46f};

// GUItool: begin automatically generated code
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
AudioConnection          patchCord30(soundGenerators[12].waveform, soundGenerators[12].envelope);
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
AudioConnection          patchCord31(soundGenerators[12].envelope, 0, mixer4, 3);
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
    Separator,
    Count,
    CountDone
};
Status packetStatus = Status::None;
unsigned int channel = 0;
unsigned int count = 0;


unsigned int saturationCurve(unsigned int packetCount)
{
    return static_cast<unsigned int>(std::round(std::sqrt(packetCount/4) * 12 + 30));
}


unsigned int BPMtoMs(unsigned int BPM)
{
    return static_cast<unsigned int >(std::round((60.f * 1000.f) / BPM));
}


void setup() {
    Serial4.begin(115200);
    Serial.begin(115200);

    AudioMemory(150);
    audioShield.enable();
    audioShield.micGain(60);  //0-63
    audioShield.volume(0.5);  //0-1

    for (unsigned int i = 0; i < CHANNELS; ++i)
    {
        soundGenerators[i].waveform.begin(WAVEFORM_SINE);
        soundGenerators[i].waveform.amplitude(0.7);
        soundGenerators[i].waveform.frequency(pentatonicScaleCochlear[i]);

        soundGenerators[i].envelope.attack(10.f);
        soundGenerators[i].envelope.hold(90.f);
        soundGenerators[i].envelope.delay(40.f);
        soundGenerators[i].envelope.sustain(0.f);

        soundGenerators[i].elapsedMs = 0;
        soundGenerators[i].rateInMs = 1000;
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
                    if (channel > 0 && channel <= CHANNELS)
                    {
                        unsigned int BPM = saturationCurve(count);
                        unsigned int ms = BPMtoMs(BPM);
                        /*Serial.print("BPM: ");
                        Serial.print(BPM);
                        Serial.print(" ms: ");
                        Serial.println(ms);*/
                        //soundGenerators[channel - 1].envelope.noteOn();
                        soundGenerators[channel - 1].rateInMs = ms;
                        soundGenerators[channel - 1].elapsedMs = 0;
                    }
                    channel = 0;
                    count = 0;
                    packetStatus = Status::None;
                }
                break;
        }
        // Serial.println(incomingByte);
    }

    // Iterate through the soundGenerators and trigger the envelopes once enough time has passed
    for(auto& soundGenerator: soundGenerators)
    {
        if (soundGenerator.elapsedMs >= soundGenerator.rateInMs)
        {
            soundGenerator.envelope.noteOn();
            soundGenerator.elapsedMs = 0;
        }
    }
}
