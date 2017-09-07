#ifndef ST_FUZZER_ELF
#define ST_FUZZER_ELF

#include <stdint.h>
#include <stdbool.h>

/*
 * With reference to:
 * http://www.skyfree.org/linux/references/ELF_Format.pdf
 */

typedef uint32_t Elf32_Addr;
typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Off;
typedef  int32_t Elf32_Sword;
typedef uint32_t Elf32_Word;

#define EI_IDENT 16

typedef struct {
    unsigned char e_ident[EI_IDENT];
    Elf32_Half    e_type;
    Elf32_Half    e_machine;
    Elf32_Word    e_version;
    Elf32_Addr    e_entry;
    Elf32_Off     e_phoff;
    Elf32_Off     e_shoff;
    Elf32_Word    e_flags;
    Elf32_Half    e_ehsize;
    Elf32_Half    e_phentsize;
    Elf32_Half    e_phnum;
    Elf32_Half    e_shentsize;
    Elf32_Half    e_shnum;
    Elf32_Half    e_shstrndx;
} Elf32_Ehdr;

#define SHT_SYMTAB 2
#define SHT_STRTAB 3

typedef struct {
    Elf32_Word sh_name;
    Elf32_Word sh_type;
    Elf32_Word sh_flags;
    Elf32_Addr sh_addr;
    Elf32_Off  sh_offset;
    Elf32_Word sh_size;
    Elf32_Word sh_link;
    Elf32_Word sh_info;
    Elf32_Word sh_addralign;
    Elf32_Word sh_entsize;
} Elf32_Shdr;

typedef struct {
    Elf32_Word    st_name;
    Elf32_Addr    st_value;
    Elf32_Off     st_size;
    unsigned char st_info;
    unsigned char st_other;
    Elf32_Half    st_shndx;
} Elf32_Sym;

typedef struct {
    uint32_t elf_strings_len;
    char * elf_strings;
    Elf32_Word n_syms;
    Elf32_Sym * syms;
} ExecData;

bool elf_load_symbols(ExecData * data, char * sym_file);
Elf32_Addr elf_lookup_symbol(ExecData * data, char * symbol_name);

#endif
