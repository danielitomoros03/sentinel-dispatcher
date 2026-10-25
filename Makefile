# =============================================================================
# Makefile de sentinel-dispatcher
# Proyecto 2 de Sistemas Operativos (6004, UCV, I-2026)
#
# Objetivos:
#   make            compila todo y produce el binario ./sentinel-dispatcher
#   make test       compila y corre todos los escenarios de inputs/
#   make zip CI="CI1 CI2 CI3"
#                   arma Proyecto2_CI1_CI2_CI3.zip para la entrega
#   make informe    genera docs/informe.pdf desde docs/informe.md
#                   (requiere pandoc y Chromium/Chrome; CHROME=<ruta>)
#   make clean      borra los objetos y el binario
# =============================================================================

# Compilador y banderas. -pthread va tanto al compilar como al enlazar.
# _POSIX_C_SOURCE=200809L habilita pthread_barrier_t y getline() en C99.
CC       := gcc
CFLAGS   ?= -O2
CFLAGS   += -std=c99 -Wall -Wextra -pedantic -pthread -D_POSIX_C_SOURCE=200809L
LDFLAGS  += -pthread

# Nombre del ejecutable y ubicación de fuentes y objetos.
BIN      := sentinel-dispatcher
SRC_DIR  := src
OBJ_DIR  := build
SRCS     := $(wildcard $(SRC_DIR)/*.c)
OBJS     := $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
DEPS     := $(OBJS:.o=.d)

# Archivos de la entrega (ver make zip y make informe).
INFORME  := docs/informe.pdf
CHROME   ?= chromium

.PHONY: all test zip informe clean

all: $(BIN)

# Enlace final del binario.
$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

# Compilación de cada .c a su .o. -MMD -MP generan las dependencias con los .h
# para que un cambio en una cabecera recompile lo necesario.
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

$(OBJ_DIR):
	mkdir -p $@

# Corre todos los escenarios de inputs/ contra sus salidas esperadas.
test: $(BIN)
	@sh tests/run_tests.sh ./$(BIN)

# Empaqueta la entrega: fuentes, Makefile, informe, escenarios y script de
# pruebas. Uso: make zip CI="30035269 CI2 CI3"
zip: $(BIN)
	@if [ -z "$(strip $(CI))" ]; then \
		echo 'Uso: make zip CI="CI1 CI2 CI3"'; exit 1; fi
	@if [ ! -f $(INFORME) ]; then \
		echo "Falta $(INFORME): genera el informe antes de empaquetar."; exit 1; fi
	@name="Proyecto2_$$(echo $(CI) | tr ' ' '_').zip"; \
	rm -f "$$name"; \
	zip -q "$$name" Makefile $(SRC_DIR)/*.c $(SRC_DIR)/*.h $(INFORME) \
		inputs/*.txt inputs/invalid/*.txt tests/run_tests.sh README.md && \
	echo "Creado $$name"

# Informe en PDF: pandoc pasa el Markdown a HTML y Chromium lo imprime a PDF.
informe: docs/informe.md docs/informe.css | $(OBJ_DIR)
	pandoc docs/informe.md -s --embed-resources --css docs/informe.css \
		-o $(OBJ_DIR)/informe.html
	$(CHROME) --headless --no-sandbox --disable-gpu --no-pdf-header-footer \
		--print-to-pdf=$(INFORME) $(OBJ_DIR)/informe.html
	@echo "Creado $(INFORME)"

clean:
	rm -rf $(OBJ_DIR) $(BIN)

-include $(DEPS)
