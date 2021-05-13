#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define BLINK_GPIO          2
#define BLINK_INTERVAL      200

#define GPIO_OUTPUT_PIN_SEL       ((1ULL<<BLINK_GPIO))

static void
blink_task(void* unused)
{
  while(1)
  {
    gpio_set_level(BLINK_GPIO, 0);
    vTaskDelay(BLINK_INTERVAL / portTICK_RATE_MS);
    gpio_set_level(BLINK_GPIO, 1);
    vTaskDelay(BLINK_INTERVAL / portTICK_RATE_MS);
  }
}

void
app_blink_init(void)
{
  gpio_config_t io_conf;

  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
  io_conf.pull_down_en = 0;
  io_conf.pull_up_en = 0;
  gpio_config(&io_conf);

  xTaskCreate(blink_task, "display", 256, NULL, 3, NULL);
}
