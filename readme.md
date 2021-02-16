<h1>Actuator for smart home, via MQTT over Ethernet protocols</h1>
<h1>Виконувальний пристрій для розумного дому</h1>
<b>На Arduino Mega 2560 + Ethernet shield w5100 по протоколу MQTT</b> 

The actuator is a group of relays that can be controlled by the MQTT protocol, 
you can also connect buttons and control the relay directly via the Arduino, and the status will be sent to the broker ("1", "0"). 
<br /><i>Виконувальний пристрій являє собою групу реле якими можна керувати по протоколу MQTT,
також можна підключити кнопки та керувати реле напряму через Arduino, при цьому буде відправлятися статус на брокер ("1", "0").</i>

<b>You can also optionally connect:</b><br />
<b>#define DEBUG 1</b> // Constant for debugging on the serial port<br />
<b>#define LCD_PRESENT 1</b> // The presence of an LCD display<br />
<b>#define DHT_PRESENT 1</b> // Availability of DHT temperature / humidity sensors<br />
<b>#define ENERGYMON_PRESENT 1</b> // The presence of current and voltage sensors<br />
<b>#define PIR_PRESENT 1</b> // Motion sensors - PIR (Passive Infra-Red) HC-SR501(L - mode) or a smoke sensor connected via TTL/CMOS converter<br />
<b>#define FR_PRESENT 1</b> // Light sensors, analog photoresistor 5мм GL5528, connected to Arduino analog inputs<br />
<b>#define LM_PRESENT 1</b> // Temperature sensors, analog sensors, such as LM35, connected to Arduino analog inputs<br />

<b>Також опціонально можна підключити:</b><br />
<i>LCD дсплей по протоколу I2C - Z-LCD I2C модуль;</i><br />
<i>датчики температури/вологості DHT11, DHT22;</i><br />
<i>датчики руху/диму/геркони;</i><br />
<i>електролічильник (струм, напруга, потудність, частота, коефіцієт потужності - power factor, лічильник) - PZEM004T v3.0;</i><br />
<i>звичайні (5мм GL5528 )фоторезистори, як датчики освітлення;</i><br />
<i>аналогові датчики температури, такі як LM35;</i><br />
<i>Arduino буде відправляти значення датчиків на брокер :)</i>

If the LCD display is activated, the status of the relay will be displayed and the "smart" backlight will be activated. 
<br /><i>Якщо LCD дисплей задіяно - буде відображатися стан реле, а також буде задіяна "розумна" підсвітка.</i>

For debugging, on the serial port, the debug mode is provided, after full adjustment to translate in DEBUG 0 
<br /><i>Для відлагодження, по послідовному порту, передбачений режим дебагу, після повного налаштування перевести в DEBUG 0</i>
<br /><b>#define DEBUG 1</b> 

The Arduino contacts that will be used as inputs (to which we connect the buttons) are described in the array 
<br /><i>Контакти Arduino які будуть використовуватись як входи (до яких підключимо кнопки) описуються в масиві</i>
<br /><b>const byte input_pins[]</b>

The Arduino contacts that will be used as outputs (which control the relay) are described in the array 
<br /><i>Контакти Arduino які будуть використовуватись як виходи (які керують реле), описуються в масиві</i>
<br /><b>const byte output_pins[]</b><br />
there may be more than the number of inputs. Those that go beyond the buttons - will not switch buttons 
<br /><i>їх може бути більше ніж кількість входів. Ті що виходять за межі кнопок - перемикатися конопками не будуть</i>

The <b>MajorDoMo classes extensions.txt</b> file describes extensions to the standard subclasses, the SDevices system class, to connect the executable to the MajorDoMo smart home system.
<br /><i>В файлі <b>MajorDoMo classes extensions.txt</b> описані розширення стандартних підкласів, системного класу SDevices, щоб підключити виконувальний пристрій до системи розумного дому MajorDoMo.</i>

<b>Yurii Radio 2017 - 2021</b>