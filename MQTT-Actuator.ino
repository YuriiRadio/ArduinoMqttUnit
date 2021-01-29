/*
  Actuator for smart home, via MQTT over Ethernet protocols
  Виконувальний пристрій для розумного дому, через протоколі MQTT over Ethernet
  Platform: Arduino Mega 2560
  
  Yurii Radio 2017-2021
  v3.2 (Fix errors)
  yurii.radio@gmail.com

*/
#define DEBUG 0 // Constant for debugging on the serial port

#define ON 1    // Constant for the on state
#define OFF 0   // Constant for the off state
#define LCD_PRESENT 1 // The presence of an LCD display
#define DHT_PRESENT 1 // Availability of DHT temperature / humidity sensors
#define ENERGYMON_PRESENT 1 // The presence of current and voltage sensors
#define PIR_PRESENT 1 // Motion sensors - PIR (Passive Infra-Red) SR501 or a smoke sensor connected via TTL/CMOS converter
#define FR_PRESENT 1 // Light sensors, analog photoresistor 5мм GL5528, connected to Arduino analog inputs
#define LM_PRESENT 1 // Temperature sensors, analog sensors, such as LM35, connected to Arduino analog inputs

#include <SPI.h>          // Ethernet shield
#include <Ethernet.h>     // Ethernet shield
#include <PubSubClient.h> // MQTT

const byte input_pins[] = {42, 44}; // Контакти Arduino які будуть використовуватись як входи
const byte output_pins[] = {43, 45, 47, 49, 27}; // (!!!27 Invert - siren!!!) Контакти Arduino які будуть використовуватись як виходи. Може бути більше ніж кількість входів. Ті що виходять за межі кнопок - перемикатися конопками не будуть, але можна керувати через MQTT
byte output_pins_states[] = {OFF, OFF, OFF, OFF, ON}; // Початкові стани вихідних контактів. Обовязково дорівнює кількості виходів!!!
const byte input_on_state = 0; // 1, 0 - стан входу - коли натиснена кнопка
const byte output_on_state = 0; // 1, 0 - стан виходу - коли ввімкнено
unsigned int debounce_button_delay = 100; // Затримка в мілісекундах для подолання дребезгу контактів кнопки
byte buttons_states[sizeof(input_pins)]; // Масив події натиснення кнопки, по замовчуванню 0, рівнятися кількості кнопок
byte last_buttons_states[sizeof(input_pins)]; // Масив останніх станів кнопок, по замовчуванню 1, рівнятися кількості кнопок
unsigned long last_debounce_time[sizeof(input_pins)]; // Обовязково unsigned long, щоб при переповненні millis(), різниця millis() - last_debounce_time[i] була позитивна

#if LCD_PRESENT // LCD section
#include <LiquidCrystal_I2C.h> // LCD
LiquidCrystal_I2C lcd(0x27, 16, 2); // Задаємо адресу і розширення дисплею
unsigned int lcd_backlight_delay = 10000; // Час підсвітки дисплею.
unsigned long last_lcd_backlight_time = 0; // Обовязково unsigned long, щоб при переповненні millis(), різниця millis() - last_lcd_backlight_time була позитивна
byte lcd_backlight_flag = 0; // Флаг для підсвітки
#endif

// Ethernet section
byte mac[] = {0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED}; // mac адреса виконувального пристрою
IPAddress ip(192, 168, 1, 100); // Ай-пі адреса виконувального пристрою (Arduino)
IPAddress server(192, 168, 1, 99); // Ай-пі адреса серверу, де встановлений брокер MQTT
EthernetClient ethClient; // Ініціалізуємо Ethernet клієнт

// MQTT section
PubSubClient mqttClient(ethClient);   // Ініціалізуємо MQTT клієнт
unsigned long last_mqtt_reconnect_time = 0; // Обовязково unsigned long, щоб при переповненні millis(), різниця millis() - last_mqtt_reconnect_time була позитивна
unsigned int mqtt_delay = 60000; // Затримка в мілісекундах перед повторною спробою підключитися до MQTT брокера
const char mqtt_username[] = "mqttusr";
const char mqtt_password[] = "mqttusr";
String mqtt_client_name  = "MQTT-Client-01";
String mqtt_client_subscribe = mqtt_client_name + "/relays/#";
String mqtt_client_relay_topic = mqtt_client_name + "/relays/";

byte i; //Глобальна змінна для лічильників в функціях
String publish_str; //Глобальна змінна для публікації в функціях

#if DHT_PRESENT //DHT11 Or DHT22
#include <DHT.h>
//#define DHTTYPE DHT11 // DHT 11
#define DHTTYPE DHT22  // DHT 22  (AM2302), AM2321
const byte dht_input_pins[] = {26, 28}; // Цифрові контактів, до яких будуть підключені цифрові датчики DHT
DHT dht1(dht_input_pins[0], DHTTYPE);
DHT dht2(dht_input_pins[1], DHTTYPE);
unsigned int dht_delay = 30000; // Час затримки на опитування датчика.
unsigned long last_dht_time = 0; // Обовязково unsigned long, щоб при переповненні millis(), різниця millis() - last_dht_time була позитивна
String mqtt_client_dht1_humidity_topic = mqtt_client_name + "/DHT-hum/1";
String mqtt_client_dht1_temperature_topic = mqtt_client_name + "/DHT-temp/1";
String mqtt_client_dht2_humidity_topic = mqtt_client_name + "/DHT-hum/2";
String mqtt_client_dht2_temperature_topic = mqtt_client_name + "/DHT-temp/2";
#endif

#if PIR_PRESENT
// Motion sensors PIR (Passive Infra-Red) SR501 L - режим, затримка мінімальна - 5 сек, підключені через TTL/CMOS converter (2,2K, 10K, 2N2222)
unsigned int pir_init_delay = 30000; // Затримка в мілісекундах для ініціалізації датчиків
byte pir_init_delay_trigger = 0;
unsigned int pir_delay = 30000; // Час затримки на опитування датчика після спрацювання.
const byte pir_input_pins[] = {22, 23, 24, 25, 46, 48}; // Цифрові контакти Arduino куди підключаються виходи PIR датчиків чи датчиків диму
byte pirs_states[sizeof(pir_input_pins)]; // Flag sensor triggered, default 0
unsigned long last_pirs_time[sizeof(pir_input_pins)]; // Обовязково unsigned long, щоб при переповненні millis(), різниця millis() - last_pirs_time[i] була позитивна
String mqtt_client_pir_topic = mqtt_client_name + "/pirs/";
#endif

#if ENERGYMON_PRESENT
#include <PZEM004Tv30.h> // PZEM004T v3.0
PZEM004Tv30 pzem(&Serial1);  // Hardware Serial 1 (18-19); PZEM004Tv30 pzem(14, 15); //Rx, Tx connect to Tx, Rx of PZEM
unsigned int energymon_delay = 10000; // Час затримки на опитування датчика.
unsigned long last_energymon_time = 0; // Обовязково unsigned long, щоб при переповненні millis(), різниця millis() - last_energymon_time була позитивна
String mqtt_client_energymon_L1_voltage_topic = mqtt_client_name + "/L1-voltage";
String mqtt_client_energymon_L1_current_topic = mqtt_client_name + "/L1-current";
String mqtt_client_energymon_L1_power_topic = mqtt_client_name + "/L1-power";
String mqtt_client_energymon_L1_energy_topic = mqtt_client_name + "/L1-energy";
String mqtt_client_energymon_L1_frequency_topic = mqtt_client_name + "/L1-frequency";
String mqtt_client_energymon_L1_pf_topic = mqtt_client_name + "/L1-pf";
#endif

#if FR_PRESENT
// Датчики освітлення, аналоговий фоторезистор - 5мм GL5528
unsigned int fr_delay = 30000; // Час затримки на опитування датчиків
const byte fr_input_pins[] = {8, 9}; // Аналогові контакти Arduino куди підключені фоторезистори
unsigned long last_frs_time[sizeof(fr_input_pins)]; // Обовязково unsigned long, щоб при переповненні millis(), різниця millis() - last_frs_time[i] була позитивна
String mqtt_client_fr_topic = mqtt_client_name + "/FotoRs/";
#endif

#if LM_PRESENT
// Датчики температури - LM35
unsigned int lm_delay = 60000; // Час затримки на опитування датчиків
const byte lm_input_pins[] = {3}; // Аналогові контакти Arduino куди підключені Датчики температури
unsigned long last_lms_time[sizeof(lm_input_pins)]; // Обовязково unsigned long, щоб при переповненні millis(), різниця millis() - last_lms_time[i] була позитивна
String mqtt_client_lm_topic = mqtt_client_name + "/LM-temp/";
#endif

#if LCD_PRESENT
/* Функція для вимикання підсвітки дисплею на певний час (lcd_backlight_delay мілісекунд) */
void lcdBacklight(byte backlight = OFF) {
  if (backlight) { // Вмикаємо підсвітку
    lcd.backlight();
    last_lcd_backlight_time = millis();
    lcd_backlight_flag = 1;
  }

  if (lcd_backlight_flag) {
    if ((millis() - last_lcd_backlight_time) >= lcd_backlight_delay) { // Вимикаємо підсвітку через lcd_backlight_delay мілісекунд
      lcd.noBacklight();
      lcd_backlight_flag = 0;
    }
  }
}
#endif

#if DHT_PRESENT
void dhtProcess(void) {
  if ((millis() - last_dht_time) >= dht_delay) {
    if (mqttClient.connected()) {
      publish_str = String(dht1.readHumidity());
      mqttClient.publish(mqtt_client_dht1_humidity_topic.c_str(), publish_str.c_str()); // Аргументи char*
      publish_str = String(dht1.readTemperature());
      mqttClient.publish(mqtt_client_dht1_temperature_topic.c_str(), publish_str.c_str()); // Аргументи char*

      publish_str = String(dht2.readHumidity());
      mqttClient.publish(mqtt_client_dht2_humidity_topic.c_str(), publish_str.c_str()); // Аргументи char*
      publish_str = String(dht2.readTemperature());
      mqttClient.publish(mqtt_client_dht2_temperature_topic.c_str(), publish_str.c_str()); // Аргументи char*
    }
#if DEBUG
    Serial.println("Read DHT1 OK, Hum: " + String(dht1.readHumidity()) + " Temp: " + String(dht1.readTemperature()));
    Serial.println("Read DHT2 OK, Hum: " + String(dht2.readHumidity()) + " Temp: " + String(dht2.readTemperature()));
#endif
    last_dht_time = millis();
  }
}
#endif

/*#if PIR_PRESENT
void pirProcess(void) {
  if (!pir_init_delay_trigger) { //Затримка для ініціалізації датчиків
    if ((millis() - pir_init_delay) >= 0) {
      pir_init_delay_trigger = 1;
    }
  }
  //Обробка датчиків, після витримки ініціалізації
  if (pir_init_delay_trigger) {
    for (i = 0; i < sizeof(pir_input_pins); i++) {
      if ((millis() - last_pirs_time[i]) >= pir_delay) {
        if (!digitalRead(pir_input_pins[i])) {
          if (mqttClient.connected()) {
            mqttClient.publish((mqtt_client_pir_topic + String(i)).c_str(), "1"); // Аргументи char*
          }
#if DEBUG
          Serial.println("PIR #" + String(i) + " Value: 1");
#endif
        } else {
          if (mqttClient.connected()) {
            mqttClient.publish((mqtt_client_pir_topic + String(i)).c_str(), "0"); // Аргументи char*
          }
#if DEBUG
          Serial.println("PIR #" + String(i) + " Value: 0");
#endif
        }//End if status
        last_pirs_time[i] = millis();
      }//End if pause
    }//End for
  }//End Обробка датчиків
}
#endif*/

#if PIR_PRESENT
void pirProcess(void) {
  if (!pir_init_delay_trigger) { //Затримка для ініціалізації датчиків
    if ((millis() - pir_init_delay) >= 0) {
      pir_init_delay_trigger = 1;
    }
  }
  //Обробка датчиків, після витримки ініціалізації
  if (pir_init_delay_trigger) {
    for (i = 0; i < sizeof(pir_input_pins); i++) {
    
    if (!digitalRead(pir_input_pins[i]) && !pirs_states[i]) {
      pirs_states[i] = 1; // Flag sensor triggered, default 0
      last_pirs_time[i] = millis(); // Reset timer
      if (mqttClient.connected()) {
        mqttClient.publish((mqtt_client_pir_topic + String(i)).c_str(), "1"); // Аргументи char*
      }
#if DEBUG
      //Serial.println("PIR #" + String(i) + " Value: 1");
      Serial.print("PIR #");Serial.print(i);Serial.println(" Value: 1");
#endif      
    }
    
    if ((millis() - last_pirs_time[i]) >= pir_delay) { // last_pirs_time[i] default 0
      if (digitalRead(pir_input_pins[i])) {
        pirs_states[i] = 0; // Reset flag
        if (mqttClient.connected()) {
          mqttClient.publish((mqtt_client_pir_topic + String(i)).c_str(), "0"); // Аргументи char*
        }
#if DEBUG
        //Serial.println("PIR #" + String(i) + " Value: 0");
        Serial.print("PIR #");Serial.print(i);Serial.println(" Value: 0");
#endif
      }
      last_pirs_time[i] = millis(); // Reset timer Для затримки відправки 0
        }//End if pause
    
    }//End for
  }//End Обробка датчиків
}
#endif

#if FR_PRESENT
void frProcess(void) {
  for (i = 0; i < sizeof(fr_input_pins); i++) {
    if ((millis() - last_frs_time[i]) >= fr_delay) {
        byte value = map(analogRead(fr_input_pins[i]), 25, 1020, 0, 100); //(fr_input_pins[i]), 0, 1023, 1, 100);
        if (mqttClient.connected()) {
          publish_str = String(value);
          mqttClient.publish((mqtt_client_fr_topic + String(i)).c_str(), publish_str.c_str()); // Аргументи char*;
        }
#if DEBUG
        Serial.print("FR #"); Serial.print(i); Serial.print(" Value: "); Serial.println(value);
#endif
      last_frs_time[i] = millis();
    }//End if pause
  }//End for
}
#endif

#if LM_PRESENT
void lmProcess(void) {
  for (i = 0; i < sizeof(lm_input_pins); i++) {
    if ((millis() - last_lms_time[i]) >= lm_delay) {
        float temp = (analogRead(lm_input_pins[i])/1023.0)*5.0*1000/10; //LM35
        if (mqttClient.connected()) {
          publish_str = String(temp, 2);
          mqttClient.publish((mqtt_client_lm_topic + String(i)).c_str(), publish_str.c_str()); // Аргументи char*;
        }
#if DEBUG
        Serial.print("LM #"); Serial.print(i); Serial.print(" Temp: "); Serial.println(temp);
#endif
      last_lms_time[i] = millis();
    }//End if pause
  }//End for
}
#endif

/* Функція яка перевіряє статус кнопок NEW */
void checkButtonsStates(void) {
  for (i = 0; i < sizeof(input_pins); i++) {
    
  if (digitalRead(input_pins[i]) != last_buttons_states[i]) { // last_buttons_states[i], default 1 for input_on_state 0 | default 0 for input_on_state 1
    last_debounce_time[i] = millis(); // Reset timer
    buttons_states[i] = 1; // Set flag (button is pressed); buttons_states[i] default 0
  } 

  if (buttons_states[i]) {
    if ((millis() - last_debounce_time[i]) >= debounce_button_delay) { // Переповнення не буде, так як last_debounce_time[i] - unsigned long (невідємне ціле); millis() 50 days reset, micros() - for test 70 minutes reset
      if (input_on_state ? digitalRead(input_pins[i]) : !digitalRead(input_pins[i])) {
        // Кнопка input_pins[i] натиснута
        // Це може бути клік, а може і помилковий сигнал (дребезг), виникає в момент замикання/розмикання контактів кнопки
        // тому даємо кнопці повністю "заспокоїтися" ...
        // Якщо затримка витримана та кнопка ще натиснута - міняємо стан вихідного контакту на протилежний
        switchOutputPin(output_pins[i]);
#if DEBUG
        Serial.println("Button id: " + String(i) + " - last debounce time: " + String(last_debounce_time[i]) + "; status: " + String(output_pins_states[i]) + ";");
#endif
      } 
    buttons_states[i] = 0; // Reset flag
    }
  }
    last_buttons_states[i] = digitalRead(input_pins[i]);
  
  } // End For
}

/* Функція для зміни стану вихідного контакту - перемикання */
void switchOutputPin(byte pin) {
  for (i = 0; i < sizeof(output_pins); i++) { // Шукаємо індекс піна в масиві output_pins
    if (output_pins[i] == pin) { // Якщо номер піна дорівнює переданому, виконуємо перемикання
      if (output_pins_states[i] == ON) { // Якщо було ввімкнено - вимкнемо
        output_pins_states[i] = OFF;
        //digitalWrite(pin, OFF);
        output_on_state ? digitalWrite(pin, 0) : digitalWrite(pin, 1);
      } else { // Якщо було вимкнемо - ввімкнемо
        output_pins_states[i] = ON;
        //digitalWrite(pin, ON);
        output_on_state ? digitalWrite(pin, 1) : digitalWrite(pin, 0);
      }
      break; // Якщо знайшли контакт - виходимо з циклу
    }
  }
  if (mqttClient.connected()) {
    mqttClient.publish((mqtt_client_relay_topic + String(i)).c_str(), output_pins_states[i] ? "1" : "0"); // Аргументи char*
  }
#if LCD_PRESENT
  lcd.clear();      // очистка дисплею
  lcdBacklight(ON); // Вмикаємо підсвітку на певний час
  lcd.print("Button id: " + String(i)); // Выводим текст
  lcd.setCursor(0, 1); // Встановлюємо курсор в початок 2 рядка
  lcd.print("status: " + String(output_pins_states[i] ? "ON" : "OFF"));
#endif
}

#if ENERGYMON_PRESENT
void energymonProcess(void) {
  if ((millis() - last_energymon_time) >= energymon_delay) {
    float voltage = pzem.voltage();
    float current = pzem.current();
    float power = pzem.power();
    float energy = pzem.energy();
    float frequency = pzem.frequency();
    float pf = pzem.pf();

    if (!isnan(voltage)) {
      publish_str= String(voltage, 1);
      if (mqttClient.connected()) {
        mqttClient.publish(mqtt_client_energymon_L1_voltage_topic.c_str(), publish_str.c_str()); // Аргументи char*
      }
#if DEBUG
      Serial.print("Voltage: "); Serial.print(publish_str); Serial.println("V");
#endif
    } else {
      if (mqttClient.connected()) {
        mqttClient.publish(mqtt_client_energymon_L1_voltage_topic.c_str(), "NAN");
      }
#if DEBUG
      Serial.println("Error reading voltage");
#endif
    }

    if (!isnan(current)) {
      publish_str = String(current);
      if (mqttClient.connected()) {
        mqttClient.publish(mqtt_client_energymon_L1_current_topic.c_str(), publish_str.c_str()); // Аргументи char*
      }
#if DEBUG
      Serial.print("Current: "); Serial.print(publish_str); Serial.println("A");
#endif
    } else {
      if (mqttClient.connected()) {
        mqttClient.publish(mqtt_client_energymon_L1_current_topic.c_str(), "NAN");
      }
#if DEBUG
      Serial.println("Error reading current");
#endif
    }

    if (!isnan(power)) {
      publish_str = String(power);
      if (mqttClient.connected()) {
        mqttClient.publish(mqtt_client_energymon_L1_power_topic.c_str(), publish_str.c_str()); // Аргументи char*
      }
#if DEBUG
      Serial.print("Power: "); Serial.print(publish_str); Serial.println("W");
#endif
    } else {
      if (mqttClient.connected()) {
        mqttClient.publish(mqtt_client_energymon_L1_power_topic.c_str(), "NAN");
      }
#if DEBUG
      Serial.println("Error reading power");
#endif
    }

    if (!isnan(energy)) {
      publish_str = String(energy, 3);
      if (mqttClient.connected()) {
        mqttClient.publish(mqtt_client_energymon_L1_energy_topic.c_str(), publish_str.c_str()); // Аргументи char*
      }
#if DEBUG
      Serial.print("Energy: "); Serial.print(publish_str); Serial.println("kWh");
#endif
    } else {
      if (mqttClient.connected()) {
        mqttClient.publish(mqtt_client_energymon_L1_energy_topic.c_str(), "NAN");
      }
#if DEBUG
      Serial.println("Error reading energy");
#endif
    }

    if (!isnan(frequency)) {
      publish_str = String(frequency, 1);
      if (mqttClient.connected()) {
        mqttClient.publish(mqtt_client_energymon_L1_frequency_topic.c_str(), publish_str.c_str()); // Аргументи char*
      }
#if DEBUG
      Serial.print("Frequency: "); Serial.print(publish_str); Serial.println("Hz");
#endif
    } else {
      if (mqttClient.connected()) {
        mqttClient.publish(mqtt_client_energymon_L1_frequency_topic.c_str(), "NAN");
      }
#if DEBUG
      Serial.println("Error reading frequency");
#endif
    }

    if (!isnan(pf)) {
      publish_str= String(pf);
      if (mqttClient.connected()) {
        mqttClient.publish(mqtt_client_energymon_L1_pf_topic.c_str(), publish_str.c_str()); // Аргументи char*
      }
#if DEBUG
      Serial.print("PF: "); Serial.println(publish_str);
#endif
    } else {
      if (mqttClient.connected()) {
        mqttClient.publish(mqtt_client_energymon_L1_pf_topic.c_str(), "NAN");
      }
#if DEBUG
      Serial.println("Error reading power factor");
#endif
    }
    last_energymon_time = millis();
  }
}
#endif

void mqttReconnect(void) {
  if ((millis() - last_mqtt_reconnect_time) >= mqtt_delay) {
#if DEBUG
    Serial.print("Attempting MQTT connection...");
#endif
#if LCD_PRESENT
    lcd.clear();      // очистка дисплею
    lcdBacklight(ON); // Вмикаємо підсвітку на певний час
    lcd.print("Attempting MQTT"); // Выводим текст
    lcd.setCursor(0, 1); // Встановлюємо курсор в початок 2 рядка
    lcd.print("connection...");
#endif
    if (mqttClient.connect(mqtt_client_name.c_str(), mqtt_username, mqtt_password)) {
#if DEBUG
      Serial.println("connected");
#endif
#if LCD_PRESENT
      lcd.clear();      // очистка дисплею
      lcdBacklight(ON); // Вмикаємо підсвітку на певний час
      lcd.print("MQTT Client"); // Выводим текст
      lcd.setCursor(0, 1); // Встановлюємо курсор в початок 2 рядка
      lcd.print("connected");
#endif
      mqttClient.subscribe(mqtt_client_subscribe.c_str()); // Підписуємось на топік(и)
      for (i = 0; i < sizeof(output_pins); i++) {
        if (mqttClient.connected()) {
          mqttClient.publish((mqtt_client_relay_topic + String(i)).c_str(), output_pins_states[i] ? "1" : "0"); // Аргументи char*
        }
      }
    } else {
#if DEBUG
      Serial.print("failed, rc = ");
      Serial.print(mqttClient.state());
      // Possible values for client.state()
      //MQTT_CONNECTION_TIMEOUT     -4
      //MQTT_CONNECTION_LOST        -3
      //MQTT_CONNECT_FAILED         -2
      //MQTT_DISCONNECTED           -1
      //MQTT_CONNECTED               0
      //MQTT_CONNECT_BAD_PROTOCOL    1
      //MQTT_CONNECT_BAD_CLIENT_ID   2
      //MQTT_CONNECT_UNAVAILABLE     3
      //MQTT_CONNECT_BAD_CREDENTIALS 4
      //MQTT_CONNECT_UNAUTHORIZED    5
      Serial.println(", try again after " + String(mqtt_delay) + " milliseconds");
#endif
      // Wait mqtt_delay before retrying
      last_mqtt_reconnect_time = millis();
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
#if DEBUG
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
#endif
  payload[length] = '\0'; // Додаємо кінець - без нього не працює
  String str_topic = String(topic);
  String str_payload = String((char*)payload);

  if (str_topic.substring(0, 22) == mqtt_client_relay_topic) {
    if (str_payload == "1") { // Якщо команда ввімкнути - вмикаємо
      if (!output_pins_states[str_topic.substring(22).toInt()]) { // Вмикаємо тільки якщо дійсно було вимкнуто
        output_pins_states[str_topic.substring(22).toInt()] = ON;
        digitalWrite(output_pins[str_topic.substring(22).toInt()], output_on_state ? ON : OFF);
#if LCD_PRESENT
        lcd.clear();      // очистка дисплею
        lcdBacklight(ON); // Вмикаємо підсвітку на певний час
        lcd.print("Relays id: " + String(str_topic.substring(22).toInt())); // Виводимо текст
        lcd.setCursor(0, 1); // Встановлюємо курсор в початок 2 рядка
        lcd.print("status: ON");
#endif
      }
    }
    if (str_payload == "0") { // Якщо команда вимкнути - вимикаємо
      if (output_pins_states[str_topic.substring(22).toInt()]) { // Вимикаємо тільки якщо дійсно було ввімкнено
        output_pins_states[str_topic.substring(22).toInt()] = OFF;
        digitalWrite(output_pins[str_topic.substring(22).toInt()], output_on_state ? OFF : ON);
#if LCD_PRESENT
        lcd.clear();      // очистка дисплею
        lcdBacklight(ON); // Вмикаємо підсвітку на певний час
        lcd.print("Relays id: " + String(str_topic.substring(22).toInt())); // Виводимо текст
        lcd.setCursor(0, 1); // Встановлюємо курсор в початок 2 рядка
        lcd.print("status: OFF");
#endif
      }
    }
  }
}

/* Розділ ініціалізації програми */
void setup() {
#if DEBUG
  Serial.begin(9600);
#endif

  // Цикл для ініціалізації контактів, input_pins, як входів, масивів: last_debounce_time, buttons_states, last_buttons_states
  for (i = 0; i < sizeof(input_pins); i++) {
    pinMode(input_pins[i], INPUT);
    last_debounce_time[i] = 0; // Ініціалізуємо масив unsigned long last_debounce_time, default 0
  buttons_states[i] = 0; // Set flag (1 - button is pressed); buttons_states[i] default 0
    input_on_state ? last_buttons_states[i] = 0 : last_buttons_states[i] = 1; // Ініціалізуємо масив last_buttons_states[i], default 1 for input_on_state 0 | default 0 for input_on_state 1
  }

  // Цикл для ініціалізації контактів, output_pins, як виходів, зі встановленням станів по замовчуванню
  for (i = 0; i < sizeof(output_pins); i++) {
    pinMode(output_pins[i], OUTPUT);
    digitalWrite(output_pins[i], output_on_state ? output_pins_states[i] : !output_pins_states[i]);
  }

#if LCD_PRESENT
  lcd.init();       // Ініціалізуємо LCD
#endif

  mqttClient.setServer(server, 1883); // Ініціалізуємо MQTT клієнт
  mqttClient.setCallback(callback);   // Ініціалізуємо MQTT клієнт

  Ethernet.begin(mac, ip);
  //Allow the Ethernet hardware to sort itself out
  //delay(1500);
  if (mqttClient.connect(mqtt_client_name.c_str(), mqtt_username, mqtt_password)) {
    mqttClient.subscribe(mqtt_client_subscribe.c_str()); // Підписуємося на топіки
#if LCD_PRESENT
    lcd.clear();      // очистка дисплею
    lcdBacklight(ON); // Вмикаємо підсвітку на певний час
    lcd.print("MQTT Client"); // Выводим текст
    lcd.setCursor(0, 1); // Встановлюємо курсор в початок 2 рядка
    lcd.print("connected");
#endif
  }

  for (i = 0; i < sizeof(output_pins); i++) {
    if (mqttClient.connected()) {
      mqttClient.publish((mqtt_client_relay_topic + String(i)).c_str(), output_pins_states[i] ? "1" : "0"); // Аргументи char*
    }
  }

#if PIR_PRESENT
  // Цикл для ініціалізації контактів pir_input_pins
  for (i = 0; i < sizeof(pir_input_pins); i++) {
    pinMode(pir_input_pins[i], INPUT);
  pirs_states[i] = 0; // Flag sensor triggered, default 0
    last_pirs_time[i] = 0; // Ініціалізуємо масив unsigned long last_pirs_time, по замовчуванню нулями
    if (mqttClient.connected()) {
      mqttClient.publish((mqtt_client_pir_topic + String(i)).c_str(), "0"); // Аргументи char*
    }
  }
#endif

#if DHT_PRESENT
  dht1.begin();
  dht2.begin();
#endif
  
#if FR
  for (i = 0; i < sizeof(fr_input_pins); i++) {
        last_frs_time[i] = 0; // Ініціалізуємо масив unsigned long last_pirs_time, по замовчуванню нулями
  }//End for
#endif

#if LM
  for (i = 0; i < sizeof(lm_input_pins); i++) {
        last_lms_time[i] = 0; // Ініціалізуємо масив unsigned long last_pirs_time, по замовчуванню нулями
  }//End for
#endif

}

/* The main cycle of the program */
void loop() {
  checkButtonsStates(); // Перевіряємо статуси кнопок

  if (!mqttClient.connected()) {
    mqttReconnect();
  } else { // Client connected
    mqttClient.loop();
  }

#if LCD_PRESENT
  lcdBacklight();
#endif

#if DHT_PRESENT
  dhtProcess();
#endif

#if PIR_PRESENT
  pirProcess();
#endif

#if ENERGYMON_PRESENT
  energymonProcess();
#endif

#if FR_PRESENT
  frProcess();
#endif

#if LM_PRESENT
  lmProcess();
#endif
}