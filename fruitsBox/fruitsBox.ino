// Fruit box climate monitor: DHT sensor on an Arduino Nano.
// Prints one labelled line per reading to the serial monitor at 9600 baud
// and mirrors the same reading on a 20x4 I2C LCD.

#include <DHT.h>
#include <LiquidCrystal_I2C.h>

#define DHTPIN 2       // data line, needs a 10k pull-up to 5V
#define DHTTYPE DHT11  // change to DHT22 when the white sensor is replaced

// PCF8574 backpacks answer on 0x27; the PCF8574A variant uses 0x3F instead.
// An I2C scan on this board found the display at 0x27.
#define LCD_ADDR 0x27
const uint8_t LCD_COLS = 20;
const uint8_t LCD_ROWS = 4;

// The DHT needs 2s between reads; polling faster returns stale values.
const unsigned long READ_INTERVAL_MS = 2000;

// Top of the DHT11's usable humidity range. Past this the sensor saturates
// and flatlines instead of erroring, so the reading needs a visible caveat.
const float RH_SENSOR_LIMIT = 90.0;

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);
unsigned long lastRead = 0;
unsigned long lastClockTick = 0;

// Writes text at the start of a row and blanks the rest of it, so a shorter
// reading never leaves digits behind from a longer one (100.0 -> 61.0).
void lcdPrintLine(uint8_t row, const char *text) {
  lcd.setCursor(0, row);
  uint8_t col = 0;
  while (col < LCD_COLS && text[col] != '\0') {
    lcd.write(text[col]);
    col++;
  }
  while (col < LCD_COLS) {
    lcd.write(' ');
    col++;
  }
}

// Header row: title on the left, running uptime on the right. Knowing how long
// the board has been up tells you whether a frozen reading is stale or current.
void lcdPrintHeader() {
  unsigned long seconds = millis() / 1000UL;
  char uptime[9];
  char line[LCD_COLS + 1];

  // %u is 16-bit on AVR, so the hour field wraps past 65535 -- about 7 years
  // of uptime, and well past the ~49-day millis() rollover underneath it.
  snprintf(uptime, sizeof(uptime), "%02u:%02u:%02u",
           (unsigned int)(seconds / 3600UL),
           (unsigned int)((seconds / 60UL) % 60UL),
           (unsigned int)(seconds % 60UL));

  // The literal is padded to put the clock at column 12; no printf field
  // widths, which the AVR's cut-down snprintf does not reliably support.
  snprintf(line, sizeof(line), "Fruit Box   %s", uptime);
  lcdPrintLine(0, line);
}

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ;  // no-op on the Nano, kept so the sketch ports to native-USB boards
  }
  dht.begin();

  lcd.init();
  lcd.backlight();
  lcdPrintLine(0, "Fruit Box");
  lcdPrintLine(1, "Temp:   --.- " "\xDF" "C");
  lcdPrintLine(2, "Hum :   --.- %");
  lcdPrintLine(3, "Starting up...");

  Serial.println(F("Fruit box climate monitor"));
  Serial.println(F("-------------------------"));

  // First read after begin() is often NaN; burn it so the log starts clean.
  dht.readTemperature();
  lastRead = millis();
}

void loop() {
  // The clock only changes once a second; redrawing it every pass through
  // loop() would flood the I2C bus and visibly flicker the row.
  if (millis() - lastClockTick >= 1000) {
    lastClockTick = millis();
    lcdPrintHeader();
  }

  // Subtraction handles the ~49-day millis() rollover; a direct compare would not.
  if (millis() - lastRead < READ_INTERVAL_MS) {
    return;
  }
  lastRead = millis();

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  char line[LCD_COLS + 1];
  char value[6];  // "-99.9" plus terminator

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println(F("Temperature: --.- C   |   Humidity: --.- %   (read failed)"));
    lcdPrintLine(1, "Temp:   --.- " "\xDF" "C");
    lcdPrintLine(2, "Hum :   --.- %");
    lcdPrintLine(3, "Sensor read failed");
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

  // dtostrf right-aligns in a 5-wide field, matching the serial padding.
  // 0xDF is the HD44780 degree glyph; split from "C" so the hex escape ends.
  dtostrf(temperature, 5, 1, value);
  snprintf(line, sizeof(line), "Temp: %s " "\xDF" "C", value);
  lcdPrintLine(1, line);

  dtostrf(humidity, 5, 1, value);
  snprintf(line, sizeof(line), "Hum : %s %%", value);
  lcdPrintLine(2, line);

  if (humidity >= RH_SENSOR_LIMIT) {
    lcdPrintLine(3, "RH at sensor limit");
  } else {
    lcdPrintLine(3, "Status: OK");
  }
}
