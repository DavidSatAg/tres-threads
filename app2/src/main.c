#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <inttypes.h>

/* Definições para o tempo do timer */
#define TIMER_INTERVAL_MS 5000

/* Configuração do botão */
#define SW0_NODE DT_ALIAS(sw0)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});
static struct gpio_callback button_cb_data;

/* Flags para rastrear as condições */
int mutex1 = -1;
int mutex2 = -1;

/* Define os Mutexes e os inicia */
K_MUTEX_DEFINE(my_mutex1);
K_MUTEX_DEFINE(my_mutex2);
K_MUTEX_DEFINE(control_mutex);  // Mutex adicional para proteger a função de verificação

/* Função Mutexes */
void mutexTravado() {
    /* Trava o mutex de controle */
    if (k_mutex_lock(&control_mutex, K_FOREVER) == 0) {
        if (mutex1 == 0 && mutex2 == 0) {
            printk("Ambos Travados\n");
        } else {
            printk("NAO ESTAO ambos travados\n");
        }
        /* Destrava o mutex de controle */
        k_mutex_unlock(&control_mutex);
    } else {
        printk("Erro ao acessar o mutex de controle.\n");
    }
}

/* Callback do timer 1 */
void timer_expiry_callback1(struct k_timer *dummy) {
    /* Trava o Mutex 1 */
    mutex1 = k_mutex_lock(&my_mutex1, K_FOREVER);
    printk("Mutex 1 Travado\n");
    mutexTravado();
}

/* Callback do timer 2 */
void timer_expiry_callback2(struct k_timer *dummy) {
    /* Destrava o Mutex 1 */
    k_mutex_unlock(&my_mutex1);
    printk("Mutex 1 Destravado\n");
}

/* Callback do botão */
void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    if (gpio_pin_get(dev, button.pin)) {
        /* Trava o Mutex 2 */
        mutex2 = k_mutex_lock(&my_mutex2, K_FOREVER);
        printk("Mutex 2 Travado\n");
    } else {
        /* Destrava o Mutex 2 */
        k_mutex_unlock(&my_mutex2);
        printk("Mutex 2 Destravado\n");
    }
}

/* Timers para simular eventos */
K_TIMER_DEFINE(my_timer1, timer_expiry_callback1, NULL);
K_TIMER_DEFINE(my_timer2, timer_expiry_callback2, NULL);

/* Thread para inicializar os timers */
void timer_thread(void *arg1, void *arg2, void *arg3) {
    /* Inicia os timers */
    k_timer_start(&my_timer1, K_MSEC(TIMER_INTERVAL_MS), K_MSEC(TIMER_INTERVAL_MS));
    k_timer_start(&my_timer2, K_MSEC(TIMER_INTERVAL_MS), K_MSEC(TIMER_INTERVAL_MS));

    printk("Timers iniciados!\n");

    while (1) {
        k_msleep(1000); // Mantém a thread viva
    }
}

/* Thread para gerenciar o botão */
void button_thread(void *arg1, void *arg2, void *arg3) {
    int ret;

    /* Configuração do botão */
    if (!gpio_is_ready_dt(&button)) {
        printk("Erro: Botão não está pronto\n");
        return;
    }

    ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
    if (ret != 0) {
        printk("Erro ao configurar o botão\n");
        return;
    }

    ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_BOTH);
    if (ret != 0) {
        printk("Erro ao configurar a interrupção do botão\n");
        return;
    }

    gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data);

    printk("Botão configurado e callback ativo.\n");

    while (1) {
        k_msleep(1000); // Mantém a thread viva
    }
}

/* Define as threads com K_THREAD_DEFINE */
K_THREAD_DEFINE(timer_thread_id, 1024, timer_thread, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(button_thread_id, 1024, button_thread, NULL, NULL, NULL, 5, 0, 0);

/* Função principal */
int main(void) {
    printk("Sistema inicializado com threads, callbacks e controle adicional de mutex!\n");

    while (1) {
        k_msleep(1000); // Reduz o uso da CPU enquanto espera por eventos
    }

    return 0;
}
