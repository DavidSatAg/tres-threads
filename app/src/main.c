#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define LED1_NODE DT_ALIAS(led1)
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

#define LED2_NODE DT_ALIAS(led2)
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);

#define BUTTON_NODE DT_ALIAS(sw0)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);

K_MUTEX_DEFINE(timer_mutex);
K_MUTEX_DEFINE(button_mutex);

#define LED0_PIN led0.pin
#define LED1_PIN led1.pin
#define LED2_PIN led2.pin

void blink_led(void)
{
    int ret;

    if (!device_is_ready(led0.port)) {
        printk("Dispositivo GPIO do LED principal não está pronto\n");
        return;
    }

    ret = gpio_pin_configure(led0.port, LED0_PIN, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Falha ao configurar o pino %d\n", LED0_PIN);
        return;
    }

    while (1) {
        k_mutex_lock(&timer_mutex, K_FOREVER);

        gpio_pin_set(led0.port, LED0_PIN, 1);
        printk("LED0 ON\n");
        k_sleep(K_SECONDS(1));

        gpio_pin_set(led0.port, LED0_PIN, 0);
        printk("LED0 OFF\n");

        k_mutex_unlock(&timer_mutex);

        k_sleep(K_SECONDS(4));
    }
}

void button_led(void)
{
    int ret;

    if (!device_is_ready(led1.port) || !device_is_ready(button.port)) {
        printk("Dispositivo GPIO do LED ou botão não está pronto\n");
        return;
    }

    ret = gpio_pin_configure(led1.port, LED1_PIN, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        printk("Falha ao configurar o pino %d\n", LED1_PIN);
        return;
    }

    ret = gpio_pin_configure(button.port, button.pin, GPIO_INPUT | GPIO_PULL_UP);
    if (ret < 0) {
        printk("Falha ao configurar o pino do botão\n");
        return;
    }

    while (1) {
        if (gpio_pin_get(button.port, button.pin) == 0) {
            k_mutex_lock(&button_mutex, K_FOREVER);
            gpio_pin_set(led1.port, LED1_PIN, 1);
            printk("LED1 ON\n");
        } else {
            gpio_pin_set(led1.port, LED1_PIN, 0);
            printk("LED1 OFF\n");
            k_mutex_unlock(&button_mutex);
        }
        k_sleep(K_MSEC(100));
    }
}

void third_led_control(void)
{
    int ret;

    if (!device_is_ready(led2.port)) {
        printk("Dispositivo GPIO do LED3 não está pronto\n");
        return;
    }

    ret = gpio_pin_configure(led2.port, LED2_PIN, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        printk("Falha ao configurar o pino %d\n", LED2_PIN);
        return;
    }

    while (1) {
        if (k_mutex_lock(&timer_mutex, K_NO_WAIT) == 0) {
            k_mutex_unlock(&timer_mutex);
        } else if (k_mutex_lock(&button_mutex, K_NO_WAIT) == 0) {
            k_mutex_unlock(&button_mutex);
        } else {
            gpio_pin_set(led2.port, LED2_PIN, 1);
            printk("LED2 ON - Ambos os mutexes bloqueados\n");
            k_sleep(K_MSEC(100));
            continue;
        }

        gpio_pin_set(led2.port, LED2_PIN, 0);
        printk("LED2 OFF - Condição não satisfeita\n");

        k_sleep(K_MSEC(100));
    }
}

K_THREAD_DEFINE(blink_led_id, 1024, blink_led, NULL, NULL, NULL, 7, 0, 0);
K_THREAD_DEFINE(button_led_id, 1024, button_led, NULL, NULL, NULL, 7, 0, 0);
K_THREAD_DEFINE(third_led_id, 1024, third_led_control, NULL, NULL, NULL, 7, 0, 0);
