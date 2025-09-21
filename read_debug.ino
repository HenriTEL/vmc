#include <Wire.h>

#include "HumidityStore.h"

// Serial console speed
#define SERIAL_BAUDS              115200


HumidityStore rh_store;
char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void loop() {
  static bool sent = false;
  if (sent) return;  // only dump once per boot

  uint8_t rh_buffer[1500];
  uint16_t key_index = 0;
  int32_t bytes_read = 1;

  Serial.println("Printing base 64 encoded humidity values");
  while (bytes_read > 0) {
    bytes_read = rh_store.read(key_index, rh_buffer);
    if (bytes_read) {
	  Serial.printf("Key %d:\n", key_index);
      print_block_b64(rh_buffer, bytes_read);
      key_index++;
    }
  }

  sent = true; // prevent repeating
}

void print_block_b64(uint8_t rh_buffer[], int32_t size) {
    if (size <= 0) {
        return;
    }
    
    
    for (int32_t i = 0; i < size; i += 3) {
        uint32_t triple = 0;
        int chars_to_process = (size - i < 3) ? size - i : 3;
        
        /* Pack 3 bytes into a 24-bit integer */
        for (int j = 0; j < chars_to_process; j++) {
            triple |= (rh_buffer[i + j] << (8 * (2 - j)));
        }
        
        /* Extract 4 base64 characters (6 bits each) */
        for (int j = 0; j < 4; j++) {
            if (i * 4 / 3 + j < ((size + 2) / 3) * 4) {
                if (j < chars_to_process + 1) {
                    int index = (triple >> (6 * (3 - j))) & 0x3F;
                    Serial.printf("%c", base64_chars[index]);
                } else {
                    Serial.printf("="); /* Padding */
                }
            }
        }
    }
    Serial.printf("\n");
}

void setup() {
  Serial.begin(SERIAL_BAUDS);
  delay(1000); // Allow USB-serial setup
  storage_setup();
}

void storage_setup() {
  if (!rh_store.begin()) {
      Serial.println("Halting due to NVS init failure for HumidityStore.");
      while (1) delay(1000);
  }
}
