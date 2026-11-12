#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#define ROTL64(x, b) (((x) << (b)) | ((x) >> (64 - (b))))

// Função auxiliar para printar o estado atual das 4 variáveis de 64 bits em Hex
static inline void print_state(const char* label, uint64_t v0, uint64_t v1, uint64_t v2, uint64_t v3) {
    printf("  [TRACE] %-20s -> v0: %016lx | v1: %016lx | v2: %016lx | v3: %016lx\n", label, v0, v1, v2, v3);
}

// Macro de round com print embutido para inspecionar a mistura
#define SIPROUND_DEBUG(name) \
    do { \
        v0 += v1; v1 = ROTL64(v1, 13); \
        v1 ^= v0; v0 = ROTL64(v0, 32); \
        v2 += v3; v3 = ROTL64(v3, 16); \
        v3 ^= v2; \
        v0 += v3; v3 = ROTL64(v3, 21); \
        v3 ^= v0; \
        v2 += v1; v1 = ROTL64(v1, 17); \
        v1 ^= v2; v2 = ROTL64(v2, 32); \
        print_state(name, v0, v1, v2, v3); \
    } while(0)

uint64_t siphash24_debug(const void *data, size_t len, uint64_t k0, uint64_t k1) {
    const unsigned char *in = (const unsigned char *)data;
    
    printf("\n=== [SIPHASH] INICIO (len: %zu) ===\n", len);

    uint64_t v0 = k0 ^ 0x736f6d6570737564ULL;
    uint64_t v1 = k1 ^ 0x646f72616e646f6dULL;
    uint64_t v2 = k0 ^ 0x6c7967656e657261ULL;
    uint64_t v3 = k1 ^ 0x7465646279746573ULL;

    print_state("Estado Inicial", v0, v1, v2, v3);

    uint64_t b = ((uint64_t)len) << 56;
    const size_t left = len & 7;
    const unsigned char *end = in + len - left;
    int block_num = 0;

    // Processamento por blocos de 8 bytes
    for (; in != end; in += 8) {
        uint64_t m = (uint64_t)in[0] | ((uint64_t)in[1] << 8) |
                    ((uint64_t)in[2] << 16) | ((uint64_t)in[3] << 24) |
                    ((uint64_t)in[4] << 32) | ((uint64_t)in[5] << 40) |
                    ((uint64_t)in[6] << 48) | ((uint64_t)in[7] << 56);

        printf("\n--- Bloco %d (m = %016lx) ---\n", block_num++, m);
        v3 ^= m;
        print_state("Apos v3 ^= m", v0, v1, v2, v3);
        
        SIPROUND_DEBUG("Bloco Round 1");
        SIPROUND_DEBUG("Bloco Round 2");
        
        v0 ^= m;
        print_state("Apos v0 ^= m", v0, v1, v2, v3);
    }

    // Processamento do último bloco (resto)
    uint64_t last = 0;
    switch (left) {
        case 7: last |= ((uint64_t)in[6]) << 48; // fallthrough
        case 6: last |= ((uint64_t)in[5]) << 40; // fallthrough
        case 5: last |= ((uint64_t)in[4]) << 32; // fallthrough
        case 4: last |= ((uint64_t)in[3]) << 24; // fallthrough
        case 3: last |= ((uint64_t)in[2]) << 16; // fallthrough
        case 2: last |= ((uint64_t)in[1]) << 8;  // fallthrough
        case 1: last |= ((uint64_t)in[0]);
    }

    printf("\n--- Ultimo Bloco / Resto (left: %zu, last: %016lx) ---\n", left, last);
    b |= last;
    v3 ^= b;
    print_state("Apos v3 ^= b", v0, v1, v2, v3);
    
    SIPROUND_DEBUG("Resto Round 1");
    SIPROUND_DEBUG("Resto Round 2");
    
    v0 ^= b;
    print_state("Apos v0 ^= b", v0, v1, v2, v3);

    // Finalização
    printf("\n--- Finalizacao (4 rounds) ---\n");
    v2 ^= 0xff;
    print_state("Apos v2 ^= 0xff", v0, v1, v2, v3);
    
    SIPROUND_DEBUG("Final Round 1");
    SIPROUND_DEBUG("Final Round 2");
    SIPROUND_DEBUG("Final Round 3");
    SIPROUND_DEBUG("Final Round 4");

    uint64_t final_hash = v0 ^ v1 ^ v2 ^ v3;
    printf("\n=== HASH FINAL: %016lx ===\n\n", final_hash);
    return final_hash;
}

// Main para teste rápido
int main() {
    const char* test_key = "cba";
    siphash24_debug(test_key, 3, 0x0123456789abcdefULL, 0xfedcba9876543210ULL);
    return 0;
}
