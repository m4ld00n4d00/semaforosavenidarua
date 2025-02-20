#include <stdbool.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

// Definição dos pinos
#define A_R      2
#define A_Y      4
#define A_G      5
#define R_R      18
#define R_Y      19
#define R_G      21
#define AV_L_R   25
#define AV_L_Y   15  // Novo pino para o amarelo da conversão
#define AV_L_G   26
#define P_R      12
#define P_G      23
#define P2_R     27  
#define P2_G     32
#define BTN_RU   34  // Botão para pedestres na rua
#define BTN_AV   35  // Botão para pedestres na avenida
#define BZ_AV    14  
#define BZ_RU    33  

// Enumeração de estados
typedef enum {
    STATE_AV_GREEN,
    STATE_AV_YELLOW,
    STATE_RU_GREEN,
    STATE_RU_YELLOW,
    STATE_PED_GREEN
} traffic_state_t;

// Variáveis globais
static traffic_state_t state = STATE_AV_GREEN;
static bool pedRequest = false;
static TickType_t lastButtonPress = 0;

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

void init_gpio(void) {
    gpio_config_t outputs = {
        .pin_bit_mask = ((1ULL << A_R) | (1ULL << A_Y) | (1ULL << A_G) |
                        (1ULL << R_R) | (1ULL << R_Y) | (1ULL << R_G) |
                        (1ULL << AV_L_R) | (1ULL << AV_L_Y) | (1ULL << AV_L_G) |
                        (1ULL << P_R) | (1ULL << P_G) |
                        (1ULL << P2_R) | (1ULL << P2_G) |
                        (1ULL << BZ_AV) | (1ULL << BZ_RU)),
        .mode = GPIO_MODE_OUTPUT
    };
    gpio_config(&outputs);

    gpio_config_t btn = {
        .pin_bit_mask = (1ULL << BTN_RU) | (1ULL << BTN_AV),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    gpio_config(&btn);
}

void set_av(const char *color) {
    gpio_set_level(A_R, strcmp(color, "red") == 0);
    gpio_set_level(A_Y, strcmp(color, "yellow") == 0);
    gpio_set_level(A_G, strcmp(color, "green") == 0);
}

void set_rua(const char *color) {
    gpio_set_level(R_R, strcmp(color, "red") == 0);
    gpio_set_level(R_Y, strcmp(color, "yellow") == 0);
    gpio_set_level(R_G, strcmp(color, "green") == 0);
}

void set_ped(const char *color) {
    gpio_set_level(P_R, strcmp(color, "red") == 0);
    gpio_set_level(P_G, strcmp(color, "green") == 0);
}

void set_ped2(const char *color) {
    gpio_set_level(P2_R, strcmp(color, "red") == 0);
    gpio_set_level(P2_G, strcmp(color, "green") == 0);
}

void set_av_left(const char *color) {
    gpio_set_level(AV_L_R, strcmp(color, "red") == 0);
    gpio_set_level(AV_L_Y, strcmp(color, "yellow") == 0);
    gpio_set_level(AV_L_G, strcmp(color, "green") == 0);
}

void all_red(void) {
    set_av("red");
    set_rua("red");
    set_ped("red");
    set_ped2("red");
    set_av_left("red");
}

void buzzer_beep(void) {
    for (int i = 0; i < 5; i++) {
        gpio_set_level(BZ_AV, 1);
        gpio_set_level(BZ_RU, 1);
        vTaskDelay(pdMS_TO_TICKS(1000));
        gpio_set_level(BZ_AV, 0);
        gpio_set_level(BZ_RU, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void check_button(void) {
    if (gpio_get_level(BTN_RU) == 0 || gpio_get_level(BTN_AV) == 0) {
        if ((xTaskGetTickCount() - lastButtonPress) > pdMS_TO_TICKS(2000)) {
            pedRequest = true;
            lastButtonPress = xTaskGetTickCount();
        }
    }
}

void traffic_control(void *pvParameter) {
    init_gpio();

    while (1) {
        check_button();
        
        switch (state) {
            case STATE_AV_GREEN:
                all_red();
                set_av("green");
                set_av_left("green");
                set_ped("red");
                set_ped2("red");
                
                while (!pedRequest) {
                    check_button();
                    vTaskDelay(pdMS_TO_TICKS(500));
                }
                pedRequest = false;
                vTaskDelay(pdMS_TO_TICKS(4000));
                state = STATE_AV_YELLOW;
                break;

            case STATE_AV_YELLOW:
                set_av("yellow");
                set_av_left("yellow");
                vTaskDelay(pdMS_TO_TICKS(1000));
                state = STATE_RU_GREEN;
                break;

            case STATE_RU_GREEN:
                all_red();
                set_rua("green");
                vTaskDelay(pdMS_TO_TICKS(4000));
                state = STATE_RU_YELLOW;
                break;

            case STATE_RU_YELLOW:
                set_rua("yellow");
                vTaskDelay(pdMS_TO_TICKS(1000));
                state = STATE_PED_GREEN;
                break;

            case STATE_PED_GREEN:
                all_red();
                set_ped("green");
                set_ped2("green");
                buzzer_beep();
                
                for (int i = 0; i < 3; i++) {
                    set_ped("red");
                    set_ped2("red");
                    vTaskDelay(pdMS_TO_TICKS(500));
                    set_ped("green");
                    set_ped2("green");
                    vTaskDelay(pdMS_TO_TICKS(500));
                }
                
                state = STATE_AV_GREEN;
                break;
        }
    }
}

void app_main(void) {
    xTaskCreate(traffic_control, "traffic_ctrl", 4096, NULL, 5, NULL);
}

// by chatgpt, deepseek, Samuel e grupo
