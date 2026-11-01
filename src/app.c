#include "app.h"
#include "config.h"
#include "gpio.h"
#include "pins.h"
#include "uart.h"
#include "millis.h"
#include "spi.h"
#include "buzzer.h"
#include "servo.h"
#include "mfrc522.h"
#include "74hc595.h"
#include "keypad.h"
#include "twi.h"
#include "led.h"
#include "access_protocol.h"
#include <stdint.h>

#define PIN_LENGTH             4U
#define PIN_TIMEOUT_MS        10000UL
#define BACKEND_TIMEOUT_MS     3000UL
#define DOOR_OPEN_MS          5000UL
#define FEEDBACK_MS            100UL
#define BLINK_MS               250UL
#define ERROR_DISPLAY_MS      2000UL

typedef enum
{
    STATE_IDLE,
    STATE_WAIT_BACKEND_UID,
    STATE_WAIT_PIN,
    STATE_WAIT_BACKEND_PIN,
    STATE_ACCESS_GRANTED,
    STATE_ACCESS_DENIED,
    STATE_BACKEND_TIMEOUT
} app_state_t;

static app_state_t state = STATE_IDLE;
static millis_t state_started;
static millis_t blink_started;
static millis_t feedback_started;
static uint8_t blink_visible;
static uint8_t feedback_active;
static uint16_t session_id;
static char pin[PIN_LENGTH + 1];
static uint8_t pin_count;

static void set_state(app_state_t next)
{
    state = next;
    state_started = millis_get();
    blink_started = state_started;
    blink_visible = 1;
    feedback_active = 0;

    switch (next)
    {
        case STATE_IDLE:
            pin_count = 0;
            pin[0] = '\0';
            session_id = 0;
            buzzer_quiet();
            servo_close();
            red_led_on();
            break;
        case STATE_WAIT_PIN:
            yellow_led_on();
            break;
        case STATE_ACCESS_GRANTED:
            servo_open();
            buzzer_scream();
            green_led_on();
            break;
        case STATE_ACCESS_DENIED:
            red_led_on();
            break;
        case STATE_BACKEND_TIMEOUT:
            blue_led_on();
            break;
        default:
            break;
    }
}

static uint8_t elapsed(millis_t start, millis_t duration)
{
    return (millis_t)(millis_get() - start) >= duration;
}

static void blink_update(void)
{
    if (!elapsed(blink_started, BLINK_MS)) return;
    blink_started = millis_get();
    blink_visible = !blink_visible;

    if (!blink_visible)
    {
        rgb_off();
    } else if (state == STATE_WAIT_PIN)
    {
        yellow_led_on();
    } else if (state == STATE_ACCESS_DENIED)
    {
        red_led_on();
    } else if (state == STATE_BACKEND_TIMEOUT)
    {
        blue_led_on();
    }
}

static void uid_to_hex(const mfrc522_uid_t *uid, char *out)
{
    static const char hex[] = "0123456789ABCDEF";
    for (uint8_t i = 0; i < uid->size; ++i)
    {
        out[2U * i] = hex[(uid->uid[i] >> 4) & 0x0F];
        out[2U * i + 1U] = hex[uid->uid[i] & 0x0F];
    }
    out[2U * uid->size] = '\0';
}

void app_init(void)
{
    gpio_pin_output(&LED_DDR, LED_PIN);
    gpio_pin_low(&LED_PORT, LED_PIN);
    millis_init();
    uart_init(UART_BAUDRATE);
    spi_init();
    servo_init();
    buzzer_init();
    mfrc522_init();
    shift_register_init();
    keypad_init();
    twi_init(100000);
    init_led();
    access_init();
    set_state(STATE_IDLE);
}

void app_run(void)
{
    access_message_t msg;
    access_status_t status = access_read_message(&msg);

    /* Protokollet delar UART med backend: skriv inte debugtext här. */
    if (status == ACCESS_STATUS_OK)
    {
        if (msg.command == ACCESS_CMD_PING)
        {
            access_send_pong();
        } else if (state == STATE_WAIT_BACKEND_UID)
        {
            if (msg.command == ACCESS_CMD_REQ_PIN && msg.sid != 0)
            {
                session_id = msg.sid;
                set_state(STATE_WAIT_PIN);
            } else if (msg.command == ACCESS_CMD_ERR ||
                       msg.command == ACCESS_CMD_LOCKED ||
                       msg.command == ACCESS_CMD_NACK)
                       {
                set_state(STATE_ACCESS_DENIED);
            }
        } else if (state == STATE_WAIT_BACKEND_PIN && msg.sid == session_id)
        {
            if (msg.command == ACCESS_CMD_OK)
            {
                set_state(STATE_ACCESS_GRANTED);
            } else if (msg.command == ACCESS_CMD_ERR ||
                       msg.command == ACCESS_CMD_LOCKED ||
                       msg.command == ACCESS_CMD_NACK)
                       {
                set_state(STATE_ACCESS_DENIED);
            } else if (msg.command == ACCESS_CMD_TIMEOUT)
            {
                set_state(STATE_BACKEND_TIMEOUT);
            }
        }
    }

    switch (state)
    {
        case STATE_IDLE:
        {
            uint8_t atqa[2];
            uint8_t atqa_len = sizeof(atqa);
            mfrc522_uid_t uid;
            if (mfrc522_request_a(atqa, &atqa_len) == MFRC522_OK &&
                mfrc522_anticoll_select(&uid) == MFRC522_OK &&
                uid.size > 0 && uid.size <= sizeof(uid.uid))
                {
                char uid_hex[sizeof(uid.uid) * 2U + 1U];
                uid_to_hex(&uid, uid_hex);
                mfrc522_halt();
                if (access_send_uid(uid_hex) == ACCESS_STATUS_OK)
                {
                    set_state(STATE_WAIT_BACKEND_UID);
                } else
                {
                    set_state(STATE_BACKEND_TIMEOUT);
                }
            }
            break;
        }

        case STATE_WAIT_BACKEND_UID:
            if (elapsed(state_started, BACKEND_TIMEOUT_MS))
            {
                set_state(STATE_BACKEND_TIMEOUT);
            }
            break;

        case STATE_WAIT_PIN:
        {
            if (elapsed(state_started, PIN_TIMEOUT_MS))
            {
                access_send_reset(session_id);
                set_state(STATE_IDLE);
                break;
            }

            if (feedback_active && elapsed(feedback_started, FEEDBACK_MS))
            {
                feedback_active = 0;
                blink_started = millis_get();
                blink_visible = 1;
                yellow_led_on();
            }

            /* Den befintliga drivrutinen debouncar och väntar på släpp. */
            char key = keypad_get_key_debounced();
            if (key != 0)
            {
                feedback_active = 1;
                feedback_started = millis_get();
                green_led_on();

                if (key == '*')
                {
                    pin_count = 0;
                    pin[0] = '\0';
                } else if (key >= '0' && key <= '9' && pin_count < PIN_LENGTH)
                {
                    pin[pin_count++] = key;
                    pin[pin_count] = '\0';
                    if (pin_count == PIN_LENGTH)
                    {
                        if (access_send_pin(session_id, pin) == ACCESS_STATUS_OK)
                        {
                            set_state(STATE_WAIT_BACKEND_PIN);
                        } else
                        {
                            set_state(STATE_BACKEND_TIMEOUT);
                        }
                        break;
                    }
                }
            }
            if (!feedback_active) blink_update();
            break;
        }

        case STATE_WAIT_BACKEND_PIN:
            if (elapsed(state_started, BACKEND_TIMEOUT_MS))
            {
                set_state(STATE_BACKEND_TIMEOUT);
            }
            break;

        case STATE_ACCESS_GRANTED:
            if (elapsed(state_started, DOOR_OPEN_MS))
            {
                set_state(STATE_IDLE);
            }
            break;

        case STATE_ACCESS_DENIED:
        case STATE_BACKEND_TIMEOUT:
            if (elapsed(state_started, ERROR_DISPLAY_MS))
            {
                set_state(STATE_IDLE);
            } else
            {
                blink_update();
            }
            break;
    }
}
