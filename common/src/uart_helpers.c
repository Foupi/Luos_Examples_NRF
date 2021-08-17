#include "uart_helpers.h"

// C STANDARD
#include <stdbool.h>            // bool
#include <stdint.h>             // uint32_t
#include <string.h>             // memset

// NRF
#include "boards.h"             /* RX_PIN_NUMBER, TX_PIN_NUMBER,
                                ** APP_UART_FLOW_CONTROL_DISABLED
                                */
#include "nrf_uart.h"           // NRF_UART_BAUDRATE_115200

// NRF APPS
#include "app_error.h"          // APP_ERROR_CHECK
#include "app_uart.h"           // app_uart_*, APP_UART_FIFO_INIT
#include "app_util_platform.h"  // APP_IRQ_PRIORITY_LOWEST

void uart_init(const app_uart_event_handler_t handler)
{
    app_uart_comm_params_t params;
    memset(&params, 0, sizeof(app_uart_comm_params_t));

    params.rx_pin_no    = RX_PIN_NUMBER;
    params.tx_pin_no    = TX_PIN_NUMBER;
    params.rts_pin_no   = RTS_PIN_NUMBER;
    params.cts_pin_no   = CTS_PIN_NUMBER;
    params.flow_control = APP_UART_FLOW_CONTROL_DISABLED;
    params.use_parity   = false;
    params.baud_rate    = NRF_UART_BAUDRATE_115200;

    uint32_t err_code;
    APP_UART_FIFO_INIT(
        &params,
        RX_FIFO_SIZE,
        TX_FIFO_SIZE,
        handler,
        APP_IRQ_PRIORITY_LOWEST,
        err_code
    );
    APP_ERROR_CHECK(err_code);
}

// Sends the given amount of bytes on the UART.
int _write(int fd, char* str, int len)
{
    for (int index = 0; index < len; index++)
    {
        uint32_t err_code;
        do
        {
            err_code = app_uart_put(str[index]);
        } while (err_code != NRF_SUCCESS);
    }

    return len;
}

// Reads bytes from the UART until there is no more.
int _read(int fd, char* str, int len)
{
    int index = 0;

    uint32_t err_code;
    do
    {
        err_code = app_uart_get((uint8_t*)(str + index));
        if (err_code == NRF_SUCCESS)
        {
            index++;
        }
    } while (err_code == NRF_SUCCESS && index < len);

    return index;
}
