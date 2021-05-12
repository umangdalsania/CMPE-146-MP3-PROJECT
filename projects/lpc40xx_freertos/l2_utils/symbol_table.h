#include <stdlib.h>

#include "sl_string.h"

typedef enum {
  data_type_uintptr_t = 0,
  data_type_bool,
  data_type_char,
  data_type_float,
  data_type_double,
  data_type_int8_t,
  data_type_int16_t,
  data_type_int32_t,
  data_type_int64_t,
  data_type_uint8_t,
  data_type_uint16_t,
  data_type_uint32_t,
  data_type_uint64_t,
  symbol_table__data_type_size,
} symbol_table__data_type_e;

typedef struct {
  const char *name;
  const void *address;
  symbol_table__data_type_e data_type;
  size_t size;
  size_t bit_size;
  size_t bit_offset;
} symbol_table__symbol_s;

const symbol_table__symbol_s *symbol_table__lookup_symbol(const sl_string_s symbol_name);
size_t symbol_table__get_symbol_data(const symbol_table__symbol_s *symbol, sl_string_s output);

extern const size_t symbol_table__size;
extern const symbol_table__symbol_s *symbol_table__base;
