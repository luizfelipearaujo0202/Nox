#!/usr/bin/env bash
set -euo pipefail

# Diretório raiz do projeto (onde este script está localizado)
ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

# ------------------------------------------------------------
# 1️⃣ Corrige typo da macro de debug‑tree
# ------------------------------------------------------------
sed -i '147s/##__VAR_ARGS__/##__VA_ARGS__/' "${ROOT}/globals.h"

# ------------------------------------------------------------
# 2️⃣ Insere auto‑macros após a definição das _NOX_* existentes
# ------------------------------------------------------------
awk '
/^#define _NOX_REALLOC/ {
    print;
    print "";
    print "/* AUTO‑MACROS: já trazem o contexto global */";
    print "#define _NOX_ALLOC_AUTO(sz)      _NOX_ALLOC(_NOX_MEM_CTX, (sz))";
    print "#define _NOX_FREE_AUTO(p)        _NOX_FREE(_NOX_MEM_CTX, (p))";
    print "#define _NOX_REALLOC_AUTO(p,sz) _NOX_REALLOC(_NOX_MEM_CTX, (p), (sz))";
    next
}
{print}' "${ROOT}/globals.h" > "${ROOT}/globals.h.tmp" && mv "${ROOT}/globals.h.tmp" "${ROOT}/globals.h"

# ------------------------------------------------------------
# 3️⃣ Silencia parâmetros não usados nas funções de mem_logger
# ------------------------------------------------------------
for fn in free alloc realloc; do
    sed -i "/static inline void[[:space:]]*__NoxMem_${fn}(/,+5s/{/{\\n    (void)c; (void)reg;/" "${ROOT}/mem_logger.h"
done

# ------------------------------------------------------------
# 4️⃣ Adiciona placeholder \"\" nas chamadas de _NOX_DEBUG_LOG sem variadic args
# ------------------------------------------------------------
find "${ROOT}" -type f \( -name '*.h' -o -name '*.c' \) | while read -r f; do
    grep -nE '_NOX_DEBUG_LOG\([^,]+,[^,]+,[^,]+,"[^"]*"\)[[:space:]]*;' "$f" | cut -d: -f1 | while read -r l; do
        sed -i "${l}s/);/, \"\");/" "$f"
    done
done

# ------------------------------------------------------------
# 5️⃣ Substitui chamadas de alocação/liberação/realloc de 1/2 args por auto‑macros
# ------------------------------------------------------------
for macro in ALLOC FREE REALLOC; do
    find "${ROOT}" -type f \( -name '*.h' -o -name '*.c' \) | while read -r f; do
        grep -n "_NOX_${macro}([^,)]*)" "$f" | cut -d: -f1 | while read -r l; do
            sed -i "${l}s/_NOX_${macro}(/_NOX_${macro}_AUTO(/" "$f"
        done
    done
done

# ------------------------------------------------------------
# 6️⃣ Corrige função sem retorno em parser/parser.h (linha 1547)
# ------------------------------------------------------------
sed -i '1547i\    return NULL;' "${ROOT}/parser/parser.h"

echo "✅ Concluído! Revise os diff’s com 'git diff' e compile novamente."
