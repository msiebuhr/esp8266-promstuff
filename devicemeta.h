#include <OneWire.h>
#include <DallasTemperature.h>
#include <FS.h>

// function to print a device address
void sprintAddress(char buf[], DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++) {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) {
      sprintf(buf + sizeof(char) * i * 2, "0%x", deviceAddress[i]);
    } else {
      sprintf(buf + sizeof(char) * i * 2, "%x", deviceAddress[i]);
    }
  }
}

int get_device_meta(char buf[], int buflen, DeviceAddress device_address) {
  char filename[] = "/DEADBEEFDEADBEEF.txt";
  sprintAddress(filename + 1, device_address); \
  strcpy(filename + 17, ".txt");

  // Zero out buffer
  memset(buf, 0, buflen);

  File f = SPIFFS.open(filename, "r");
  if (!f) {
    return 0;
  }

  // Copy over data
  int len = f.size();
  if (len > buflen) {
    len = buflen;
  }

  // Copy over data
  f.readBytes(buf, len);

  // Trailing newline?
  if (buf[len - 1] == 0x0a) { // LF
    buf[len - 1] = 0;
  }

  return len - 1;
}

bool set_device_meta(char buf[], int buf_len, char address[16]) {
  char filename[] = "/DEADBEEFDEADBEEF.txt";
  strcpy(filename + 1, address);
  strcpy(filename + 17, ".txt");

  File f = SPIFFS.open(filename, "w");
  if (!f) {
    return false;
  }

  f.write((uint8_t*) buf, buf_len);

  // Copy over data
  f.readBytes(buf, buf_len);

  return true;
}

bool set_device_from_address(char buf[], int buf_len, DeviceAddress device_address) {
  char filename[16] = "DEADBEEFDEADBEE";
  sprintAddress(filename, device_address);
  return set_device_meta(buf, buf_len, filename);
}
