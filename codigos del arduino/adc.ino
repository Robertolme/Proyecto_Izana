#include "driver/i2s.h"
#include "driver/adc.h"

#define I2S_SAMPLE_RATE   1000000  // 1MSPS

void setup() {
  Serial.begin(115200);

  adc1_config_width(ADC_WIDTH_BIT_9); // 9 bits = 0-511
  adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);

  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
    .sample_rate = I2S_SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,  // necesario por hardware
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags = 0,
    .dma_buf_count = 4,
    .dma_buf_len = 1024,
    .use_apll = false
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_adc_mode(ADC_UNIT_1, ADC1_CHANNEL_0);
  i2s_adc_enable(I2S_NUM_0);
}

void loop() {
  const int muestra_count = 1000;  // Puedes ajustar según lo que quieras
  uint8_t buffer[muestra_count * 2];  // cada muestra = 2 bytes
  size_t bytes_read;

  i2s_read(I2S_NUM_0, buffer, sizeof(buffer), &bytes_read, portMAX_DELAY);

  for (int i = 0; i < bytes_read; i += 2) {
    // Combinar dos bytes en un entero de 16 bits
    uint16_t muestra = buffer[i] | (buffer[i + 1] << 8);
    muestra &= 0x01FF;  // Solo 9 bits útiles
    Serial.print("signal:");
    Serial.println(muestra);
  }

  delay(100);  // Espera opcional para evitar saturar el buffer serial
}
