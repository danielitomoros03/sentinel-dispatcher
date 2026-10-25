#!/bin/sh
# =============================================================================
# run_tests.sh - Pruebas de sentinel-dispatcher (se corre con "make test").
#
#   1. Escenarios válidos: cada inputs/X.txt con su inputs/X.expected.txt.
#      - La salida estándar debe ser idéntica al .expected.txt.
#      - En modo verbose, los eventos (ARRIVAL, QUANTUM_EXPIRED, PREEMPTED,
#        COMPLETED) y las métricas deben ser los mismos que en modo estándar.
#   2. Entradas inválidas: cada inputs/invalid/X.txt debe terminar con error,
#      sin nada en stdout y con el mensaje de inputs/invalid/X.expected.txt.
#   3. Errores de línea de comandos: archivo inexistente, falta --input y
#      opción desconocida.
#
# Uso: sh tests/run_tests.sh [ruta-del-binario]   (desde la raíz del repo)
# =============================================================================

BIN=${1:-./sentinel-dispatcher}
pass=0
fail=0

tmp=$(mktemp -d) || exit 1
trap 'rm -rf "$tmp"' EXIT

ok()   { pass=$((pass + 1)); printf '  OK     %s\n' "$1"; }
fails() { fail=$((fail + 1)); printf '  FALLA  %s\n' "$1"; }

# Líneas de eventos del modo estándar.
events() { grep -E '^\[T-[0-9]+\] (ARRIVAL|QUANTUM_EXPIRED|PREEMPTED|COMPLETED):' "$1"; }

echo "Escenarios validos:"
for input in inputs/*.txt; do
    case "$input" in *.expected.txt) continue ;; esac
    expected="${input%.txt}.expected.txt"
    if [ ! -f "$expected" ]; then
        printf '  --     %s (sin .expected.txt, no se prueba)\n' "$input"
        continue
    fi

    # Modo estándar: idéntico al archivo esperado.
    if "$BIN" --input "$input" > "$tmp/out" 2> "$tmp/err" \
        && diff -u "$expected" "$tmp/out" > "$tmp/diff"; then
        ok "$input"
    else
        fails "$input"
        cat "$tmp/diff" "$tmp/err"
    fi

    # Modo verbose: mismos eventos y mismas métricas (últimas 6 líneas).
    if "$BIN" -v --input "$input" > "$tmp/vout" 2> "$tmp/err" \
        && events "$tmp/vout" > "$tmp/vev" && events "$expected" > "$tmp/eev" \
        && diff -u "$tmp/eev" "$tmp/vev" > "$tmp/diff" \
        && tail -n 6 "$tmp/vout" > "$tmp/vmet" && tail -n 6 "$expected" > "$tmp/emet" \
        && diff -u "$tmp/emet" "$tmp/vmet" >> "$tmp/diff"; then
        ok "$input (-v)"
    else
        fails "$input (-v)"
        cat "$tmp/diff" "$tmp/err"
    fi
done

echo "Entradas invalidas:"
for input in inputs/invalid/*.txt; do
    case "$input" in *.expected.txt) continue ;; esac
    expected="${input%.txt}.expected.txt"
    "$BIN" --input "$input" > "$tmp/out" 2> "$tmp/err"
    rc=$?
    if [ "$rc" -ne 0 ] && [ ! -s "$tmp/out" ] \
        && diff -u "$expected" "$tmp/err" > "$tmp/diff"; then
        ok "$input"
    else
        fails "$input (codigo $rc)"
        cat "$tmp/diff" "$tmp/out"
    fi
done

echo "Linea de comandos:"
# Cada caso debe terminar con código distinto de 0 y explicar el error en stderr.
check_cli() {
    desc=$1
    shift
    "$BIN" "$@" > "$tmp/out" 2> "$tmp/err"
    rc=$?
    if [ "$rc" -ne 0 ] && [ -s "$tmp/err" ]; then
        ok "$desc"
    else
        fails "$desc (codigo $rc)"
    fi
}
check_cli "archivo inexistente" --input inputs/no_existe.txt
check_cli "falta --input" -v
check_cli "opcion desconocida" --input inputs/sion_attack.txt --foo
check_cli "argumento suelto" --input inputs/sion_attack.txt extra

echo
echo "Resultado: $pass OK, $fail fallas"
[ "$fail" -eq 0 ]
