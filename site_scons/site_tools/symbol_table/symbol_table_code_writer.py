import json
import sys

from symbol_table import SymbolTableContainer


SYMBOL_TABLE_C_FILE_FORMAT = """\
#include \"symbol_table.h\"

__attribute__((section(".symbol_table"))) static symbol_table__symbol_s symbol_table[{symbol_table_size}U] = {{
{symbol_table_entries}
}};

const size_t symbol_table__size __attribute__((section(".symbol_table_size")))= {symbol_table_size}U;
const symbol_table__symbol_s *symbol_table__base __attribute__((section(".symbol_table_base")))= symbol_table;
"""

STRUCT_FORMAT = '''\
    {
        .name="%s",
        .address=(void*)%s,
        .data_type=%s,
        .size=%s,
        .bit_size=%s,
        .bit_offset=%s
    },
'''


class SymbolTableCodeWriter():
    def __init__(self, file):
        self.symbol_table_container = SymbolTableContainer.deserialize(file)

    def generate_c_file(self, c_filepath):
        symbol_string_list = []
        for symbol in self.symbol_table_container.symbol_table:
            address = "0x{}".format(symbol.address[2:].zfill(8))
            data_type = "data_type_{}".format(symbol.data_type.value)
            bit_size = symbol.bit_size if (None != symbol.bit_size) else 0
            bit_offset = symbol.bit_offset if (None != symbol.bit_offset) else 0
            symbol_string = STRUCT_FORMAT % (symbol.name, address, data_type, symbol.size, bit_size, bit_offset)
            symbol_string_list.append(symbol_string)

        struct_string = "".join(symbol_string_list)

        c_file = SYMBOL_TABLE_C_FILE_FORMAT.format(
            symbol_table_size=self.symbol_table_container.count,
            symbol_table_entries=struct_string,)

        with open(c_filepath, "w") as file:
            file.write(c_file)
