#include <ETH.h>
#include <WiFiUdp.h>
#include "DFRobot_ESP32CAN.h"

// =====================================================
// EDGE101 CAN <-> ETHERNET BRIDGE
// =====================================================
//
// Telemetry out to Orin:
//   UDP 20000
//
// Commands in from Orin:
//   UDP 20001
//
// Example command:
//   {"t":"cmd","id":21,"mode":"current","value":1.0}
//
// Output examples:
//   HB rx=12345 tx=1
//   V1 id=21 erpm=0 cur=0.0 duty=0.000 raw=0000000000000000
//   TX 00000115#000003E8
//   DBG ack id=21 mode=current value=1.000 ok=1
//
// =====================================================


// =====================================================
// NETWORK CONFIG - UPDATED TO 192.168.50.x
// =====================================================

// Orin Ethernet IP
IPAddress orinIP(192, 168, 50, 2);

// Static IP for this Edge101
IPAddress localIP(192, 168, 50, 68);

// Gateway is not very important for direct local traffic,
// but keep it in the same subnet.
IPAddress gateway(192, 168, 50, 1);

IPAddress subnet(255, 255, 255, 0);

// Edge101 -> Orin telemetry/debug
const uint16_t telemetryPort = 20000;

// Orin -> Edge101 commands
const uint16_t commandPort = 20001;

WiFiUDP udpTx;
WiFiUDP udpRx;

bool eth_connected = false;


// =====================================================
// CAN CONFIG
// =====================================================

DFRobot_ESP32CAN CANBus;

can_message_t rxMsg;
can_message_t txMsg;


// =====================================================
// STATE
// =====================================================

unsigned long lastHeartbeat = 0;
unsigned long canFramesSeen = 0;
unsigned long canFramesSent = 0;

// Set true if you want every raw CAN frame printed.
// Leave false for usable output.
bool printRawRxFrames = false;


// =====================================================
// VESC CAN COMMAND IDS
// =====================================================
//
// VESC extended CAN ID format:
//   ID = (command << 8) | controller_id
//
// Examples:
//   motor 21 status1: command 9 -> 0x915
//   motor 21 current: command 1 -> 0x115
//   motor 21 rpm:     command 3 -> 0x315
//
// =====================================================

static const uint8_t VESC_CAN_SET_DUTY          = 0;
static const uint8_t VESC_CAN_SET_CURRENT       = 1;
static const uint8_t VESC_CAN_SET_CURRENT_BRAKE = 2;
static const uint8_t VESC_CAN_SET_RPM           = 3;

static const uint8_t VESC_CAN_STATUS_1          = 9;


// =====================================================
// ETHERNET EVENT HANDLER
// =====================================================

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      ETH.setHostname("edge101-can");
      break;

    case ARDUINO_EVENT_ETH_GOT_IP:
      eth_connected = true;
      break;

    case ARDUINO_EVENT_ETH_DISCONNECTED:
    case ARDUINO_EVENT_ETH_STOP:
      eth_connected = false;
      break;

    default:
      break;
  }
}


// =====================================================
// HEX HELPERS
// =====================================================

String toHexN(uint32_t value, int width) {
  String s = String(value, HEX);
  s.toUpperCase();
  while ((int)s.length() < width) s = "0" + s;
  return s;
}

String payloadHex(const can_message_t& m) {
  String out = "";
  for (int i = 0; i < m.data_length_code; i++) {
    out += toHexN(m.data[i], 2);
  }
  return out;
}


// =====================================================
// UDP OUTPUT
// =====================================================

void sendUDPLine(const String& s) {
  if (!eth_connected) return;

  udpTx.beginPacket(orinIP, telemetryPort);
  udpTx.print(s);
  udpTx.print('\n');
  udpTx.endPacket();
}

void sendHeartbeat() {
  String s = "HB rx=" + String(canFramesSeen) + " tx=" + String(canFramesSent);
  sendUDPLine(s);
}

void sendDebug(const String& msg) {
  sendUDPLine("DBG " + msg);
}

void sendRxFrame(const can_message_t& m) {
  String s = "RX " + toHexN(m.identifier, 8) + "#" + payloadHex(m);
  sendUDPLine(s);
}

void sendTxFrame(const can_message_t& m) {
  String s = "TX " + toHexN(m.identifier, 8) + "#" + payloadHex(m);
  sendUDPLine(s);
}


// =====================================================
// VESC STATUS DECODER
// =====================================================

void decodeAndSendVescStatus1(const can_message_t& m) {
  uint32_t id = m.identifier;

  uint8_t controller_id = id & 0xFF;
  uint8_t command = (id >> 8) & 0xFF;

  if (command != VESC_CAN_STATUS_1) return;
  if (m.data_length_code != 8) return;

  int32_t erpm_raw =
    ((int32_t)m.data[0] << 24) |
    ((int32_t)m.data[1] << 16) |
    ((int32_t)m.data[2] << 8)  |
    ((int32_t)m.data[3]);

  int16_t curr_raw =
    ((int16_t)m.data[4] << 8) |
    ((int16_t)m.data[5]);

  int16_t duty_raw =
    ((int16_t)m.data[6] << 8) |
    m.data[7];

  float current = curr_raw / 10.0f;
  float duty = duty_raw / 1000.0f;

  String s = "V1 id=";
  s += String(controller_id);
  s += " erpm=";
  s += String(erpm_raw);
  s += " cur=";
  s += String(current, 1);
  s += " duty=";
  s += String(duty, 3);
  s += " raw=";
  s += payloadHex(m);

  sendUDPLine(s);
}


// =====================================================
// CAN TX HELPERS
// =====================================================

bool sendVescCommandRaw(uint8_t controller_id, uint8_t command, int32_t value) {
  txMsg.identifier = ((uint32_t)command << 8) | controller_id;
  txMsg.flags = CAN_MSG_FLAG_EXTD;
  txMsg.data_length_code = 4;

  txMsg.data[0] = (value >> 24) & 0xFF;
  txMsg.data[1] = (value >> 16) & 0xFF;
  txMsg.data[2] = (value >> 8) & 0xFF;
  txMsg.data[3] = value & 0xFF;

  bool ok = CANBus.transmit(&txMsg, pdMS_TO_TICKS(10));

  if (ok) {
    canFramesSent++;
    sendTxFrame(txMsg);
  } else {
    sendDebug("tx_fail id=" + String(controller_id) + " cmd=" + String(command));
  }

  return ok;
}

bool sendVescSetCurrent(uint8_t controller_id, float amps) {
  int32_t value = (int32_t)(amps * 1000.0f);   // amps -> milliamps
  return sendVescCommandRaw(controller_id, VESC_CAN_SET_CURRENT, value);
}

bool sendVescSetBrake(uint8_t controller_id, float amps) {
  int32_t value = (int32_t)(amps * 1000.0f);   // amps -> milliamps
  return sendVescCommandRaw(controller_id, VESC_CAN_SET_CURRENT_BRAKE, value);
}

bool sendVescSetRPM(uint8_t controller_id, int32_t rpm) {
  return sendVescCommandRaw(controller_id, VESC_CAN_SET_RPM, rpm);
}

bool sendVescSetDuty(uint8_t controller_id, float duty) {
  int32_t value = (int32_t)(duty * 100000.0f);
  return sendVescCommandRaw(controller_id, VESC_CAN_SET_DUTY, value);
}


// =====================================================
// SIMPLE COMMAND PARSERS
// =====================================================
//
// Expected UDP command:
//   {"t":"cmd","id":21,"mode":"current","value":1.0}
//
// No full JSON parser. Just simple extraction.
//
// =====================================================

bool extractUInt8(const String& s, const String& key, uint8_t& out) {
  int k = s.indexOf("\"" + key + "\"");
  if (k < 0) return false;

  int colon = s.indexOf(':', k);
  if (colon < 0) return false;

  int i = colon + 1;
  while (i < (int)s.length() && s[i] == ' ') i++;

  String num = "";
  while (i < (int)s.length() && isDigit(s[i])) {
    num += s[i++];
  }

  if (num.length() == 0) return false;
  out = (uint8_t)num.toInt();
  return true;
}

bool extractFloat(const String& s, const String& key, float& out) {
  int k = s.indexOf("\"" + key + "\"");
  if (k < 0) return false;

  int colon = s.indexOf(':', k);
  if (colon < 0) return false;

  int i = colon + 1;
  while (i < (int)s.length() && s[i] == ' ') i++;

  String num = "";
  if (i < (int)s.length() && (s[i] == '-' || s[i] == '+')) {
    num += s[i++];
  }

  while (i < (int)s.length() && (isDigit(s[i]) || s[i] == '.')) {
    num += s[i++];
  }

  if (num.length() == 0) return false;
  out = num.toFloat();
  return true;
}

bool extractString(const String& s, const String& key, String& out) {
  int k = s.indexOf("\"" + key + "\"");
  if (k < 0) return false;

  int colon = s.indexOf(':', k);
  if (colon < 0) return false;

  int q1 = s.indexOf('"', colon + 1);
  if (q1 < 0) return false;

  int q2 = s.indexOf('"', q1 + 1);
  if (q2 < 0) return false;

  out = s.substring(q1 + 1, q2);
  return true;
}


// =====================================================
// COMMAND HANDLER
// =====================================================

void handleCommand(const String& cmd) {
  if (cmd.indexOf("\"t\":\"cmd\"") < 0) {
    sendDebug("packet_missing_t_cmd");
    return;
  }

  uint8_t id = 0;
  float value = 0.0f;
  String mode = "";

  if (!extractUInt8(cmd, "id", id)) {
    sendDebug("parse_fail_id");
    return;
  }

  if (!extractString(cmd, "mode", mode)) {
    sendDebug("parse_fail_mode");
    return;
  }

  if (!extractFloat(cmd, "value", value)) {
    sendDebug("parse_fail_value");
    return;
  }

  bool ok = false;

  if (mode == "current") {
    ok = sendVescSetCurrent(id, value);
  } else if (mode == "brake") {
    ok = sendVescSetBrake(id, value);
  } else if (mode == "rpm") {
    ok = sendVescSetRPM(id, (int32_t)value);
  } else if (mode == "duty") {
    ok = sendVescSetDuty(id, value);
  } else {
    sendDebug("unknown_mode_" + mode);
    return;
  }

  sendDebug("ack id=" + String(id) +
            " mode=" + mode +
            " value=" + String(value, 3) +
            " ok=" + String(ok ? 1 : 0));
}


// =====================================================
// UDP COMMAND RECEIVE
// =====================================================

void pollUDPCommands() {
  int packetSize = udpRx.parsePacket();
  if (packetSize <= 0) return;

  String cmd = "";
  while (udpRx.available()) {
    cmd += (char)udpRx.read();
  }

  cmd.trim();

  sendDebug("rx_" + cmd);

  if (cmd.length() > 0) {
    handleCommand(cmd);
  }
}


// =====================================================
// SETUP
// =====================================================

void setup() {
  Serial.begin(115200);
  delay(300);

  WiFi.onEvent(WiFiEvent);

  // Start Ethernet first
  ETH.begin();

  // Give driver a moment
  delay(100);

  // THEN apply static IP config
  ETH.config(localIP, gateway, subnet);

  unsigned long timeout = millis();
  while (!eth_connected && millis() - timeout < 10000) {
    delay(100);
  }

  // Listen for incoming commands from Orin
  udpRx.begin(commandPort);

  // CAN init
  can_general_config_t g_config = CAN_GENERAL_CONFIG(CAN_MODE_NORMAL);
  can_timing_config_t  t_config = CAN_TIMING_CONFIG_500KBITS();
  can_filter_config_t  f_config = CAN_FILTER_CONFIG_ACCEPT_ALL();

  while (!CANBus.init(&g_config, &t_config, &f_config)) {
    delay(500);
  }

  while (!CANBus.start()) {
    delay(500);
  }

  sendDebug("boot_ok ip=192.168.50.60 orin=192.168.50.2");
}


// =====================================================
// MAIN LOOP
// =====================================================

void loop() {
  if (millis() - lastHeartbeat >= 1000) {
    lastHeartbeat = millis();
    sendHeartbeat();
  }

  // Receive commands from Orin
  pollUDPCommands();

  // Receive CAN frames from VESC bus
  if (CANBus.receive(&rxMsg, pdMS_TO_TICKS(5))) {
    canFramesSeen++;

    bool isExt = (rxMsg.flags & CAN_MSG_FLAG_EXTD) != 0;

    if (printRawRxFrames) {
      sendRxFrame(rxMsg);
    }

    if (isExt && rxMsg.data_length_code == 8) {
      decodeAndSendVescStatus1(rxMsg);
    }
  }

  delay(1);
}
