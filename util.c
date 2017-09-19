#include "util.h"

bool safe_compare_null_term(char * v1, char * v2, uint32_t max_len) {
    for (int i=0; i < max_len + 1; i++) {
        if ((v1[i] == '\0') && (v2[i] == '\0')) return true;
        if (v1[i] != v2[i]) return false;
    }
    
    return false;
}
