/*
  xsns_88_rinai.ino - Rinai Boiler sensor support for Tasmota

  Copyright (C) 2021  Theo Arends

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef USE_RINAI
/*********************************************************************************************\
 * DHT11, AM2301 (DHT21, DHT22, AM2302, AM2321), SI7021 - Temperature and Humidity
 *
 * Reading temperature or humidity takes about 250 milliseconds!
 * Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
 *
 * This version is based on ESPEasy _P005_DHT.ino 20191201
\*********************************************************************************************/

#define XSNS_88          88


//#define DHT_MAX_SENSORS  4
//#define DHT_MAX_RETRY    8

//uint8_t dht_data[5];
//uint8_t dht_sensors = 0;
//uint8_t dht_pin_out = 0;                      // Shelly GPIO00 output only
bool rinai_active = true;                       // DHT configured
//bool dht_dual_mode = false;                   // Single pin mode
uint8 rinai_temp = 0;

/*
struct DHTSTRUCT {
  int8_t   pin;
  uint8_t  type;
  uint8_t  lastresult;
  char     stype[12];
  float    t = NAN;
  float    h = NAN;
} Dht[DHT_MAX_SENSORS];
*/

bool RinaiWaitState(uint32_t sensor, uint32_t level)
{
/*
  unsigned long timeout = micros() + 100;
  while (digitalRead(Dht[sensor].pin) != level) {
    if (TimeReachedUsec(timeout)) {
      AddLog(LOG_LEVEL_DEBUG, PSTR(D_LOG_DHT D_TIMEOUT_WAITING_FOR " %s " D_PULSE),
        (level) ? D_START_SIGNAL_HIGH : D_START_SIGNAL_LOW);
      return false;
    }
    delayMicroseconds(1);
  }
*/
  return true;
}

//
// internal function to publish device information with respect to all `SetOption`s
//
void jsonPublishTemperature (){
  TasmotaGlobal.mqtt_data[0] = 0; // clear string
  ResponseTime_P(PSTR(""));
  //Response_P(PSTR("{\"%s\":"), "Temperature=41");

#ifdef xxx
 // What key do we use, shortaddr or name?
  if (!Settings.flag5.zb_omit_json_addr) {
    if (use_fname) {
      Response_P(PSTR("%s{\"%s\":"), TasmotaGlobal.mqtt_data, friendlyName);
    } else {
      Response_P(PSTR("%s{\"0x%04X\":"), TasmotaGlobal.mqtt_data, shortaddr);
    }
  }
  ResponseAppend_P(PSTR("{"));

  // Add "Device":"0x...."
  ResponseAppend_P(PSTR("\"" D_JSON_ZIGBEE_DEVICE "\":\"0x%04X\","), shortaddr);
  // Add "Name":"xxx" if name is present
  if (friendlyName) {
    ResponseAppend_P(PSTR("\"" D_JSON_ZIGBEE_NAME "\":\"%s\","), EscapeJSONString(friendlyName).c_str());
  }
  // Add all other attributes
  ResponseAppend_P(PSTR("%s}"), attr_list.toString(false).c_str());

  if (!Settings.flag5.zb_omit_json_addr) {
    ResponseAppend_P(PSTR("}"));
  }

  if (!Settings.flag4.remove_zbreceived && !Settings.flag5.zb_received_as_subtopic) {
    ResponseAppend_P(PSTR("}"));
  }

  if (Settings.flag4.zigbee_distinct_topics) {
    char subtopic[TOPSZ];
    if (Settings.flag4.zb_topic_fname && friendlyName && strlen(friendlyName)) {
      // Clean special characters
      char stemp[TOPSZ];
      strlcpy(stemp, friendlyName, sizeof(stemp));
      MakeValidMqtt(0, stemp);
      if (Settings.flag5.zigbee_hide_bridge_topic) {
        snprintf_P(subtopic, sizeof(subtopic), PSTR("%s"), stemp);
      } else {
        snprintf_P(subtopic, sizeof(subtopic), PSTR("%s/%s"), TasmotaGlobal.mqtt_topic, stemp);
      } 
    } else {
      if (Settings.flag5.zigbee_hide_bridge_topic) {
        snprintf_P(subtopic, sizeof(subtopic), PSTR("%04X"), shortaddr);
      } else {
        snprintf_P(subtopic, sizeof(subtopic), PSTR("%s/%04X"), TasmotaGlobal.mqtt_topic, shortaddr);
      } 
    }
    if (Settings.flag5.zb_topic_endpoint) {
      if (attr_list.isValidSrcEp()) {
        snprintf_P(subtopic, sizeof(subtopic), PSTR("%s_%d"), subtopic, attr_list.src_ep);
      }
    }
    char stopic[TOPSZ];
    if (Settings.flag5.zb_received_as_subtopic)
      GetTopic_P(stopic, TELE, subtopic, json_prefix);
    else
      GetTopic_P(stopic, TELE, subtopic, PSTR(D_RSLT_SENSOR));
    MqttPublish(stopic, Settings.flag.mqtt_sensor_retain);
  } else {
    MqttPublishPrefixTopic_P(TELE, PSTR(D_RSLT_SENSOR), Settings.flag.mqtt_sensor_retain);
.  }
#endif
  MqttPublishPrefixTopic_P(TELE, PSTR(D_RSLT_SENSOR), 0);
  XdrvRulesProcess(0);     // apply rules
}

int RinaiDecodDiplay (int v)
{
  switch (v)
  {
    case 102:
      return 0;
    case 32:
      return 1;
    case 101:
      return 2;
    case 97:
      return 3;
    case 35:
      return 4;
    case 67:
      return 5;
    case 71:
      return 6;
    case 98:
      return 7;
    case 103:
      return 8;
    case 99:
      return 9;
    default:
      return -1;
  }
}

bool RinaiRead(uint32_t sensor)
{
  int t = 0;
  int c[2] = {0, 0};
  int d[2] = {-1, -1};
  
  //AddLog(LOG_LEVEL_INFO, "r1=%d r2=%d", digitalRead(4), digitalRead(5));
  //return true;

  for (t = 0; t < 10; t++)
  {
    if (digitalRead (16) == HIGH) 
    {
      c[1] = 0;
      if (d[0] == -1){
        c[0]++;
        if (c[0] == 3){
          d[0] = digitalRead (4); //a
          d[0] = (d[0] << 1) + digitalRead (14); //b
          d[0] = (d[0] << 1); //c
          d[0] = (d[0] << 1); //d
          d[0] = (d[0] << 1) + digitalRead (13); //e
          d[0] = (d[0] << 1) + digitalRead (12); //f
          d[0] = (d[0] << 1) + digitalRead (3);  //g
        }
      }
    }
    else {
      c[0] = 0;
      if (d[1] == -1){
        c[1]++;
        if (c[1] == 3){
          d[1] = digitalRead (4); //a
          d[1] = (d[1] << 1) + digitalRead (14); //b
          d[1] = (d[1] << 1); //c
          d[1] = (d[1] << 1); //d
          d[1] = (d[1] << 1) + digitalRead (13); //e
          d[1] = (d[1] << 1) + digitalRead (12); //f
          d[1] = (d[1] << 1) + digitalRead (3);  //g
        }
      }
    }
    if (d[0] >= 0 && d[1] >= 0){
      rinai_temp = RinaiDecodDiplay(d[0]) * 10 + RinaiDecodDiplay(d[1]);
      AddLog(LOG_LEVEL_INFO, "d=%d-%d->%d", d[0], d[1], rinai_temp);
      return true;
    }  
    rinai_temp = -1;
    delay (1);
  }
  return false;
}

#ifdef xxx    
  dht_data[0] = dht_data[1] = dht_data[2] = dht_data[3] = dht_data[4] = 0;

  if (!dht_dual_mode) {
    pinMode(Dht[sensor].pin, OUTPUT);
    digitalWrite(Dht[sensor].pin, LOW);
  } else {
    digitalWrite(dht_pin_out, LOW);
  }

  switch (Dht[sensor].type) {
    case GPIO_DHT11:                                    // DHT11
      delay(19);  // minimum 18ms
      break;
    case GPIO_DHT22:                                    // DHT21, DHT22, AM2301, AM2302, AM2321
//      delay(2);   // minimum 1ms
      delayMicroseconds(2000);                          // See https://github.com/arendst/Tasmota/pull/7468#issuecomment-647067015
      break;
    case GPIO_SI7021:                                   // iTead SI7021
      delayMicroseconds(500);
      break;
  }

  if (!dht_dual_mode) {
    pinMode(Dht[sensor].pin, INPUT_PULLUP);
  } else {
    digitalWrite(dht_pin_out, HIGH);
  }

  switch (Dht[sensor].type) {
    case GPIO_DHT11:                                    // DHT11
    case GPIO_DHT22:                                    // DHT21, DHT22, AM2301, AM2302, AM2321
      delayMicroseconds(50);
      break;
    case GPIO_SI7021:                                   // iTead SI7021
      delayMicroseconds(20);                            // See: https://github.com/letscontrolit/ESPEasy/issues/1798
      break;
  }

/*
  bool error = false;
  noInterrupts();
  if (DhtWaitState(sensor, 0) && DhtWaitState(sensor, 1) && DhtWaitState(sensor, 0)) {
    for (uint32_t i = 0; i < 5; i++) {
      int data = 0;
      for (uint32_t j = 0; j < 8; j++) {
        if (!DhtWaitState(sensor, 1)) {
          error = true;
          break;
        }
        delayMicroseconds(35);                          // Was 30
        if (digitalRead(Dht[sensor].pin)) {
          data |= (1 << (7 - j));
        }
        if (!DhtWaitState(sensor, 0)) {
          error = true;
          break;
        }
      }
      if (error) { break; }
      dht_data[i] = data;
    }
  } else {
    error = true;
  }
  interrupts();
  if (error) { return false; }
*/

  uint32_t i = 0;
  noInterrupts();
  if (DhtWaitState(sensor, 0) && DhtWaitState(sensor, 1) && DhtWaitState(sensor, 0)) {
    for (i = 0; i < 40; i++) {
      if (!DhtWaitState(sensor, 1)) { break; }
      delayMicroseconds(35);                          // Was 30
      if (digitalRead(Dht[sensor].pin)) {
        dht_data[i / 8] |= (1 << (7 - i % 8));
      }
      if (!DhtWaitState(sensor, 0)) { break; }
    }
  }
  interrupts();
  if (i < 40) { return false; }

  uint8_t checksum = (dht_data[0] + dht_data[1] + dht_data[2] + dht_data[3]) & 0xFF;
  if (dht_data[4] != checksum) {
    char hex_char[15];
    AddLog(LOG_LEVEL_DEBUG, PSTR(D_LOG_DHT D_CHECKSUM_FAILURE " %s =? %02X"),
      ToHex_P(dht_data, 5, hex_char, sizeof(hex_char), ' '), checksum);
    return false;
  }

  float temperature = NAN;
  float humidity = NAN;
  switch (Dht[sensor].type) {
    case GPIO_DHT11:                                    // DHT11
      humidity = dht_data[0];
/*
      // DHT11 no negative temp:
      temperature = dht_data[2] + ((float)dht_data[3] * 0.1f);  // Issue #3164
*/
      // DHT11 (Adafruit):
      temperature = dht_data[2];
      if (dht_data[3] & 0x80) {
        temperature = -1 - temperature;
      }
      temperature += (dht_data[3] & 0x0f) * 0.1f;
/*
      // DHT12 (Adafruit):
      temperature = dht_data[2];
      temperature += (dht_data[3] & 0x0f) * 0.1f;
      if (dht_data[2] & 0x80) {
        temperature *= -1;
      }
*/
      break;
    case GPIO_DHT22:                                    // DHT21, DHT22, AM2301, AM2302, AM2321
    case GPIO_SI7021:                                   // iTead SI7021
      humidity = ((dht_data[0] << 8) | dht_data[1]) * 0.1;
      // DHT21/22 (Adafruit):
      temperature = ((int16_t)(dht_data[2] & 0x7F) << 8 ) | dht_data[3];
      temperature *= 0.1f;
      if (dht_data[2] & 0x80) {
        temperature *= -1;
      }
      break;
  }
  if (isnan(temperature) || isnan(humidity)) {
    AddLog(LOG_LEVEL_DEBUG, PSTR(D_LOG_DHT "Invalid NAN reading"));
    return false;
  }

  if (humidity > 100) { humidity = 100.0; }
  if (humidity < 0) { humidity = 0.1; }
  Dht[sensor].h = ConvertHumidity(humidity);
  Dht[sensor].t = ConvertTemp(temperature);
  Dht[sensor].lastresult = 0;
  return true;
}
#endif

/********************************************************************************************/

bool RinaiPinState()
{
/*
  if ((XdrvMailbox.index >= AGPIO(GPIO_DHT11)) && (XdrvMailbox.index <= AGPIO(GPIO_SI7021))) {
    if (dht_sensors < DHT_MAX_SENSORS) {
      Dht[dht_sensors].pin = XdrvMailbox.payload;
      Dht[dht_sensors].type = BGPIO(XdrvMailbox.index);
      dht_sensors++;
      XdrvMailbox.index = AGPIO(GPIO_DHT11);
    } else {
      XdrvMailbox.index = 0;
    }
    return true;
  }
*/
  return false;
}

void RinaiInit(void)
{
  //pinMode (0, INPUT); //d3
  //pinMode (1, INPUT); //tx
  //pinMode (2, INPUT); //d4
  pinMode (3, INPUT); //rx
  pinMode (4, INPUT); //d2
  pinMode (5, INPUT); //d1
  pinMode (12, INPUT); //d8
  pinMode (13, INPUT); //d7
  pinMode (14, INPUT); //d5
  pinMode (16, INPUT); //d0
  //pinMode (19, INPUT_PULLUP); //d2
  //pinMode (20, INPUT_PULLUP); //d1
/*
  if (dht_sensors) {
    if (PinUsed(GPIO_DHT11_OUT)) {
      dht_pin_out = Pin(GP
      13IO_DHT11_OUT);
      dht_dual_mode = true;    // Dual pins mode as used by Shelly
      dht_sensors = 1;         // We only support one sensor in pseudo mode
      pinMode(dht_pin_out, OUTPUT);
    }

    for (uint32_t i = 0; i < dht_sensors; i++) {
      pinMode(Dht[i].pin, INPUT_PULLUP);
      Dht[i].lastresult = DHT_MAX_RETRY;  // Start with NAN
      GetTextIndexed(Dht[i].stype, sizeof(Dht[i].stype), Dht[i].type, kSensorNames);
      if (dht_sensors > 1) {
        snprintf_P(Dht[i].stype, sizeof(Dht[i].stype), PSTR("%s%c%02d"), Dht[i].stype, IndexSeparator(), Dht[i].pin);
      }
    }
    AddLog(LOG_LEVEL_DEBUG, PSTR(D_LOG_DHT "(v5) " D_SENSORS_FOUND " %d"), dht_sensors);
  } else {
    rinai_active = false;
  }
*/
}

void RinaiEverySecond(void)
{
  AddLog(LOG_LEVEL_INFO, PSTR("RINAI"));
  RinaiRead (1);
  MqttPublishTeleperiodSensor();
  //MqttShowSensor ();
  //jsonPublishTemperature ();
/*
  if (TasmotaGlobal.uptime &1) {  // Every 2 seconds
    for (uint32_t sensor = 0; sensor < dht_sensors; sensor++) {
      // DHT11 and AM2301 25mS per sensor, SI7021 5mS per sensor
      if (!DhtRead(sensor)) {
        Dht[sensor].lastresult++;
        if (Dht[sensor].lastresult > DHT_MAX_RETRY) {  // Reset after 8 misses
          Dht[sensor].t = NAN;
          Dht[sensor].h = NAN;
        }
      }
    }
  }
*/
}

void RinaiShow(bool json){
  //TempHumDewShow(json, (0 == TasmotaGlobal.tele_period), "Rinai", rinai_temp, 50);
  TempHumDewShow(json, true, "Rinai", rinai_temp, 50);
  //MqttPublishTeleperiodSensor();
  //MqttPublishPrefixTopic_P(TELE, PSTR(D_RSLT_SENSOR), 0);
  //XdrvRulesProcess(0);     // apply rules
}

/*********************************************************************************************\
 * Interface
\*********************************************************************************************/

bool Xsns88(uint8_t function)
{
  bool result = false;

  if (rinai_active) {
    switch (function) {
      case FUNC_EVERY_SECOND:
        AddLog(LOG_LEVEL_INFO, PSTR("RINAI 1"));
        RinaiEverySecond();
        break;
      case FUNC_JSON_APPEND:
        AddLog(LOG_LEVEL_INFO, PSTR("RINAI 2"));
        RinaiShow(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_SENSOR:
        AddLog(LOG_LEVEL_INFO, PSTR("RINAI 3"));
        RinaiShow(0);
        break;
#endif  // USE_WEBSERVER
      case FUNC_INIT:
        AddLog(LOG_LEVEL_INFO, PSTR("RINAI 4"));
        RinaiInit();
        break;
      case FUNC_PIN_STATE:
        AddLog(LOG_LEVEL_INFO, PSTR("RINAI 5"));
        result = RinaiPinState();
        break;
    }
  }
  return result;
}

#endif  // USE_RINAI
