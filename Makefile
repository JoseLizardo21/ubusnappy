# Makefile para UbuSnappy con GStreamer

CC = g++
CFLAGS = -Wall -Wextra `pkg-config --cflags gtk+-3.0 gstreamer-1.0 gstreamer-app-1.0`
LDFLAGS = `pkg-config --libs gtk+-3.0 gstreamer-1.0 gstreamer-app-1.0`
TARGET = ubusnappy
SRC = main.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(SRC) -o $(TARGET) $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(TARGET)
	rm -f output/*.png

run: $(TARGET)
	./$(TARGET)

install-deps:
	@echo "Instalando dependencias de GStreamer y GTK..."
	sudo apt-get update
	sudo apt-get install -y \
		libgtk-3-dev \
		build-essential \
		pkg-config \
		libgstreamer1.0-dev \
		libgstreamer-plugins-base1.0-dev \
		gstreamer1.0-tools \
		gstreamer1.0-plugins-base \
		gstreamer1.0-plugins-good \
		gstreamer1.0-plugins-bad
	@echo ""
	@echo "Detectando tipo de sesión para instalar plugin específico..."
	@if [ "$$XDG_SESSION_TYPE" = "wayland" ]; then \
		echo "Sesión Wayland detectada. Instalando pipewire y gstreamer1.0-pipewire..."; \
		sudo apt-get install -y pipewire gstreamer1.0-pipewire wireplumber; \
	else \
		echo "Sesión X11 detectada. Los plugins necesarios ya están instalados."; \
	fi
	@echo ""
	@echo "¡Dependencias instaladas! Ahora ejecuta 'make' para compilar."

.PHONY: all clean run install-deps
