APP_DIR=./app

APP_C_SOURCES = \
$(APP_DIR)/app.c \
$(APP_DIR)/sys_timer.c \
$(APP_DIR)/circ_buffer.c \
$(APP_DIR)/event_dispatcher.c \
$(APP_DIR)/mainloop_timer.c \
$(APP_DIR)/soft_timer.c \
$(APP_DIR)/misc.c \
$(APP_DIR)/shell.c \
$(APP_DIR)/shell_if_usb.c \
$(APP_DIR)/gpio_app.c \
$(APP_DIR)/pwm.c \
$(APP_DIR)/soft_pwm.c \
$(APP_DIR)/adc_u.c \
$(APP_DIR)/adc_reader.c \
$(APP_DIR)/ssd1306/ssd1306.c \
$(APP_DIR)/ssd1306/ssd1306_fonts.c \
$(APP_DIR)/heater/fan.c \
$(APP_DIR)/heater/oil_pump.c \
$(APP_DIR)/heater/glow_plug.c \
$(APP_DIR)/heater/settings.c \
$(APP_DIR)/heater/ntc50.c \
$(APP_DIR)/heater/heater.c \
$(APP_DIR)/heater/display.c


C_SOURCES += $(APP_C_SOURCES)
CFLAGS += -I$(APP_DIR) -I$(APP_DIR)/ssd1306 -I$(APP_DIR)/heater
