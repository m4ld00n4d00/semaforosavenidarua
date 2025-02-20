#include <stdbool.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

// Definição dos pinos
#define A_R      2  // Pino para o semáforo vermelho da avenida
#define A_Y      4  // Pino para o semáforo amarelo da avenida
#define A_G      5  // Pino para o semáforo verde da avenida
#define R_R      18 // Pino para o semáforo vermelho da rua
#define R_Y      19 // Pino para o semáforo amarelo da rua
#define R_G      21 // Pino para o semáforo verde da rua
#define AV_L_R   25 // Pino para o semáforo vermelho da conversão (avenida à esquerda)
#define AV_L_Y   15 // Pino para o semáforo amarelo da conversão
#define AV_L_G   26 // Pino para o semáforo verde da conversão
#define P_R      12 // Pino para o semáforo vermelho para pedestres
#define P_G      23 // Pino para o semáforo verde para pedestres
#define P2_R     27 // Pino para o semáforo vermelho para pedestres 2
#define P2_G     32 // Pino para o semáforo verde para pedestres 2
#define BTN_RU   34 // Pino para o botão de pedestre na rua
#define BTN_AV   35 // Pino para o botão de pedestre na avenida
#define BZ_AV    14 // Pino para o buzzer na avenida
#define BZ_RU    33 // Pino para o buzzer na rua

// Enumeração de estados
typedef enum {
    STATE_AV_GREEN,  // Estado em que a avenida está com semáforo verde
    STATE_AV_YELLOW, // Estado em que a avenida está com semáforo amarelo
    STATE_RU_GREEN,  // Estado em que a rua está com semáforo verde
    STATE_RU_YELLOW, // Estado em que a rua está com semáforo amarelo
    STATE_PED_GREEN  // Estado em que os pedestres têm permissão para atravessar
} traffic_state_t;

// Variáveis globais
static traffic_state_t state = STATE_AV_GREEN; // Estado inicial da máquina de estados
static bool pedRequest = false;  // Flag que indica se há um pedido de pedestres
static TickType_t lastButtonPress = 0;  // Tempo do último pressionamento de botão

// Protótipos de funções
void init_gpio(void);
void set_av(const char *color);
void set_rua(const char *color);
void set_ped(const char *color);
void set_ped2(const char *color);
void set_av_left(const char *color);
void all_red(void);
void buzzer_beep(void);
void check_button(void);
void traffic_control(void *pvParameter);

// Função de inicialização de GPIO
void init_gpio(void) {
    gpio_config_t outputs = {
        .pin_bit_mask = ((1ULL << A_R) | (1ULL << A_Y) | (1ULL << A_G) |  // Semáforo da avenida
                         (1ULL << R_R) | (1ULL << R_Y) | (1ULL << R_G) |  // Semáforo da rua
                         (1ULL << AV_L_R) | (1ULL << AV_L_Y) | (1ULL << AV_L_G) |  // Semáforo da conversão
                         (1ULL << P_R) | (1ULL << P_G) |  // Semáforo de pedestres
                         (1ULL << P2_R) | (1ULL << P2_G) |  // Semáforo de pedestres 2
                         (1ULL << BZ_AV) | (1ULL << BZ_RU)),  // Pinos do buzzer
        .mode = GPIO_MODE_OUTPUT  // Todos os pinos definidos são saídas
    };
    gpio_config(&outputs);

    gpio_config_t btn = {
        .pin_bit_mask = (1ULL << BTN_RU) | (1ULL << BTN_AV),  // Botões de pedestres na rua e na avenida
        .mode = GPIO_MODE_INPUT,  // Botões como entradas
        .pull_up_en = GPIO_PULLUP_ENABLE  // Habilita o resistor pull-up para garantir que o estado inicial seja alto
    };
    gpio_config(&btn);
}

// Função para setar as cores do semáforo da avenida
void set_av(const char *color) {
    gpio_set_level(A_R, strcmp(color, "red") == 0);  // Define o semáforo da avenida como vermelho
    gpio_set_level(A_Y, strcmp(color, "yellow") == 0);  // Define o semáforo da avenida como amarelo
    gpio_set_level(A_G, strcmp(color, "green") == 0);  // Define o semáforo da avenida como verde
}

// Função para setar as cores do semáforo da rua
void set_rua(const char *color) {
    gpio_set_level(R_R, strcmp(color, "red") == 0);  // Define o semáforo da rua como vermelho
    gpio_set_level(R_Y, strcmp(color, "yellow") == 0);  // Define o semáforo da rua como amarelo
    gpio_set_level(R_G, strcmp(color, "green") == 0);  // Define o semáforo da rua como verde
}

// Função para setar as cores do semáforo de pedestres
void set_ped(const char *color) {
    gpio_set_level(P_R, strcmp(color, "red") == 0);  // Define o semáforo de pedestre como vermelho
    gpio_set_level(P_G, strcmp(color, "green") == 0);  // Define o semáforo de pedestre como verde
}

// Função para setar as cores do semáforo de pedestres 2
void set_ped2(const char *color) {
    gpio_set_level(P2_R, strcmp(color, "red") == 0);  // Define o semáforo de pedestre 2 como vermelho
    gpio_set_level(P2_G, strcmp(color, "green") == 0);  // Define o semáforo de pedestre 2 como verde
}

// Função para setar as cores do semáforo de conversão da avenida
void set_av_left(const char *color) {
    gpio_set_level(AV_L_R, strcmp(color, "red") == 0);  // Define o semáforo da conversão como vermelho
    gpio_set_level(AV_L_Y, strcmp(color, "yellow") == 0);  // Define o semáforo da conversão como amarelo
    gpio_set_level(AV_L_G, strcmp(color, "green") == 0);  // Define o semáforo da conversão como verde
}

// Função para colocar todos os semáforos em vermelho
void all_red(void) {
    set_av("red");  // Coloca o semáforo da avenida em vermelho
    set_rua("red");  // Coloca o semáforo da rua em vermelho
    set_ped("red");  // Coloca o semáforo de pedestres em vermelho
    set_ped2("red");  // Coloca o semáforo de pedestres 2 em vermelho
    set_av_left("red");  // Coloca o semáforo de conversão em vermelho
}

// Função para emitir som com o buzzer
void buzzer_beep(void) {
    for (int i = 0; i < 5; i++) {  // Faz o buzzer emitir som 5 vezes
        gpio_set_level(BZ_AV, 1);  // Liga o buzzer da avenida
        gpio_set_level(BZ_RU, 1);  // Liga o buzzer da rua
        vTaskDelay(pdMS_TO_TICKS(1000));  // Espera 1 segundo
        gpio_set_level(BZ_AV, 0);  // Desliga o buzzer da avenida
        gpio_set_level(BZ_RU, 0);  // Desliga o buzzer da rua
        vTaskDelay(pdMS_TO_TICKS(1000));  // Espera 1 segundo
    }
}

// Função para verificar se os botões de pedestre foram pressionados
void check_button(void) {
    if (gpio_get_level(BTN_RU) == 0 || gpio_get_level(BTN_AV) == 0) {  // Se algum botão foi pressionado
        if ((xTaskGetTickCount() - lastButtonPress) > pdMS_TO_TICKS(2000)) {  // Se passaram mais de 2 segundos desde o último pressionamento
            pedRequest = true;  // Há um pedido de pedestres
            lastButtonPress = xTaskGetTickCount();  // Atualiza o tempo do último pressionamento
        }
    }
}

// Função para controlar os semáforos e o fluxo de tráfego
void traffic_control(void *pvParameter) {
    init_gpio();  // Inicializa os pinos GPIO

    while (1) {  // Loop infinito para alternar os semáforos
        check_button();  // Verifica se há algum pedido de pedestre
        
        switch (state) {  // Máquinas de estados
            case STATE_AV_GREEN:  // Quando a avenida está verde
                all_red();  // Coloca todos os semáforos em vermelho
                set_av("green");  // Coloca o semáforo da avenida em verde
                set_av_left("green");  // Coloca o semáforo de conversão em verde
                set_ped("red");  // Coloca o semáforo de pedestres em vermelho
                set_ped2("red");  // Coloca o semáforo de pedestres 2 em vermelho
                
                while (!pedRequest) {  // Espera por um pedido de pedestre
                    check_button();  // Verifica os botões
                    vTaskDelay(pdMS_TO_TICKS(500));  // Aguarda 500 ms
                }
                pedRequest = false;  // Resetar pedido de pedestre
                vTaskDelay(pdMS_TO_TICKS(4000));  // Aguarda 4 segundos antes de mudar para o próximo estado
                state = STATE_AV_YELLOW;  // Muda para o estado de semáforo amarelo da avenida
                break;

            case STATE_AV_YELLOW:  // Quando a avenida está amarela
                set_av("yellow");  // Coloca o semáforo da avenida em amarelo
                set_av_left("yellow");  // Coloca o semáforo de conversão em amarelo
                vTaskDelay(pdMS_TO_TICKS(1000));  // Aguarda 1 segundo
                state = STATE_RU_GREEN;  // Muda para o estado de semáforo verde da rua
                break;

            case STATE_RU_GREEN:  // Quando a rua está verde
                all_red();  // Coloca todos os semáforos em vermelho
                set_rua("green");  // Coloca o semáforo da rua em verde
                vTaskDelay(pdMS_TO_TICKS(4000));  // Aguarda 4 segundos
                state = STATE_RU_YELLOW;  // Muda para o estado de semáforo amarelo da rua
                break;

            case STATE_RU_YELLOW:  // Quando a rua está amarela
                set_rua("yellow");  // Coloca o semáforo da rua em amarelo
                vTaskDelay(pdMS_TO_TICKS(1000));  // Aguarda 1 segundo
                state = STATE_PED_GREEN;  // Muda para o estado de pedestres
                break;

            case STATE_PED_GREEN:  // Quando os pedestres têm permissão para atravessar
                all_red();  // Coloca todos os semáforos em vermelho
                set_ped("green");  // Coloca o semáforo de pedestres em verde
                set_ped2("green");  // Coloca o semáforo de pedestres 2 em verde
                buzzer_beep();  // Emite um som com o buzzer

                for (int i = 0; i < 3; i++) {  // Pisca o semáforo de pedestres 3 vezes
                    set_ped("red");  // Coloca o semáforo de pedestres em vermelho
                    set_ped2("red");  // Coloca o semáforo de pedestres 2 em vermelho
                    vTaskDelay(pdMS_TO_TICKS(500));  // Espera 500 ms
                    set_ped("green");  // Coloca o semáforo de pedestres em verde
                    set_ped2("green");  // Coloca o semáforo de pedestres 2 em verde
                    vTaskDelay(pdMS_TO_TICKS(500));  // Espera 500 ms
                }
                
                state = STATE_AV_GREEN;  // Retorna ao estado de semáforo verde da avenida
                break;
        }
    }
}

// Função principal
void app_main(void) {
    xTaskCreate(traffic_control, "traffic_ctrl", 4096, NULL, 5, NULL);  // Cria a tarefa para controlar o tráfego
}
