"""
References:
- https://sourceware.org/binutils/docs/binutils/readelf.html
- https://github.com/eliben/pyelftools
"""

from collections import defaultdict, OrderedDict
import os, sys
from itertools import dropwhile
import re
import json

from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection
from elftools.elf.descriptions import describe_symbol_type
from elftools.common.py3compat import itervalues
from elftools.dwarf.descriptions import describe_attr_value


SELF_DIRPATH = os.path.dirname(__file__)
REPO_ROOT_DIRPATH = os.path.join(SELF_DIRPATH, "../../..")
DEFAULT_ELF_FILEPATH = os.path.join(REPO_ROOT_DIRPATH, "_build_lpc40xx_freertos", "lpc40xx_freertos.elf")
DEFAULT_FILE_NAME = 'lpc40xx_freertos'

class Symbol(object):
    def __init__(self, symbol, address, size, type=''):
        self.entry = defaultdict(dict)
        self.name = symbol

        struct = {'symbol' : symbol, 'address': address, 'size': size, 'type': type}
        self.entry[symbol] =  struct


    def __getitem__(self, symbol):
        return self.entry[symbol]

class SymbolTable(object):
    def __init__(self):
        self.table = []

    def add_entry(self, symbol, address, size, type=''):
        self.table.append(Symbol(symbol,address,size,type))

    def __getitem__(self, num):
        return self.table[num]


class SymbolTableParser(object):
    def __init__(self, file):
        self.elffile = ELFFile(file)

        self._symbol_table = SymbolTable()
        self._dwarf_abbrev_dict = OrderedDict()

        self.valid_symbol_table = None


        self._parse_symbol_table()
        self._parse_dwarf_info()
        self._parse_valid_symbol()

    """
    Public methods
    """
    def to_json(self, file_name=DEFAULT_FILE_NAME):
        """ output symbol table to json file

            file_name:
                name of json file to output
        """
        json_output = '{0}.json'.format(file_name)
        if self.valid_symbol_table:
            with open(json_output, 'w') as wf:
                for symbol in self.valid_symbol_table:
                    json.dump(symbol[symbol.name], wf, indent=True)

    """
    Private methods
    """
    def _parse_symbol_table(self):
        """ build symbol table data structure
        """
        symbol_tables = [(idx, s) for idx, s in enumerate(self.elffile.iter_sections())
                         if isinstance(s, SymbolTableSection)]

        for section_index, section in symbol_tables:
            if not isinstance(section, SymbolTableSection):
                continue

            for nsym, symbol in enumerate(section.iter_symbols()):
                if ((int(symbol['st_size']) > 0) and ('OBJECT' == describe_symbol_type(symbol['st_info']['type']))):
                    sym_value = self._format_hex(symbol['st_value'], fullhex=True, lead0x=True)
                    self._symbol_table.add_entry(symbol.name, sym_value, str(symbol['st_size']))


    def _parse_dwarf_info(self):
        """ build dwarf info data structure
        """
        dwarf_info = self.elffile.get_dwarf_info()

        if not dwarf_info.has_debug_info:
            return

        section_offset = dwarf_info.debug_info_sec.global_offset

        for cu in dwarf_info.iter_CUs():
            die_depth = 0
            for die in cu.iter_DIEs():

                if die.is_null():
                    die_depth -= 1
                    continue

                # abbrev property of interest
                abbrev_dict = OrderedDict()
                abbrev_dict['depth'] = die_depth
                abbrev_dict['offset'] = hex(die.offset)
                abbrev_dict['code'] = die.abbrev_code
                abbrev_dict['tag'] = die.tag if not die.is_null() else ''
                abbrev_dict['attr'] = []

                for attr in itervalues(die.attributes):
                    # abbrev attributes of interest, only look for these attributes
                    attr_type = [{'DW_AT_name':': ([a-zA-Z_][a-zA-Z0-9_]*)'},       # variable name
                                    {'DW_AT_type':'0[xX][0-9a-fA-F]+'},             # hex number
                                    {'DW_AT_location':'DW_OP_addr: [0-9a-fA-F]+'},  # hex number without 0x
                                    {'DW_AT_data_member_location': '[0-9]+'},       # base10 number
                                    {'DW_AT_sibling': '0[xX][0-9a-fA-F]+'},         # hex number
                                    {'DW_AT_byte_size': '[0-9]+'}]                  # base10 number
                    name = attr.name

                    attr_desc = describe_attr_value(attr, die, section_offset)

                    attr_dict = OrderedDict()
                    attr_dict['offset'] = hex(attr.offset)
                    attr_dict['name'] = name


                    for types in attr_type:
                        for k , v in types.items():
                            if attr_dict['name'] == k:
                                attr_desc = self._format_attr_desc(attr_desc, k , v)
                                attr_dict['desc'] = attr_desc.rstrip()
                                abbrev_dict['attr'].append(attr_dict)
                                break
                
                if abbrev_dict['attr']:
                    self._dwarf_abbrev_dict[hex(die.offset)] = abbrev_dict

                if die.has_children:
                    die_depth += 1

    def _parse_valid_symbol(self):
        """ build valid symbol table data structure
            compare symbol name and address from symbol table with dwarf info attributes
            valid symbol if address match and symbol name within dwarf attribute
            get the base type for all valid symbols
        """
        self.valid_symbol_table = SymbolTable()

        if self._symbol_table is None or self._dwarf_abbrev_dict is None:
            return

        for entry in self._symbol_table:
            for abbrev in self._dwarf_abbrev_dict:
                symbol_found = False
                addr_found = False
                for attr in self._dwarf_abbrev_dict[abbrev]['attr']:
                    # There may be multiple symbol names so we do not match exact string
                    if attr['desc'] in entry[entry.name]['symbol']:
                        if not symbol_found:
                            symbol_found = True
                    # We do however, match address since its definitive
                    if attr['desc'] == entry[entry.name]['address']:
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
                            member['symbol'] = '{0}.{1}'.format(entry[entry.name]['symbol'], member['symbol'])
                            member['address'] = self._add_hex(entry[entry.name]['address'], member['address'])
                            self.valid_symbol_table.add_entry(member['symbol'], member['address'], member['size'], member['type'])

                    elif base_type == 'void':
                        # do nothing
                        pass
                    else:
                        # base types or pointers
                        entry[entry.name]['type'] = base_type
                        self.valid_symbol_table.add_entry(entry[entry.name]['symbol'], entry[entry.name]['address'],
                                            entry[entry.name]['size'], entry[entry.name]['type'])
                    break

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
                if not isinstance(s[s.name]['type'], str):
                    for member in s[s.name]['type']:
                        member_list =  self._decode_struct(member)
                        for m in member_list:
                            m['symbol'] = '{0}.{1}'.format(s[s.name]['symbol'], m['symbol'])
                            m['address'] = self._add_hex(s[s.name]['address'], m['address'])
                            struct_list.append(m)
                else:
                    struct_list.append(s[s.name])
        elif isinstance(struct, Symbol):
            member_list = self._decode_struct(struct[struct.name]['type'])

            for m in member_list:
                m['symbol'] = '{0}.{1}'.format(struct[struct.name]['symbol'], m['symbol'])
                m['address'] = self._add_hex(struct[struct.name]['address'], m['address'])
                struct_list.append(m)

            if not isinstance(struct[struct.name]['type'],list):
                struct_list.append(struct[struct.name])

        return struct_list

    def _get_type(self, offset):
        """ get base symbol information
            recursive call to find base type, if needed

            offset:
                offset number in dwarf info to start search

            return:
                return tuple of base symbol information
        """
        symbol_offset = self._get_type_offset(offset, 'DW_AT_type')

        if symbol_offset:
            # we found the root
            symbol_type = self._get_description(symbol_offset, 'DW_AT_name')
            symbol_size = self._get_description(symbol_offset, 'DW_AT_byte_size')
            
            if self._is_struct_or_union(symbol_offset):
                # name the type struct for later processing
                symbol_type = 'struct'
            if self._is_pointer(symbol_offset):
                symbol_type = 'uintptr_t'
            elif not symbol_type:
                # name the type void for later processing
                symbol_type = 'void'
            # return tuple after we find base type
            return (symbol_offset,symbol_type, symbol_size)


    def _get_type_offset(self, offset, type):
        """ get descripton at offset for type

            offset:
                offset number in dwarf info

            type:
                attribute type of interest, i.e. DW_AT_type

            return:
                offset location to find base type
        """
        if self._is_pointer(offset):
            # return early if pointer type
            return offset

        symbol_offset = self._get_description(offset, type)

        if symbol_offset:
            return self._get_type_offset(symbol_offset, type)
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

        for k, v in dropwhile(lambda x: x[0] != offset, self._dwarf_abbrev_dict.items()):
            # we start iterating from offset
            if v['offset'] == offset:
                # we skip it, we are interested in the struct members
                base_die_depth = v['depth']
                continue

            if v['tag'] == 'DW_TAG_member' and v['depth'] == base_die_depth+1:
                member_name = self._get_description(k, 'DW_AT_name')
                member_location_offset = self._get_description(k, 'DW_AT_data_member_location')
                if not member_location_offset:
                    # for union, since they do not have 'DW_AT_data_member_location' type
                    member_location_offset = 0
                else:
                    member_location_offset = int(member_location_offset)
                base_member_offset, base_member_type, base_member_size = self._get_type(k)
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
        return self._check_abbrev_tag(offset, 'DW_TAG_structure_type') \
                or self._check_abbrev_tag(offset, 'DW_TAG_union_type')

    def _is_pointer(self, offset):
        """ check if pointer tag exists at offset

            offset:
                offset number in dwarf info

            return:
                True if pointer tag, else False
        """
        return self._check_abbrev_tag(offset, 'DW_TAG_pointer_type')

    def _check_abbrev_tag(self, offset, tag):
        """ check if abbrev tag exists at offset

            offset:
                offset number in dwarf info

            tag:
                tag to check if it exists

            return:
                boolean value if tag matches
        """
        if self._dwarf_abbrev_dict is None:
            return
        same_tag = False

        if self._dwarf_abbrev_dict is not None:
            if offset in self._dwarf_abbrev_dict:
                same_tag = self._dwarf_abbrev_dict[offset]['tag'] == tag

        return same_tag

    def _get_description(self, offset, attr_type):
        """ get description for attribute type at offset

            offset:
                offset number in dwarf info

            attr_type:
                attribute type of interest

            return:
                return description for attribute type at offset if it exist
        """
        if self._dwarf_abbrev_dict is None:
            return
        if offset in self._dwarf_abbrev_dict:
            for attr in self._dwarf_abbrev_dict[offset]['attr']:
                if attr['name'] == attr_type:
                    return attr['desc']

        return None


    def _format_attr_desc(self, desc, type, regex):
        """match regex pattern in desc if any exist

            desc:
                string to be cleaned

            type:
                attribute type of interest

            regex:
                pattern to be matched

            return:
                return clean data if desc match regex pattern, else retun desc
        """
        description = ''
        match = re.compile(regex)
        match = match.search(desc)
        if match:
            description = match.group()
            # hacky solution to further clean the data...
            if 'DW_AT_name' == type or 'DW_AT_location' == type:
                # assume string looks like: (indirect string, offset: 0x9f4): crash_registers
                description = description.split(' ')[1]
                if 'DW_AT_location' == type:
                    # assume string looks like: 5 byte block: 3 0 0 0 20    (DW_OP_addr: 20000000)
                    description = self._format_hex(int(description,16), fullhex=True, lead0x=True)
        else:
            description = desc

        return description

    @staticmethod
    def _add_hex(num1, num2):
        """ add hex strings together or base10 addition

            num1:
                a hex string or base10 number

            num2:
                a hex string or base10 number

            return:
                return hex string for addition of num1 and num2
        """
        if isinstance(num1, str):
            hex1 = int(num1, 16)
        else:
            hex1 = num1

        if isinstance(num2, str):
            hex2 = int(num2, 16)
        else:
            hex2 = num2

        return hex(hex1 + hex2)

    def _format_hex(self, addr, fieldsize=None, fullhex=False, lead0x=True,
                    alternate=False):
        """ Format an address into a hexadecimal string.

            fieldsize:
                Size of the hexadecimal field (with leading zeros to fit the
                address into. For example with fieldsize=8, the format will
                be %08x
                If None, the minimal required field size will be used.

            fullhex:
                If True, override fieldsize to set it to the maximal size
                needed for the elfclass

            lead0x:
                If True, leading 0x is added

            alternate:
                If True, override lead0x to emulate the alternate
                hexadecimal form specified in format string with the #
                character: only non-zero values are prefixed with 0x.
                This form is used by readelf.
        """
        if alternate:
            if addr == 0:
                lead0x = False
            else:
                lead0x = True
                fieldsize -= 2

        s = '0x' if lead0x else ''
        if fullhex:
            fieldsize = 8 if self.elffile.elfclass == 32 else 16
        if fieldsize is None:
            field = '%x'
        else:
            field = '%' + '0%sx' % fieldsize
        return s + field % addr

if __name__ == "__main__":  # Test
    input_filepath = os.path.abspath(DEFAULT_ELF_FILEPATH)

    try:
        with open(input_filepath,'rb') as file:
            _symbol_table_parser = SymbolTableParser(file)
            _symbol_table_parser.to_json()
    except Exception as e:
        print(e)
        sys.exit(1)  # Exit
