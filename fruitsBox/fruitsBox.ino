

#include <DHT.h>
#include <LiquidCrystal_I2C.h>

#define DHTPIN 2       
#define DHTTYPE DHT11  


#define LCD_ADDR 0x27
const uint8_t LCD_COLS = 20;
const uint8_t LCD_ROWS = 4;

.
#if DHTTYPE == DHT11
const unsigned long READ_INTERVAL_MS = 1000;
#else
const unsigned long READ_INTERVAL_MS = 2000;
#endif

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);
unsigned long lastRead = 0;
unsigned long lastClockTick = 0;


const char *const MOLD_RISK[5] = {"LOW", "LOW-MOD", "MOD-HIGH", "HIGH", "HIGH"};

  uint8_t tempBand;
  if (temperature <= 15.0) {
    tempBand = 0;
  } else if (temperature <= 20.0) {
    tempBand = 1;
  } else if (temperature <= 25.0) {
    tempBand = 2;
  } else if (temperature <= 30.0) {
    tempBand = 3;
  } else {
    tempBand = 4;
  }

  
  uint8_t humidityBand;
  if (humidity <= 60.0) {
    humidityBand = 0;
  } else if (humidity <= 70.0) {
    humidityBand = 1;
  } else if (humidity <= 80.0) {
    humidityBand = 2;
  } else if (humidity <= 90.0) {
    humidityBand = 3;
  } else {
    humidityBand = 4;
  }

  return (tempBand > humidityBand) ? tempBand : humidityBand;
}


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


void lcdPrintHeader() {
  unsigned long seconds = millis() / 1000UL;
  char uptime[9];
  char line[LCD_COLS + 1];

  
  snprintf(uptime, sizeof(uptime), "%02u:%02u:%02u",
           (unsigned int)(seconds / 3600UL),
           (unsigned int)((seconds / 60UL) % 60UL),
           (unsigned int)(seconds % 60UL));

  
  snprintf(line, sizeof(line), "Fruit Box   %s", uptime);
  lcdPrintLine(0, line);
}

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ;  
  dht.begin();

  lcd.init();
  lcd.backlight();
  lcdPrintLine(0, "Fruit Box");
  lcdPrintLine(1, "Temp:   --.- " "\xDF" "C");
  lcdPrintLine(2, "Hum :   --.- %");
  lcdPrintLine(3, "Mold risk: --");

  Serial.println(F("Fruit box climate monitor"));
  
  Serial.print(F("build "));
  Serial.println(F(__DATE__ " " __TIME__));
  Serial.println(F("-------------------------"));

  
  dht.readTemperature();
  lastRead = millis();
}

void loop() {
  
  if (millis() - lastClockTick >= 1000) {
    lastClockTick = millis();
    lcdPrintHeader();
  }

  if (millis() - lastRead < READ_INTERVAL_MS) {
    return;
  }

  float humidity = dht.readHumidity(true);
  float temperature = dht.readTemperature();

  lastRead = millis();

  char line[LCD_COLS + 1];
  char value[6];  // 

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println(F("Temperature: --.- C   |   Humidity: --.- %   (read failed)"));
    lcdPrintLine(1, "Temp:   --.- " "\xDF" "C");
    lcdPrintLine(2, "Hum :   --.- %");
    lcdPrintLine(3, "Mold risk: --");
    return;
  }

  Serial.print(F("Temperature: "));
  if (temperature < 10.0) Serial.print(' ');
  Serial.print(temperature, 1);
  Serial.print(F(" C   |   Humidity: "));
  if (humidity < 10.0) Serial.print(' ');
  Serial.print(humidity, 1);
  Serial.print(F(" %   |   Mold risk: "));
  Serial.println(MOLD_RISK[moldRiskIndex(temperature, humidity)]);

  dtostrf(temperature, 5, 1, value);
  snprintf(line, sizeof(line), "Temp: %s " "\xDF" "C", value);
  lcdPrintLine(1, line);

  dtostrf(humidity, 5, 1, value);
  snprintf(line, sizeof(line), "Hum : %s %%", value);
  lcdPrintLine(2, line);

  snprintf(line, sizeof(line), "Mold risk: %s",
           MOLD_RISK[moldRiskIndex(temperature, humidity)]);
  lcdPrintLine(3, line);
}
