#include <stdbool.h>

#include "symbol_table.h"

const size_t symbol_table__size __attribute__((section(".symbol_table_size"), weak));
const symbol_table__symbol_s *symbol_table__base __attribute__((section(".symbol_table_base"), weak));

const symbol_table__symbol_s *symbol_table__lookup_symbol(const sl_string_s symbol_name) {
  const symbol_table__symbol_s *symbol = NULL;
  const char *name = sl_string__c_str(symbol_name);
  if (NULL != name) {
    bool symbol_found = false;
    size_t index = 0;
    const symbol_table__symbol_s *symbol_ptr = symbol_table__base;

    for (index = 0; index < symbol_table__size; index++, symbol_ptr++) {
      symbol_found = sl_string__equals_to(symbol_name, symbol_ptr->name);
      if (symbol_found) {
        symbol = symbol_ptr;
        break;
      }
    }
  }
  return symbol;
}

size_t symbol_table__get_symbol_data(const symbol_table__symbol_s *symbol, sl_string_s output) {
  size_t string_size = 0U;
  if (NULL != symbol) {
    const size_t BITS_IN_BYTE = 8U;

    size_t bits_in_n_bytes = BITS_IN_BYTE * symbol->size;
    size_t mask = ~0U;
    size_t bit_offset = 0U;

    if (0U != symbol->bit_size) {
      mask = (1 << symbol->bit_size) - 1;
    }

    if (0U != symbol->bit_offset || 0U != symbol->bit_size) {
      bit_offset = (bits_in_n_bytes - symbol->bit_size - symbol->bit_offset);
    }

    switch (symbol->data_type) {
    case data_type_uintptr_t:
      // do nothing
      break;
    case data_type_bool:
      string_size = sl_string__printf(output, "%d\n", ((*(bool *)symbol->address)));
      break;
    case data_type_char:
      string_size = sl_string__printf(output, "%c\n", ((*(char *)symbol->address)));
      break;
    case data_type_float:
      // type cast to double to suppress float to double promotion warning
      string_size = sl_string__printf(output, "%f\n", (double)(*(float *)symbol->address));
      break;
    case data_type_double:
      string_size = sl_string__printf(output, "%f\n", (*(double *)symbol->address));
      break;
    case data_type_int8_t:
      string_size =
          sl_string__printf(output, "%d\n", ((*(int8_t *)symbol->address) & (mask << bit_offset)) >> bit_offset);
      break;
    case data_type_int16_t:
      string_size =
          sl_string__printf(output, "%d\n", ((*(int16_t *)symbol->address) & (mask << bit_offset)) >> bit_offset);
      break;
    case data_type_int32_t:
      string_size =
          sl_string__printf(output, "%ld\n", ((*(int32_t *)symbol->address) & (mask << bit_offset)) >> bit_offset);
      break;
    case data_type_int64_t:
      // type cast to int32_t because newlib-nano does not support 64-bit integers, truncate to 32-bits
      string_size = sl_string__printf(output, "%ld\n",
                                      ((int32_t)(*(int64_t *)symbol->address) & (mask << bit_offset)) >> bit_offset);
      break;
    case data_type_uint8_t:
      string_size =
          sl_string__printf(output, "%u\n", ((*(uint8_t *)symbol->address) & (mask << bit_offset)) >> bit_offset);
      break;
    case data_type_uint16_t:
      string_size =
          sl_string__printf(output, "%u\n", ((*(uint16_t *)symbol->address) & (mask << bit_offset)) >> bit_offset);
      break;
    case data_type_uint32_t:
      string_size =
          sl_string__printf(output, "%lu\n", ((*(uint32_t *)symbol->address) & (mask << bit_offset)) >> bit_offset);
      break;
    case data_type_uint64_t:
      // type cast to uint32_t because newlib-nano does not support 64-bit integers, truncate to 32-bits
      string_size = sl_string__printf(output, "%lu\n",
                                      ((uint32_t)(*(uint64_t *)symbol->address) & (mask << bit_offset)) >> bit_offset);
      break;
    default:
      // Do nothing
      break;
    }
  }
  return string_size;
}
