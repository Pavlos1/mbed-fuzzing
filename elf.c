#include "elf.h"
#include "util.h"

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>


/**
 * Internal use only. Sets symbol data to
 * invalid values to signify that no symbols
 * are available.
 */
void elf_no_symbols(ExecData * data) {
    data->elf_strings = NULL;
    data->syms = NULL;
    data->n_syms = 0;
    data->elf_strings_len = 0;
}


/**
 * Performs a linear search on the symbol table
 * to find an address corresponding to a given
 * label
 *
 * Returns 0 if symbol was not found
 */
Elf32_Addr elf_lookup_symbol(ExecData * data, char * symbol_name) {
    if ((data->elf_strings == NULL) || (data->syms == NULL)) return 0;
    
    for (int i=0; i<data->n_syms; i++) {
        char * cur_symbol_name = &data->elf_strings[data->syms[i].st_name];
        uint32_t max_string_len = data->elf_strings_len - data->syms[i].st_name;
        printf("[DEBUG] Matching symbol: \"%s\"\n", cur_symbol_name);
        if (safe_compare_null_term(cur_symbol_name, symbol_name, max_string_len)) {
            return data->syms[i].st_value;
        }
    }
    
    return 0;
}


/**
 * Loads (SYMTAB) symbols from ELF file
 *
 * Returns true iff. symbols were read successfully
 */
bool elf_load_symbols(ExecData * data, char * sym_file) {
    int fd = open(sym_file, O_RDONLY);
    if (fd <= 0) {
        printf("[DEBUG] Failed to open symbol file\n");
        elf_no_symbols(data);
        return false;
    }
    
    Elf32_Ehdr e_hdr;
    read(fd, &e_hdr, sizeof(Elf32_Ehdr));
    
    // Only a rudimentary check
    // XXX: Should we at least check endianness or arch?
    if (!strncmp(e_hdr.e_ident, "\x7D" "ELF", 4)) {
        printf("[DEBUG] Not an ELF file\n");
        elf_no_symbols(data);
        return false;
    }
    
    if (e_hdr.e_shentsize != sizeof(Elf32_Shdr)) {
        printf("[DEBUG] Bad section sizes\n");
        printf("[DEBUG] Expecting: %llu\n", sizeof(Elf32_Shdr));
        printf("[DEBUG] Got: %llu\n", e_hdr.e_shentsize);
        elf_no_symbols(data);
        return false;
    }
    
    if (!e_hdr.e_shoff) {
        printf("[DEBUG] No sections\n");
        elf_no_symbols(data);
        return false;
    }
    
    if (!e_hdr.e_shstrndx) {
        printf("[DEBUG] No strings\n");
        elf_no_symbols(data);
        return false;
    }
    
    lseek(fd, e_hdr.e_shoff, SEEK_SET);
    Elf32_Shdr sections[e_hdr.e_shnum];
    read(fd, &sections, e_hdr.e_shnum * sizeof(Elf32_Shdr));
    
    Elf32_Shdr * header_names = &sections[e_hdr.e_shstrndx];
    
    printf("[DEBUG] malloc'ing header_names_str\n");
    char * header_names_str = safe_malloc(header_names->sh_size);
    lseek(fd, header_names->sh_offset, SEEK_SET);
    read(fd, header_names_str, header_names->sh_size);
    
    Elf32_Shdr * symtab = NULL;
    Elf32_Shdr * strtab = NULL;
    for (int i=0; i<e_hdr.e_shnum; i++) {
        printf("[DEBUG] Section type: %d\n", sections[i].sh_type);
        if (sections[i].sh_type == SHT_SYMTAB) {
            printf("[DEBUG] Found symtab\n");
            symtab = &sections[i];
        } else if (sections[i].sh_type == SHT_STRTAB) {
            printf("[DEBUG] Found strtab\n");
            char * name = &header_names_str[sections[i].sh_name];
            uint32_t name_max_len = header_names->sh_size - sections[i].sh_name;
            if (safe_compare_null_term(name, ".strtab", name_max_len)) {
                printf("[DEBUG] Found .strtab\n");
                strtab = &sections[i];
            }
        }
    }
    
    if ((symtab == NULL) || (strtab == NULL)) {
        printf("[DEBUG] No symtab or strtab section\n");
        elf_no_symbols(data);
        free(header_names_str);
        return false;
    }
    
    if (symtab->sh_entsize != sizeof(Elf32_Sym)) {
        printf("[DEBUG] Bad symbol entry sizes\n");
        elf_no_symbols(data);
        free(header_names_str);
        return false;
    }
    
    data->elf_strings_len = strtab->sh_size;
    lseek(fd, strtab->sh_offset, SEEK_SET);
    data->elf_strings = safe_malloc(strtab->sh_size);
    read(fd, data->elf_strings, strtab->sh_size);
    
    data->n_syms = symtab->sh_size;
    lseek(fd, symtab->sh_offset, SEEK_SET);
    data->syms = safe_malloc(symtab->sh_size);
    read(fd, data->syms, symtab->sh_size);
    
    close(fd);
    free(header_names_str);
    
    printf("[DEBUG] Successfully loaded %u symbols\n", data->n_syms);
    printf("[DEBUG] They are:\n");
    for (int i=0; i<data->n_syms; i++) {
        uint32_t index = data->syms[i].st_name;
        printf("[DEBUG] %s\n", &data->elf_strings[index], data->elf_strings_len - index);
    }
    
    return true;
}
