void setup() {
    Serial4.begin(115200);
    Serial.begin(115200);

}

void loop() {
    if (Serial4.available() > 0)
    {
        incomingByte = Serial4.read();
        Serial.print(incomingByte, DEC);
    }
}
