#include "EEPROM.h"
#include "Adafruit_SGP30.h"
// Instantiate eeprom objects with parameter/argument names and sizes

EEPROMClass  TVOCBASELINE("eeprom1", 0x200);
EEPROMClass  eCO2BASELINE("eeprom2", 0x100);

boolean isSave = false;
uint16_t TVOC_base, eCO2_base;

uint16_t readTvoc = 0;
uint16_t readCo2 = 0;

Adafruit_SGP30 sgp;
uint32_t getAbsoluteHumidity(float temperature, float humidity) {
  // approximation formula from Sensirion SGP30 Driver Integration chapter 3.15
  const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature)); // [g/m^3]
  const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity); // [mg/m^3]
  return absoluteHumidityScaled;
}

// Update these with values suitable for your network.

#define tvoc_topic "sensor/tvoc"
#define eco2_topic "sensor/eco2"


void initBaseLine() {
  Serial.println("Testing EEPROMClass\n");

  if (!TVOCBASELINE.begin(TVOCBASELINE.length())) {
    Serial.println("Failed to initialise eCO2BASELINE");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }
  if (!eCO2BASELINE.begin(eCO2BASELINE.length())) {
    Serial.println("Failed to initialise eCO2BASELINE");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }
  //eCO2: 0x99F7 & TVOC: 0x935D
  //0x8512 & TVOC: 0x7DD9

  TVOCBASELINE.put(0, 0x7DD9);
  eCO2BASELINE.put(0, 0x8512);
  TVOCBASELINE.commit();
  eCO2BASELINE.commit();
  Serial.println("commit TVOC and eCo2 to EEPROM...");
  isSave = true;
}

void setBaseLine() {

  Serial.println("Done Calibrate");
  TVOCBASELINE.get(0, readTvoc);
  eCO2BASELINE.get(0, readCo2);
  sgp.setIAQBaseline(readCo2, readTvoc);
  Serial.println("Calibrate");
  Serial.print("****Baseline values: eCO2: 0x"); Serial.print(readCo2, HEX);
  Serial.print(" & TVOC: 0x"); Serial.println(readTvoc, HEX);
}
void _initSGP30 () {
  if (! sgp.begin()) {
    Serial.println("Sensor not found :(");
    while (1);
  }
  Serial.print("Found SGP30 serial #");
  Serial.print(sgp.serialnumber[0], HEX);
  Serial.print(sgp.serialnumber[1], HEX);
  Serial.println(sgp.serialnumber[2], HEX);

  //  setBaseLine();
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(15, OUTPUT); // turn on PMS7003
  digitalWrite(15, HIGH); // turn on PMS7003
  delay(500);
  pinMode(32, OUTPUT); // on BME280
  digitalWrite(32, HIGH); // on BME280
    delay(500);
  pinMode(33, OUTPUT); // on i2c
  digitalWrite(33, HIGH); // on i2c
  delay(500);
 

  _initSGP30();

  delay(2000);


}

void getDataSGP30 () {
  // put your main code here, to run repeatedly:
  // If you have a temperature / humidity sensor, you can set the absolute humidity to enable the humditiy compensation for the air quality signals
  //float temperature = 22.1; // [°C]
  //float humidity = 45.2; // [%RH]
  //sgp.setHumidity(getAbsoluteHumidity(temperature, humidity));

  if (! sgp.IAQmeasure()) {
    Serial.println("Measurement failed");
    return;
  }
  Serial.print("TVOC "); Serial.print(sgp.TVOC); Serial.print(" ppb\t");
  Serial.print("eCO2 "); Serial.print(sgp.eCO2); Serial.println(" ppm");


  if (! sgp.IAQmeasureRaw()) {
    Serial.println("Raw Measurement failed");
    return;
  }
  Serial.print("Raw H2 "); Serial.print(sgp.rawH2); Serial.print(" \t");
  Serial.print("Raw Ethanol "); Serial.print(sgp.rawEthanol); Serial.println("");


  if (! sgp.getIAQBaseline(&eCO2_base, &TVOC_base)) {
    Serial.println("Failed to get baseline readings");
    return;
  }

  Serial.print("****Get Baseline values: eCO2: 0x"); Serial.print(eCO2_base, HEX);
  Serial.print(" & TVOC: 0x"); Serial.println(TVOC_base, HEX);



}
void loop() {
  // put your main code here, to run repeatedly:



  if (!isSave) {


    Serial.println("calibrate..");

    getDataSGP30();

    delay(2000);


    ////  sgp.setIAQBaseline(readCo2, readTvoc);

      Serial.print("****Current Baseline values: eCO2: 0x"); Serial.print(readCo2, HEX);
      Serial.print(" & TVOC: 0x"); Serial.println(readTvoc, HEX);
    if (eCO2_base > 0) {
      initBaseLine();
      Serial.println("Set Base Line");
      Serial.println("Get BaseLine");
      TVOCBASELINE.get(0, readTvoc);
      eCO2BASELINE.get(0, readCo2);
      Serial.print("****get Default Baseline values: eCO2: 0x"); Serial.print(readCo2, HEX);
      Serial.print(" & TVOC: 0x"); Serial.println(readTvoc, HEX);
    }
  }
}
