#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX_CMD_LEN 64
#define MAX_VARS 10
#define MAX_VAR_NAME_LEN 16
#define MAX_VAR_VAL_LEN 32

typedef struct {
    const char* name;
    uint pin;
    bool state;
} Led;

Led leds[] = {
    {"red", 0, false},
    {"green", 1, false},
    {"blue", 2, false}
};

typedef struct {
    char name[MAX_VAR_NAME_LEN];
    char value[MAX_VAR_VAL_LEN];
} Variable;

Variable variables[MAX_VARS];
int var_count = 0;

void led_on(Led* led) {
    printf("%s LED stato prima: %s\n", led->name, led->state ? "ON" : "OFF");
    led->state = true;
    gpio_put(led->pin, 1);
    printf("%s LED stato dopo: ON\n", led->name);
}

void led_off(Led* led) {
    printf("%s LED stato prima: %s\n", led->name, led->state ? "ON" : "OFF");
    led->state = false;
    gpio_put(led->pin, 0);
    printf("%s LED stato dopo: OFF\n", led->name);
}

void led_toggle(Led* led) {
    printf("%s LED stato prima: %s\n", led->name, led->state ? "ON" : "OFF");
    led->state = !led->state;
    gpio_put(led->pin, led->state ? 1 : 0);
    printf("%s LED stato dopo: %s\n", led->name, led->state ? "ON" : "OFF");
}

Led* find_led_by_name(const char* name) {
    for (int i = 0; i < sizeof(leds)/sizeof(leds[0]); ++i) {
        if (strcmp(leds[i].name, name) == 0)
            return &leds[i];
    }
    return NULL;
}

const char* get_variable_value(const char* name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            return variables[i].value;
        }
    }
    return NULL;
}

void set_variable(const char* name, const char* value) {
    // Cerca esistente
    for (int i = 0; i < var_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            strncpy(variables[i].value, value, MAX_VAR_VAL_LEN-1);
            variables[i].value[MAX_VAR_VAL_LEN-1] = 0;
            printf("Variabile aggiornata: %s = %s\n", name, value);
            return;
        }
    }
    // Nuova variabile
    if (var_count < MAX_VARS) {
        strncpy(variables[var_count].name, name, MAX_VAR_NAME_LEN-1);
        variables[var_count].name[MAX_VAR_NAME_LEN-1] = 0;
        strncpy(variables[var_count].value, value, MAX_VAR_VAL_LEN-1);
        variables[var_count].value[MAX_VAR_VAL_LEN-1] = 0;
        var_count++;
        printf("Variabile aggiunta: %s = %s\n", name, value);
    } else {
        printf("Errore: troppe variabili.\n");
    }
}

// Funzione per sostituire %var con valore variabile in input
void expand_variables(const char* input, char* output, int out_len) {
    int in_i = 0, out_i = 0;
    while (input[in_i] && out_i < out_len -1) {
        if (input[in_i] == '%' && isalpha(input[in_i+1])) {
            // Inizio nome variabile
            char varname[MAX_VAR_NAME_LEN] = {0};
            int vn_i = 0;
            in_i++;
            while (isalnum(input[in_i]) && vn_i < MAX_VAR_NAME_LEN -1) {
                varname[vn_i++] = input[in_i++];
            }
            varname[vn_i] = 0;
            const char* val = get_variable_value(varname);
            if (val) {
                // Copia valore variabile
                for (int k = 0; val[k] && out_i < out_len -1; k++) {
                    output[out_i++] = val[k];
                }
            } else {
                // Variabile non trovata, copia nome con %
                output[out_i++] = '%';
                for (int k = 0; varname[k] && out_i < out_len -1; k++) {
                    output[out_i++] = varname[k];
                }
            }
        } else {
            output[out_i++] = input[in_i++];
        }
    }
    output[out_i] = 0;
}

int main() {
    stdio_init_all();

    // Init GPIO LEDs
    for (int i = 0; i < sizeof(leds)/sizeof(leds[0]); i++) {
        gpio_init(leds[i].pin);
        gpio_set_dir(leds[i].pin, GPIO_OUT);
        gpio_put(leds[i].pin, 0);
    }

    char line[MAX_CMD_LEN];

    while (true) {
        printf("\nEnter command:\n");
        if (!fgets(line, sizeof(line), stdin)) continue;

        // Rimuovi newline
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') line[len-1] = 0;

        // Tokenizzazione semplice
        char* argv[5];
        int argc = 0;
        char* token = strtok(line, " ");
        while (token && argc < 5) {
            argv[argc++] = token;
            token = strtok(NULL, " ");
        }
        if (argc == 0) continue;

        if (strcmp(argv[0], "led") == 0 && argc == 3) {
            Led* led = find_led_by_name(argv[2]);
            if (!led) {
                printf("LED '%s' not found\n", argv[2]);
                continue;
            }
            if (strcmp(argv[1], "on") == 0) led_on(led);
            else if (strcmp(argv[1], "off") == 0) led_off(led);
            else if (strcmp(argv[1], "toggle") == 0) led_toggle(led);
            else printf("Unknown LED action '%s'\n", argv[1]);
        } else if (strcmp(argv[0], "echo") == 0 && argc >= 2) {
            // Unisci argv da 1 in poi in stringa temporanea
            char temp[64] = {0};
            int pos = 0;
            for (int i=1; i<argc; i++) {
                if (pos + strlen(argv[i]) + 1 >= sizeof(temp)) break;
                if (i > 1) temp[pos++] = ' ';
                strcpy(&temp[pos], argv[i]);
                pos += strlen(argv[i]);
            }
            // Espandi variabili
            char expanded[128];
            expand_variables(temp, expanded, sizeof(expanded));
            printf("%s\n", expanded);
        } else if (strcmp(argv[0], "set") == 0 && argc == 3) {
            set_variable(argv[1], argv[2]);
        } else if (strcmp(argv[0], "sleep") == 0 && argc == 3) {
            int amount = atoi(argv[2]);
            if (amount < 0) amount = 0;
            if (strcmp(argv[1], "ms") == 0) {
                sleep_ms(amount);
            } else if (strcmp(argv[1], "s") == 0) {
                sleep_ms(amount * 1000);
            } else {
                printf("Unit not recognized. Use 'ms' or 's'.\n");
            }
        } else {
            printf("Commands:\n");
            printf("  led <on|off|toggle> <red|green|blue>\n");
            printf("  echo <text with optional %%var>\n");
            printf("  set <name> <value>\n");
            printf("  sleep <ms|s> <amount>\n");
        }
    }

    return 0;
}