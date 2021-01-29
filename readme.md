<h1>Actuator for smart home, via MQTT over Ethernet protocols</h1>
<h1>Виконувальний пристрій для розумного дому</h1>
На Arduino Mega 2560 + Ethernet shield w5100 по протоколу MQTT

Виконувальний пристрій являє собою групу рельє якими можна керувати по протоколу MQTT,
також можна підключити кнопки та керувати рельє напряму через Arduino, при цьому Arduino
буде відправляти на брокер стан рельє ("1", "0").

Також опціонально можна підключити LCD дсплей по протоколу I2C, датчики температури/вологості DHT (11, 22), датчикм руху/диму/геркони, датчики струму/напруги (PZEM004T v3.0) - Arduino буде відправляти температуру, вологість, події руху/диму, напругу/струм на брокер :)

<b>#define DEBUG 1</b> // Constant for debugging on the serial port<br />
<b>#define ON 1</b>    // Constant for the on state<br />
<b>#define OFF 0</b>   // Constant for the off state<br />
<b>#define LCD_PRESENT 1</b> // The presence of an LCD display<br />
<b>#define DHT_PRESENT 1</b> // Availability of DHT temperature / humidity sensors<br />
<b>#define ENERGYMON_PRESENT 1</b> // The presence of current and voltage sensors<br />
<b>#define PIR_PRESENT 1</b> // Motion sensors - PIR (Passive Infra-Red) SR501 or a smoke sensor connected via TTL/CMOS converter<br />
<b>#define FR_PRESENT 1</b> // Light sensors, analog photoresistor 5мм GL5528, connected to Arduino analog inputs<br />
<b>#define LM_PRESENT 1</b> // Temperature sensors, analog sensors, such as LM35, connected to Arduino analog inputs<br />

Якщо LCD дсплей задіяно - буде відображатися стан рельє, а також буде задіяна "розумна" підсвітка

Для відлагодження по послідовному порту передбачений режим дебагу

<b>#define DEBUG 1</b> // Константа для відлагодження по послідовному порту

Контакти Arduino які будуть використовуватись як входи описуються в масиві

<b>const byte input_pins[]</b>

Контакти Arduino які будуть використовуватись як виходи. описуються в масиві

<b>const byte output_pins[]</b>

може бути більше ніж кількість входів. Ті що виходять за межі кнопок - перемикатися конопками не будуть

<b>Yurii Radio 2017 - 2021</b>