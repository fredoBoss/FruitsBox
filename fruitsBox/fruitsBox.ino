// Fruit box climate monitor: DHT sensor on an Arduino Nano.
// Prints one labelled line per reading to the serial monitor at 9600 baud.

#include <DHT.h>

#define DHTPIN 2       // data line, needs a 10k pull-up to 5V
#define DHTTYPE DHT11  // change to DHT22 when the white sensor is replaced

// The DHT needs 2s between reads; polling faster returns stale values.
const unsigned long READ_INTERVAL_MS = 2000;

DHT dht(DHTPIN, DHTTYPE);
unsigned long lastRead = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ;  // no-op on the Nano, kept so the sketch ports to native-USB boards
  }
  dht.begin();
  Serial.println(F("Fruit box climate monitor"));
  Serial.println(F("-------------------------"));

  // First read after begin() is often NaN; burn it so the log starts clean.
  dht.readTemperature();
  lastRead = millis();
}

void loop() {
  // Subtraction handles the ~49-day millis() rollover; a direct compare would not.
  if (millis() - lastRead < READ_INTERVAL_MS) {
    return;
  }
  lastRead = millis();

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println(F("Temperature: --.- C   |   Humidity: --.- %   (read failed)"));
    return;
  }

  // Pad single-digit values so the columns stay aligned as readings change.
  Serial.print(F("Temperature: "));
  if (temperature < 10.0) Serial.print(' ');
  Serial.print(temperature, 1);
  Serial.print(F(" C   |   Humidity: "));
  if (humidity < 10.0) Serial.print(' ');
  Serial.print(humidity, 1);
  Serial.println(F(" %"));
}
