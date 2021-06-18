
float my_vcc_const = 1.1; 

#define coin_amount 1    // кількість монет, які необхідно розрізнати
float coin_value[coin_amount] = {1.0};
String currency = "UAH"; // валюта
int stb_time = 10000;    // час, через який система «засне» (мілісекунди)

int coin_signal[coin_amount]; 
int coin_quantity[coin_amount]; 
byte empty_signal;               
unsigned long standby_timer, reset_timer; 
float summ_money = 0;            

//бібліотеки
#include "LowPower.h"
#include "EEPROMex.h"
#include "LCD_1602_RUS.h"

LCD_1602_RUS lcd(0x27, 16, 2); 

boolean recogn_flag, sleep_flag = true; 
//кнопки
#define button 2         // кнопка, щоб система «прокинулась»
#define calibr_button 3  // кнопка калібрування
#define disp_power 12    // живлення дисплею
#define LEDpin 11        // живлення світлодіода
#define IRpin 17         // живлення фототранзистора
#define IRsens 14        // сигнал фототранзистора
//-------КНОПКИ---------
int sens_signal, last_sens_signal;
boolean coin_flag = false;

void setup() {
  Serial.begin(9600);  // порт для зв’язку з комп’ютером
  delay(500);

  pinMode(button, INPUT_PULLUP);
  pinMode(calibr_button, INPUT_PULLUP);

  pinMode(disp_power, OUTPUT);
  pinMode(LEDpin, OUTPUT);
  pinMode(IRpin, OUTPUT);

  digitalWrite(disp_power, 1);
  digitalWrite(LEDpin, 1);
  digitalWrite(IRpin, 1);

  attachInterrupt(0, wake_up, CHANGE);

  empty_signal = analogRead(IRsens); 

  // ініціалізація дисплею
  lcd.init();
  lcd.backlight();

  if (!digitalRead(calibr_button)) {  // якщо при запуску натиснута клавіша калібрування
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print(L"Сервис");
    delay(500);
    reset_timer = millis();
    while (1) {                                
      if (millis() - reset_timer > 3000) {     
        // очищення кількості монет
        for (byte i = 0; i < coin_amount; i++) {
          coin_quantity[i] = 0;
          EEPROM.writeInt(20 + i * 2, 0);
        }
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(L"Память очищена");
        delay(100);
      }
      if (digitalRead(calibr_button)) {  
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(L"Калибровка");
        break;
      }
    }
    while (1) {
      for (byte i = 0; i < coin_amount; i++) {
        lcd.setCursor(0, 1); lcd.print(coin_value[i]); 
        lcd.setCursor(13, 1); lcd.print(currency);     
        last_sens_signal = empty_signal;
        while (1) {
          sens_signal = analogRead(IRsens);
          if (sens_signal > last_sens_signal) last_sens_signal = sens_signal;
          if (sens_signal - empty_signal > 3) coin_flag = true; 
          if (coin_flag && (abs(sens_signal - empty_signal)) < 2) {     
            coin_signal[i] = last_sens_signal;
            EEPROM.writeInt(i * 2, coin_signal[i]);
            coin_flag = false;
            break;
          }
        }
      }
      break;
    }
  }

  // при запуску системи, підраховує скільки монет в банці
  for (byte i = 0; i < coin_amount; i++) {
    coin_signal[i] = EEPROM.readInt(i * 2);
    coin_quantity[i] = EEPROM.readInt(20 + i * 2);
    summ_money += coin_quantity[i] * coin_value[i];
  }

void loop() {
  if (sleep_flag) {  // ініціалізація дисплею після сну і виведення на екран тексту
    delay(500);
    float volts = readVcc() / 1000;
    lcd.init();
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print(L"В Копилке:");
    lcd.print(" ");
   // lcd.print(volts);
    lcd.setCursor(0, 1); lcd.print(summ_money);
    lcd.setCursor(13, 1); lcd.print(currency);
    empty_signal = analogRead(IRsens);
    sleep_flag = false;
  }

  last_sens_signal = empty_signal;
  while (1) {
    sens_signal = analogRead(IRsens); 
    if (sens_signal > last_sens_signal) last_sens_signal = sens_signal;
    if (sens_signal - empty_signal > 3) coin_flag = true;
    if (coin_flag && (abs(sens_signal - empty_signal)) < 2) {
      recogn_flag = false; 
      for (byte i = 0; i < coin_amount; i++) {
        int delta = abs(last_sens_signal - coin_signal[i]); 
        
        if (delta < 30) {   
          summ_money += coin_value[i];  
          lcd.setCursor(0, 1); lcd.print(summ_money);
          coin_quantity[i]++;  
          recogn_flag = true;
          break;
        }
      }
      coin_flag = false;
      standby_timer = millis(); 
      break;
    }
    // при бездіяльності система засинає
    if (millis() - standby_timer > stb_time) {
      good_night();
      break;
    }
        // відображення на дисплеї кількість монет
        for (byte i = 0; i < coin_amount; i++) {
          lcd.setCursor(i * 3, 0); lcd.print((int)coin_value[i]);
          lcd.setCursor(i * 3, 1); lcd.print(coin_quantity[i]);
        }
      }
    }
  }}
void good_night() {
  // перед відключенням системи, запис в EEPROM нових отриманих результатів
  for (byte i = 0; i < coin_amount; i++) {
    EEPROM.updateInt(20 + i * 2, coin_quantity[i]);
  }
  sleep_flag = true;
  digitalWrite(disp_power, 0);
  digitalWrite(LEDpin, 0);
  digitalWrite(IRpin, 0);
  delay(100);
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
}

void wake_up() {
  digitalWrite(disp_power, 1);
  digitalWrite(LEDpin, 1);
  digitalWrite(IRpin, 1);
  standby_timer = millis(); 
}
long readVcc() { //функція зчитування внутрішньої опорної напруги, універсальна для всіх Arduino
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
  ADMUX = _BV(MUX3) | _BV(MUX2);
#else
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA, ADSC)); // measuring
  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both
  long result = (high << 8) | low;

  result = my_vcc_const * 1023 * 1000 / result;
  return result; }
float my_vcc_const = 1.1; 

#define coin_amount 1    // кількість монет, які необхідно розрізнати
float coin_value[coin_amount] = {1.0};
String currency = "UAH"; // валюта
int stb_time = 10000;    // час, через який система «засне» (мілісекунди)

int coin_signal[coin_amount]; 
int coin_quantity[coin_amount]; 
byte empty_signal;               
unsigned long standby_timer, reset_timer; 
float summ_money = 0;            

//бібліотеки
#include "LowPower.h"
#include "EEPROMex.h"
#include "LCD_1602_RUS.h"

LCD_1602_RUS lcd(0x27, 16, 2); 

boolean recogn_flag, sleep_flag = true; 
//кнопки
#define button 2         // кнопка, щоб система «прокинулась»
#define calibr_button 3  // кнопка калібрування
#define disp_power 12    // живлення дисплею
#define LEDpin 11        // живлення світлодіода
#define IRpin 17         // живлення фототранзистора
#define IRsens 14        // сигнал фототранзистора
//-------КНОПКИ---------
int sens_signal, last_sens_signal;
boolean coin_flag = false;

void setup() {
  Serial.begin(9600);  // порт для зв’язку з комп’ютером
  delay(500);

  pinMode(button, INPUT_PULLUP);
  pinMode(calibr_button, INPUT_PULLUP);

  pinMode(disp_power, OUTPUT);
  pinMode(LEDpin, OUTPUT);
  pinMode(IRpin, OUTPUT);

  digitalWrite(disp_power, 1);
  digitalWrite(LEDpin, 1);
  digitalWrite(IRpin, 1);

  attachInterrupt(0, wake_up, CHANGE);

  empty_signal = analogRead(IRsens); 

  // ініціалізація дисплею
  lcd.init();
  lcd.backlight();

  if (!digitalRead(calibr_button)) {  // якщо при запуску натиснута клавіша калібрування
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print(L"Сервис");
    delay(500);
    reset_timer = millis();
    while (1) {                                
      if (millis() - reset_timer > 3000) {     
        // очищення кількості монет
        for (byte i = 0; i < coin_amount; i++) {
          coin_quantity[i] = 0;
          EEPROM.writeInt(20 + i * 2, 0);
        }
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(L"Память очищена");
        delay(100);
      }
      if (digitalRead(calibr_button)) {  
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(L"Калибровка");
        break;
      }
    }
    while (1) {
      for (byte i = 0; i < coin_amount; i++) {
        lcd.setCursor(0, 1); lcd.print(coin_value[i]); 
        lcd.setCursor(13, 1); lcd.print(currency);     
        last_sens_signal = empty_signal;
        while (1) {
          sens_signal = analogRead(IRsens);
          if (sens_signal > last_sens_signal) last_sens_signal = sens_signal;
          if (sens_signal - empty_signal > 3) coin_flag = true; 
          if (coin_flag && (abs(sens_signal - empty_signal)) < 2) {     
            coin_signal[i] = last_sens_signal;
            EEPROM.writeInt(i * 2, coin_signal[i]);
            coin_flag = false;
            break;
          }
        }
      }
      break;
    }
  }

  // при запуску системи, підраховує скільки монет в банці
  for (byte i = 0; i < coin_amount; i++) {
    coin_signal[i] = EEPROM.readInt(i * 2);
    coin_quantity[i] = EEPROM.readInt(20 + i * 2);
    summ_money += coin_quantity[i] * coin_value[i];
  }

void loop() {
  if (sleep_flag) {  // ініціалізація дисплею після сну і виведення на екран тексту
    delay(500);
    float volts = readVcc() / 1000;
    lcd.init();
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print(L"В Копилке:");
    lcd.print(" ");
   // lcd.print(volts);
    lcd.setCursor(0, 1); lcd.print(summ_money);
    lcd.setCursor(13, 1); lcd.print(currency);
    empty_signal = analogRead(IRsens);
    sleep_flag = false;
  }

  last_sens_signal = empty_signal;
  while (1) {
    sens_signal = analogRead(IRsens); 
    if (sens_signal > last_sens_signal) last_sens_signal = sens_signal;
    if (sens_signal - empty_signal > 3) coin_flag = true;
    if (coin_flag && (abs(sens_signal - empty_signal)) < 2) {
      recogn_flag = false; 
      for (byte i = 0; i < coin_amount; i++) {
        int delta = abs(last_sens_signal - coin_signal[i]); 
        
        if (delta < 30) {   
          summ_money += coin_value[i];  
          lcd.setCursor(0, 1); lcd.print(summ_money);
          coin_quantity[i]++;  
          recogn_flag = true;
          break;
        }
      }
      coin_flag = false;
      standby_timer = millis(); 
      break;
    }
    // при бездіяльності система засинає
    if (millis() - standby_timer > stb_time) {
      good_night();
      break;
    }
        // відображення на дисплеї кількість монет
        for (byte i = 0; i < coin_amount; i++) {
          lcd.setCursor(i * 3, 0); lcd.print((int)coin_value[i]);
          lcd.setCursor(i * 3, 1); lcd.print(coin_quantity[i]);
        }
      }
    }
  }}
void good_night() {
  // перед відключенням системи, запис в EEPROM нових отриманих результатів
  for (byte i = 0; i < coin_amount; i++) {
    EEPROM.updateInt(20 + i * 2, coin_quantity[i]);
  }
  sleep_flag = true;
  digitalWrite(disp_power, 0);
  digitalWrite(LEDpin, 0);
  digitalWrite(IRpin, 0);
  delay(100);
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
}

void wake_up() {
  digitalWrite(disp_power, 1);
  digitalWrite(LEDpin, 1);
  digitalWrite(IRpin, 1);
  standby_timer = millis(); 
}
long readVcc() { //функція зчитування внутрішньої опорної напруги, універсальна для всіх Arduino
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
  ADMUX = _BV(MUX3) | _BV(MUX2);
#else
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif
  delay(2);
  ADCSRA |= _BV(ADSC); 
  while (bit_is_set(ADCSRA, ADSC)); 
  uint8_t low  = ADCL;
  uint8_t high = ADCH; 
  long result = (high << 8) | low;

  result = my_vcc_const * 1023 * 1000 / result;
  return result; }
