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

/* Função Mutexes */
void mutexTravado (){
    if (mutex1 == 0 && mutex2 == 0){
        printk("Ambos Travados\n");
    }
    else{
        printk("NAO ESTAO ambos travados\n");
    }
}

/* Callback do timer 1 e 2 */
void timer_expiry_callback1(struct k_timer *dummy)
{
    /* Trava o Mutex 1 */
    mutex1 = k_mutex_lock(&my_mutex1, K_FOREVER);
    printk("Mutex 1 Travado\n");
    mutexTravado();
}
void timer_expiry_callback2(struct k_timer *dummy)
{
    /* Destrava o Mutex 1 */
    k_mutex_unlock(&my_mutex1);
    printk("Mutex 1 Destravado\n");
}

/* Callback do botão */
void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    if (gpio_pin_get(dev, button.pin)){
    /* Trava o Mutex 2 */
    mutex2 = k_mutex_lock(&my_mutex2, K_FOREVER);
    printk("Mutex 2 Travado\n");
    }
    else{
    /* Destrava o Mutex 2 */
    k_mutex_unlock(&my_mutex2);
    printk("Mutex 2 Destravado\n");
    }
}

/* Define o timer */
K_TIMER_DEFINE(my_timer1, timer_expiry_callback1, NULL);
K_TIMER_DEFINE(my_timer2, timer_expiry_callback2, NULL);

/* Função principal */
int main(void)
{
    int ret;

    /* Configuração do botão */
    if (!gpio_is_ready_dt(&button)) {
        printk("Erro: Botão não está pronto\n");
        return -1;
    }

    ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
    if (ret != 0) {
        printk("Erro ao configurar o botão\n");
        return -1;
    }

    ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_BOTH);
    if (ret != 0) {
        printk("Erro ao configurar a interrupção do botão [borda de subída e descida]\n");
        return -1;
    }

    gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data);

    /* Inicia o timer */
    k_timer_start(&my_timer1, K_MSEC(TIMER_INTERVAL_MS), K_MSEC(TIMER_INTERVAL_MS));
    k_timer_start(&my_timer2, K_MSEC(TIMER_INTERVAL_MS), K_MSEC(TIMER_INTERVAL_MS));

    printk("Sistema inicializado. Timer ativo!\n");

    /* Loop infinito */
    while (1) {
        k_msleep(1000); // Reduz o uso da CPU enquanto espera por eventos
    }

    return 0;
}