// #include <uFire_SHT20_JSON.h>
// #include <uFire_SHT20_MP.h>
#include <uFire_SHT20.h>

#include <Arduino.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <lorawan.h>

// Define sleep time in seconds
#define SLEEP_TIME 60

// For temp and humid sensor
uFire_SHT20 sht20;

// OTAA credentials for V2 stack
// const char *devEui = "0011EDAD6A519F3B";
// const char *appEui = "70B3D57ED004308E";
// const char *appKey = "404FE3A80194921F87279699A9360B56";

// OTAA credentials for V3 stack
// Errors:
//      - DevNonce has already been used
//          - this one is gone after a while, but after successfull join => Forward join-accept to application server
//            and then nothing ...
//      - uplink channel not found
const char *devEui = "0A6ECF8F6FC2644";
const char *appEui = "70B3D57ED004307B";
const char *appKey = "FEBFD438B20A3973175F28A0FB8C4934C";

// const unsigned long interval = 30000;    // 10 s interval to send message
// unsigned long previousMillis = 0;  // will store last time message sent
unsigned int counter = 0;     // message counter

char myStr[50];
char outStr[255];
byte recvStatus = 0;

const sRFM_pins RFM_pins = {
  .CS = 10,
  .RST = 9,
  .DIO0 = 2,
  .DIO1 = 5,
  .DIO2 = 6,
  .DIO5 = 8,
};

void setup()
{
    // Pull all pins low to save power
    for (int i = 2; i < 20; i++) {
        pinMode(i, OUTPUT);
        digitalWrite(i, LOW);
    }

	pinMode(LED_BUILTIN, OUTPUT);
    watchdogSetup();
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);

    delay(100);
    Wire.begin(); // I2C setup
    delay(100);
    sht20.begin(); // temp and humid sensor
    delay(100);
    Serial.begin(9600);
    delay(100);
    setup_lorawan();
}

void loop()
{
    // sprintf(myStr, "Counter-%d", counter);
    const float temp = sht20.temperature();
    delay(100);
    // Serial.print("Temperature: %d°C", temp);
    sprintf(myStr, "%f°C", temp);
    // sprintf(myStr, (String)sht20.temperature() + "°C"); 


    Serial.print("Sending: ");
    Serial.println(myStr);
    
    lora.sendUplink(myStr, strlen(myStr), 0, 1);
    counter++;

    recvStatus = lora.readData(outStr);
    if(recvStatus) {
        Serial.println(outStr);
    }
    
    // Check Lora RX
    lora.update();

    // go to sleep
    go_to_sleep(SLEEP_TIME);
}

void setup_lorawan() {
    while(!Serial);
    if(!lora.init()){
        Serial.println("RFM95 not detected");
        delay(5000);
        return;
    }

    // Set LoRaWAN Class change CLASS_A or CLASS_C
    lora.setDeviceClass(CLASS_A);

    // Set Data Rate
    lora.setDataRate(SF9BW125);

    // set channel to random
    lora.setChannel(MULTI);
    
    // Put OTAA Key and DevAddress here
    lora.setDevEUI(devEui);
    lora.setAppEUI(appEui);
    lora.setAppKey(appKey);

    // Join procedure
    bool isJoined;
    do {
        Serial.println("Joining...");
        isJoined = lora.join();
        
        //wait for 10s to try again
        delay(10000);
    }while(!isJoined);
    Serial.println("Joined to network");
}

void go_to_sleep(int timeToSleep) {
    sleep_enable();
    for (int i = 0; i < (timeToSleep / 8); i++) {
        wdt_reset();
        cli(); // clear interupts
        sleep_enable(); // set SE (sleep enable) bit to 1
        ADCSRA &= ~(1 << 7); // disable DAC
        // power_all_disable();
        // power_down();
        sei(); // enable interupts
        // Disable bod during sleep
        // sleep_bod_disable();
        MCUCR |= (3 << 5);
        MCUCR = (MCUCR & ~(1 << 5)) | (1 << 6);
        sleep_cpu(); // Sleep starts
    }
    sleep_disable(); // set SE bit to 0
    power_all_enable();
}

void watchdogSetup() {
    cli();
    wdt_reset();
    WDTCSR |= (1<<WDCE) | (1<<WDE);
    WDTCSR = (1<<WDIE) | (0<<WDE) | (1<<WDP3) | (1<<WDP0);  // 8s / interrupt, no system reset
    sei();
}

ISR(WDT_vect){
    
}