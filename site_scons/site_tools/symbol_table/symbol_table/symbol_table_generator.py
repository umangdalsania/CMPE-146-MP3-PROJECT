"""
References:
- https://sourceware.org/binutils/docs/binutils/readelf.html
- https://github.com/eliben/pyelftools
"""

from collections import defaultdict, OrderedDict
from itertools import dropwhile
import json
import os
import sys
import re

from elftools.dwarf.descriptions import describe_attr_value
from elftools.elf.descriptions import describe_symbol_type
from elftools.elf.elffile import ELFFile
from elftools.common.py3compat import itervalues
from elftools.elf.sections import SymbolTableSection


SELF_DIRPATH = os.path.dirname(__file__)
REPO_ROOT_DIRPATH = os.path.join(SELF_DIRPATH, "../../../..")
DEFAULT_ELF_FILEPATH = os.path.join(REPO_ROOT_DIRPATH, "_build_lpc40xx_freertos", "lpc40xx_freertos.elf")
DEFAULT_JSON_FILEPATH = os.path.join(SELF_DIRPATH, 'lpc40xx_freertos.json')


class Symbol(object):
    def __init__(self, symbol, address, size, data_type=''):
        self._name = symbol
        self._address = address
        self._size = size
        self._data_type = data_type

    @property
    def name(self):
        return self._name

    @name.setter
    def name(self, value):
        self._name = value

    @property
    def address(self):
        return self._address

    @address.setter
    def address(self, value):
        self._address = value

    @property
    def size(self):
        return self._size

    @size.setter
    def size(self, value):
        self._size = value

    @property
    def data_type(self):
        return self._data_type

    @data_type.setter
    def data_type(self, value):
        self._data_type = value


class SymbolTable(object):
    def __init__(self):
        self.table = []

    def add_entry(self, symbol, address, size, data_type=''):
        self.table.append(Symbol(symbol, address, size, data_type))

    def __getitem__(self, num):
        return self.table[num]


class SymbolTableParser(object):
    def __init__(self, elf_obj):
        self._elf = elf_obj
        self.symbol_table = None

    def parse_symbol_table(self):
        """ build symbol table data structure
        """
        if self.symbol_table is None:
            self.symbol_table = SymbolTable()

            symbol_tables = [section for section in self._elf.iter_sections() if isinstance(section, SymbolTableSection)]

            for section in symbol_tables:
                for symbol in section.iter_symbols():
                    if ((int(symbol['st_size']) > 0) and ('OBJECT' == describe_symbol_type(symbol['st_info']['type']))):
                        self.symbol_table.add_entry(symbol.name, symbol['st_value'], symbol['st_size'])

        return self.symbol_table


class DwarfInfoParser(object):
    def __init__(self, elf_obj):
        self._elf = elf_obj
        self.dwarf_info_dict = None

    """
    Public methods
    """
    def parse_dwarf_info(self):
        """ build dwarf info data structure
        """
        if self.dwarf_info_dict is None:
            self.dwarf_info_dict = OrderedDict()

            dwarf_info = self._elf.get_dwarf_info()
            if not dwarf_info.has_debug_info:
                raise ValueError('Debug information not available in ELF file. \
                                    Symbol table will be empty')

            section_offset = dwarf_info.debug_info_sec.global_offset

            for cu in dwarf_info.iter_CUs():
                die_depth = 0
                for die in cu.iter_DIEs():

                    if die.is_null():
                        die_depth -= 1
                        continue

                    # abbreviation property of interest
                    abbreviation = OrderedDict()
                    abbreviation['depth'] = die_depth
                    abbreviation['offset'] = die.offset
                    abbreviation['code'] = die.abbrev_code
                    abbreviation['tag'] = die.tag if not die.is_null() else ''
                    abbreviation['attr'] = []

                    for attr in itervalues(die.attributes):
                        description = self._get_attribute_description(attr, die)

                        if description is not None:
                            attr_dict = OrderedDict()
                            attr_dict['offset'] = attr.offset
                            attr_dict['name'] = attr.name
                            attr_dict['desc'] = description
                            abbreviation['attr'].append(attr_dict)

                    if abbreviation['attr']:
                        self.dwarf_info_dict[die.offset] = abbreviation

                    if die.has_children:
                        die_depth += 1

        return self.dwarf_info_dict

    """
    Private methods
    """
    def _get_attribute_description(self, attr, die):

        description = describe_attr_value(attr, die, 0)
        regex_pattern = ''
        if 'DW_AT_name' == attr.name:
            regex_pattern = '^([\w ]+\t)|: ([\w ]+\t)$'
        elif 'DW_AT_type' == attr.name:
            regex_pattern = '^<(0x[\da-fA-F]+)>\t$'
        elif 'DW_AT_location' == attr.name:
            regex_pattern = '.*DW_OP_addr: ([\w]+)'
        elif 'DW_AT_data_member_location' == attr.name:
            regex_pattern = '^([\d]+\t)$'
        elif 'DW_AT_byte_size' == attr.name:
            regex_pattern = '^([\d]+\t)$'

        if '' != regex_pattern:
            match = re.compile(regex_pattern)
            match = match.search(description)
            if match:
                match_group = match.groups()

                if attr.name in ['DW_AT_type', 'DW_AT_location']:
                    description = match_group[0].rstrip()
                    description = int(description, 16)

                elif attr.name in ['DW_AT_data_member_location', 'DW_AT_byte_size']:
                    description = match_group[0].rstrip()
                    description = int(description)

                elif attr.name in ['DW_AT_name']:
                    index = [match for match in range(len(match_group)) if match_group[match] != None]
                    description = match_group[index[0]].rstrip()
                else:
                    pass
            else:
                description = description.rstrip()
        else:
            description = None

        return description


class SymbolTableGenerator(object):
    def __init__(self, filepath):
        self._elf = ELFFile(filepath)

        self._symbol_table_parser = SymbolTableParser(self._elf)
        self._dwarf_info_parser = DwarfInfoParser(self._elf)

        self.valid_symbol_table = None

        self._symbol_table = self._symbol_table_parser.parse_symbol_table()
        self._dwarf_info = self._dwarf_info_parser.parse_dwarf_info()

    """
    Public methods
    """
    def to_json(self, json_filepath=DEFAULT_JSON_FILEPATH):
        """ output symbol table to json file

            json_filepath:
                name of json file to output
        """
        if self.valid_symbol_table:
            with open(json_filepath, 'w') as file:
                for symbol in self.valid_symbol_table:
                    json.dump(vars(symbol), file, indent=4)

    def generate_symbol_table(self):
        """ build valid symbol table data structure
            compare symbol name and address from symbol table with dwarf info attributes
            valid symbol if address match and symbol name within dwarf attribute
            get the base type for all valid symbols
        """
        if self.valid_symbol_table is None:
            self.valid_symbol_table = SymbolTable()

            if self._symbol_table is None or self._dwarf_info is None:
                return

            for entry in self._symbol_table:
                for abbrev in self._dwarf_info:
                    symbol_found = False
                    addr_found = False
                    for attr in self._dwarf_info[abbrev]['attr']:
                        # There may be multiple symbol names so we do not match exact string
                        if isinstance(attr['desc'], str):
                            if attr['desc'] in entry.name:
                                if not symbol_found:
                                    symbol_found = True
                        # We do however, match address since its definitive
                        if isinstance(attr['desc'], int):
                            if attr['desc'] == entry.address:
                                if not addr_found:
                                    addr_found = True

                    # We only want symbols that have address in its DWARF attributes
                    # get symbol type also
                    if symbol_found and addr_found:
                        # time to go down rabbit holes...
                        base_offset, base_type , base_size = self._get_type(abbrev)
                        if base_type == 'struct':
                            # collect member info from struct
                            members = self._get_struct_or_union_members(base_offset)
                            decoded_struct = self._decode_struct(members)
                            for member in decoded_struct:
                                # append struct info to its member
                                member.name = '{0}.{1}'.format(entry.name, member.name)
                                member.address = entry.address + member.address
                                member.data_type = self._decode_type(member.data_type, member.size)
                                self.valid_symbol_table.add_entry(member.name, hex(member.address),
                                                                  str(member.size), member.data_type)
                        elif base_type == 'void':
                            # do nothing
                            pass
                        else:
                            # base types or pointers
                            entry.data_type = base_type
                            entry.size = base_size
                            entry.data_type = self._decode_type(base_type, entry.size)
                            self.valid_symbol_table.add_entry(entry.name, hex(entry.address),
                                                             str(entry.size), entry.data_type)
                        break

        return self.valid_symbol_table

    """
    Private methods
    """
    def _decode_struct(self, struct):
        """ decode all members in struct
            recursive call itself to decode structs within structs, if any exists

            struct:
                a list of Symbol objects or Symbol object

            return:
                a list of all members in struct
        """
        struct_list = []
        if isinstance(struct, list):
            for s in struct:
                if not isinstance(s.data_type, str):
                    for member in s.data_type:
                        member_list =  self._decode_struct(member)
                        for m in member_list:
                            m.name = '{0}.{1}'.format(s.name, m.name)
                            m.address = s.address + m.address
                            struct_list.append(m)
                else:
                    struct_list.append(s)
        elif isinstance(struct, Symbol):
            member_list = self._decode_struct(struct.data_type)

            for m in member_list:
                m.name = '{0}.{1}'.format(struct.name, m.name)
                m.address = struct.address + m.address
                struct_list.append(m)

            if not isinstance(struct.data_type, list):
                struct_list.append(struct)

        return struct_list

    @staticmethod
    def _decode_type(data_type, data_size):
        """ Given base type and size, translate to a generic set of
            data type strings
        """
        base_type = ''
        sign = ''
        size = ''
        if 'char' == data_type:
            size_to_data_type_map = {
                1: 'char'
            }
            size = size_to_data_type_map[data_size]
        elif 'ptr' == data_type:
            size_to_data_type_map = {
                4: 'uintptr'
            }
            size = size_to_data_type_map[data_size]
        elif 'int' in data_type:
            size_to_data_type_map = {
                1: 'int8',
                2: 'int16',
                4: 'int32',
                8: 'int64'
            }
            if 'unsigned' in data_type:
                sign = 'u'
            size = size_to_data_type_map[data_size]
        elif 'char' in data_type:
            size_to_data_type_map = {
                1: 'int8'
            }
            if 'unsigned' in data_type:
                sign = 'u'
            size = size_to_data_type_map[data_size]
        elif 'float' in data_type:
            size_to_data_type_map = {
                4: 'float'
            }
            size = size_to_data_type_map[data_size]
        elif 'double' in data_type:
            size_to_data_type_map = {
                8: 'double'
            }
            size = size_to_data_type_map[data_size]
        elif data_type in ['bool', '_Bool']:
            size_to_data_type_map = {
                1: 'bool'
            }
            size = size_to_data_type_map[data_size]
        else:
            print (data_type, data_size)

        base_type = '{0}{1}'.format(sign, size)
        decoded_type = 'data_type__{0}'.format(base_type)

        return decoded_type

    def _get_type(self, offset):
        """ get base symbol information
            recursive call to find base type, if needed

            offset:
                offset number in dwarf info to start search

            return:
                return tuple of base symbol information
        """
        symbol_offset = self._get_type_offset(offset)

        if symbol_offset:
            # we found the root
            symbol_type = self._get_description(symbol_offset, 'DW_AT_name')
            symbol_size = self._get_description(symbol_offset, 'DW_AT_byte_size')

            if self._is_struct_or_union(symbol_offset):
                # name the type struct for later processing
                symbol_type = 'struct'
            elif self._is_pointer(symbol_offset):
                symbol_type = 'ptr'
            elif not symbol_type:
                # name the type void for later processing
                symbol_type = 'void'
            else:
                # base types
                pass
            # return tuple after we find base type
            return (symbol_offset,symbol_type, symbol_size)

    def _get_type_offset(self, offset):
        """ get descripton at offset for type

            offset:
                offset number in dwarf info

            return:
                offset location to find base type
        """
        if self._is_pointer(offset):
            # return early if pointer type
            return offset

        symbol_offset = self._get_description(offset, 'DW_AT_type')

        if symbol_offset:
            return self._get_type_offset(symbol_offset)
        else:
            return offset

    def _get_struct_or_union_members(self, offset):
        """ get structure or union members starting at offset

            offset:
                offset number in dwarf info

            return:
                a list of Symbol object which contain struct member information
                it is possible for Symbol['type'] to be either a string or a list
                Symbol['type'] is a list if that member is a struct and ['type'] contains the struct member
        """
        struct_members = []

        for key, val in dropwhile(lambda x: x[0] != offset, self._dwarf_info.items()):
            # we start iterating from offset
            if val['offset'] == offset:
                # we skip it, we are interested in the struct members
                base_die_depth = val['depth']
                continue

            if val['tag'] == 'DW_TAG_member' and val['depth'] == base_die_depth+1:
                member_name = self._get_description(key, 'DW_AT_name')
                member_location_offset = self._get_description(key, 'DW_AT_data_member_location')
                if not member_location_offset:
                    # for union, since they do not have 'DW_AT_data_member_location' type
                    member_location_offset = 0
                else:
                    member_location_offset = int(member_location_offset)
                base_member_offset, base_member_type, base_member_size = self._get_type(key)
                if base_member_type == 'struct':
                    # struct inside struct
                    base_member_type = self._get_struct_or_union_members(base_member_offset)
                    # embed struct members in 'type' key to be decoded

                new_struct_member = Symbol(member_name, member_location_offset, base_member_size,  base_member_type)
                struct_members.append(new_struct_member)
            else:
                return struct_members

    def _is_struct_or_union(self, offset):
        """ check if struct or union tag exists at offset

            offset:
                offset number in dwarf info

            return:
                True if struct or union tag, else False
        """
        dwarf_tag = self._get_dwarf_tag(offset)
        return dwarf_tag in ['DW_TAG_structure_type', 'DW_TAG_union_type']

    def _is_pointer(self, offset):
        """ check if pointer tag exists at offset

            offset:
                offset number in dwarf info

            return:
                True if pointer tag, else False
        """
        dwarf_tag = self._get_dwarf_tag(offset)
        return  (dwarf_tag == 'DW_TAG_pointer_type')

    def _get_dwarf_tag(self, offset):
        """ check if abbrev tag exists at offset

            offset:
                offset number in dwarf info

            return:
                boolean value if tag matches
        """
        if self._dwarf_info is None:
            return

        if offset in self._dwarf_info:
            tag = self._dwarf_info[offset]['tag']

            if not tag:
                raise ValueError('Abbreviation tag not found')

        return tag

    def _get_description(self, offset, attr_type):
        """ get description for attribute type at offset

            offset:
                offset number in dwarf info

            attr_type:
                attribute type of interest

            return:
                return description for attribute type at offset if it exist
        """
        if self._dwarf_info is None:
            return

        if offset in self._dwarf_info:
            for attr in self._dwarf_info[offset]['attr']:
                if attr['name'] == attr_type:
                    return attr['desc']

        return None

if __name__ == "__main__":  # Test
    input_filepath = os.path.abspath(DEFAULT_ELF_FILEPATH)

    try:
        with open(input_filepath,'rb') as file:
            _symbol_table_generator = SymbolTableGenerator(file)
            _symbol_table_generator.generate_symbol_table()
            _symbol_table_generator.to_json()
    except Exception as e:
        print(e)
        sys.exit(1)  # Exit
