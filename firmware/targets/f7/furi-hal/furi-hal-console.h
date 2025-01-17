#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void furi_hal_console_init();

void furi_hal_console_tx(const uint8_t* buffer, size_t buffer_size);

/**
 * Printf-like plain uart interface
 * @warning Will not work in ISR context
 * @param format 
 * @param ... 
 */
void furi_hal_console_printf(const char format[], ...);

void furi_hal_console_puts(const char* data);

#ifdef __cplusplus
}
#endif
