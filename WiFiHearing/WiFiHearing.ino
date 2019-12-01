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

#include <SerialFlash.h>
#include <SPI.h>
#include <SD.h>

#include <Bounce.h>

#include "PacketParser.h"

#include <cmath>

const int numWifiChannels = 13;

const int buttonPin = 33;
Bounce pushbutton = Bounce(buttonPin, 10);  // 10 ms debounce

bool isCyborg = true;

struct SoundGenerator
{
    AudioSynthWaveform waveform;
    AudioEffectEnvelope envelope;
    AudioConnection patchCoord {waveform, envelope};
    elapsedMillis elapsedMs;
    unsigned int rateInMs;
};

SoundGenerator soundGenerators[numWifiChannels];
/* Frequencies taken from https://en.wikipedia.org/wiki/Piano_key_frequencies */
                                              /*    D♯₂,   F♯₂,     G♯₂,     A♯₂,     C♯₃,     D♯₃,     F♯₃,     G♯₃,     A♯₃,    C♯₄,      D♯₄,     F♯₄,    G♯₄ */
const float pentatonicScale[numWifiChannels] = { 77.78f, 92.5f, 103.83f, 116.54f, 138.59f, 155.56f, 184.99f, 207.65f, 233.08f, 277.18f, 311.13f, 369.99f, 415.3f };

                                                      /*    D♯₂,     F♯₃,     C♯₄,     F♯₄,     A♯₄,     D♯₅,     F♯₅,     G♯₅,     A♯₅,      C♯₆,      D♯₆,      G♯₆,      C♯₇ */
const float pentatonicScaleCochlear[numWifiChannels] = { 77.78f, 184.99f, 277.18f, 369.99f, 466.16f, 622.25f, 739.99f, 830.60f, 932.33f, 1108.73f, 1244.51f, 1661.22f, 2217.46f };

// Audio graph
AudioMixer4              mixer1;
AudioMixer4              mixer2;
AudioMixer4              mixer3;
AudioMixer4              mixer4;
AudioOutputI2S           i2sOut;
AudioConnection          patchCord1(soundGenerators[0].envelope, 0, mixer1, 0);
AudioConnection          patchCord2(soundGenerators[1].envelope, 0, mixer1, 1);
AudioConnection          patchCord3(soundGenerators[2].envelope, 0, mixer1, 2);
AudioConnection          patchCord4(soundGenerators[3].envelope, 0, mixer1, 3);
AudioConnection          patchCord5(soundGenerators[4].envelope, 0, mixer2, 0);
AudioConnection          patchCord6(soundGenerators[5].envelope, 0, mixer2, 1);
AudioConnection          patchCord7(soundGenerators[6].envelope, 0, mixer2, 2);
AudioConnection          patchCord8(soundGenerators[7].envelope, 0, mixer2, 3);
AudioConnection          patchCord9(soundGenerators[8].envelope, 0, mixer3, 0);
AudioConnection          patchCord10(soundGenerators[9].envelope, 0, mixer3, 1);
AudioConnection          patchCord11(soundGenerators[10].envelope, 0, mixer3, 2);
AudioConnection          patchCord12(soundGenerators[11].envelope, 0, mixer3, 3);
AudioConnection          patchCord13(soundGenerators[12].envelope, 0, mixer4, 3);
AudioConnection          patchCord14(mixer1, 0, mixer4, 0);
AudioConnection          patchCord15(mixer2, 0, mixer4, 1);
AudioConnection          patchCord16(mixer3, 0, mixer4, 2);
AudioConnection          patchCord17(mixer4, 0, i2sOut, 0);
AudioConnection          patchCord18(mixer4, 0, i2sOut, 1);
AudioControlSGTL5000     audioShield;



// convert packet count to BPM
unsigned int countToBPM(unsigned int packetCount)
{
    //return static_cast<unsigned int>(std::round(std::sqrt(packetCount/4) * 12 + 20)); //curved 
    return static_cast<unsigned int>( packetCount / 6 + 50); // linear 
}


// convert packet count to envelope hold time in ms
float countToHold(unsigned int packetCount)
{
    if (packetCount > 525)
        return 90.f;
    return packetCount * -0.4f + 300.f;
}


unsigned int BPMtoMs(unsigned int BPM)
{
    return static_cast<unsigned int >(std::round((60.f * 1000.f) / BPM));
}


// Returns the gain factor for a given frequency
// so that they have equal loudness
float AWeightedGain(float f)
{
    // see https://en.wikipedia.org/wiki/A-weighting
    float numerator = (12194 * 12194) * std::pow(f, 4);
    float denominator = (f*f + 20.6f * 20.6f) * std::sqrt( (f*f + 107.7f * 107.7f) * (f*f + 737.9f * 737.9f) ) * (f*f + 12194 * 12194);
    float weight = numerator / denominator;
    float gainDB = 20.f * std::log10(weight) + 2.f; // convert to dB and add offset to normalize 0db to 1kHz
    return std::pow(10.f, -gainDB / 20.f); // convert to gain factor in dbFS
}


// callback that is called everytime a channel update packet is succesfully received
void packetParsed(uint8_t channel, uint16_t count)
{
    if (channel > 0 && channel <= numWifiChannels)
    {
        if (count == 0)
            soundGenerators[channel - 1].rateInMs = 0;
        else
        {
            unsigned int BPM = countToBPM(count);
            unsigned int ms = BPMtoMs(BPM);
            /*Serial.print("BPM: ");
            Serial.print(BPM);
            Serial.print(" ms: ");
            Serial.println(ms);*/
            soundGenerators[channel - 1].rateInMs = ms;
            soundGenerators[channel - 1].envelope.hold(countToHold(count));
        }
        //soundGenerators[channel - 1].envelope.noteOn();
        soundGenerators[channel - 1].elapsedMs = 0;
    }
}


PacketParser packetParser(packetParsed);


void setup() {
    Serial4.begin(115200);
    Serial.begin(115200);

    pinMode(buttonPin, INPUT_PULLUP);

    AudioMemory(150);
    audioShield.enable();
    audioShield.volume(0.8);  //0-1

    const float* currentScale = pentatonicScale;
    if (isCyborg)
        currentScale = pentatonicScaleCochlear;

    // setup the sound generators
    for (unsigned int i = 0; i < numWifiChannels; ++i)
    {
        soundGenerators[i].waveform.begin(WAVEFORM_SINE);
        soundGenerators[i].waveform.amplitude( AWeightedGain( currentScale[i] ) );
        soundGenerators[i].waveform.frequency( currentScale[i] );

        soundGenerators[i].envelope.attack(10.f);
        soundGenerators[i].envelope.hold(90.f);
        soundGenerators[i].envelope.delay(40.f);
        soundGenerators[i].envelope.sustain(0.f);

        soundGenerators[i].elapsedMs = 0;
        soundGenerators[i].rateInMs = 0;
    }

    // crude way to avoid clipping
    mixer1.gain(0, 0.25f);
    mixer1.gain(1, 0.25f);
    mixer1.gain(2, 0.25f);
    mixer1.gain(3, 0.25f);
    mixer2.gain(0, 0.25f);
    mixer2.gain(1, 0.25f);
    mixer2.gain(2, 0.25f);
    mixer2.gain(3, 0.25f);
    mixer3.gain(0, 0.25f);
    mixer3.gain(1, 0.25f);
    mixer3.gain(2, 0.25f);
    mixer3.gain(3, 0.25f);
    mixer4.gain(0, 0.25f);
    mixer4.gain(1, 0.25f);
    mixer4.gain(2, 0.25f);
    mixer4.gain(3, 0.0625f);
}


void loop() {
    // parse serial input
    if (Serial4.available() > 0)
    {
        uint8_t incomingByte = Serial4.read();
        packetParser.parseByte(incomingByte);
        // Serial.println(incomingByte);

    }

    // Iterate through the soundGenerators and trigger the envelopes once enough time has passed
    for(auto& soundGenerator: soundGenerators)
    {
        if (soundGenerator.rateInMs != 0 && soundGenerator.elapsedMs >= soundGenerator.rateInMs)
        {
            soundGenerator.envelope.noteOn();
            soundGenerator.elapsedMs = 0;
        }
    }

    // On button press change from the regular to the cochlear implant scale
    if (pushbutton.update()) { // true if pin state changed
        if (pushbutton.fallingEdge()) {  // button was released
            isCyborg = !isCyborg;

            const float* currentScale = pentatonicScale;
            if (isCyborg)
                currentScale = pentatonicScaleCochlear;

            for (unsigned int i = 0; i < numWifiChannels; ++i)
            {
                soundGenerators[i].waveform.amplitude( AWeightedGain( currentScale[i] ) );
                soundGenerators[i].waveform.frequency( currentScale[i] );
            }
        }
    }
}
