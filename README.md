# UbuSnappy - Captura de Pantalla con GStreamer

UbuSnappy es una aplicación profesional de captura de pantalla para Linux, desarrollada con GTK3 y GStreamer. Detecta automáticamente tu entorno (Wayland o X11) y usa el método de captura más apropiado.

## Características

- Interfaz gráfica intuitiva con GTK3
- Captura de pantalla usando GStreamer (profesional y robusto)
- **Detección automática** de Wayland/X11
  - En **Wayland**: usa PipeWire (pipewiresrc)
  - En **X11**: usa XImageSrc (ximagesrc)
- Previsualización de la captura en la ventana
- Guardar capturas en formato PNG
- Nombres de archivo con timestamp automático
- Diálogos informativos para guiar al usuario

## Tecnologías

- **GTK+ 3.0**: Interfaz gráfica
- **GStreamer 1.0**: Motor de captura de video/pantalla
- **PipeWire**: Captura en Wayland (moderno y seguro)
- **XImageSrc**: Captura en X11 (clásico y confiable)

## Requisitos

- Sistema operativo Linux (compatible con Wayland y X11)
- GTK+ 3.0
- GStreamer 1.0 con plugins
- g++ (compilador C++)
- pkg-config

### Dependencias específicas por entorno:

**Para Wayland (GNOME por defecto):**
- pipewire
- gstreamer1.0-pipewire
- wireplumber

**Para X11:**
- gstreamer1.0-plugins-good (contiene ximagesrc)

## Instalación

### 1. Instalar dependencias

La forma más fácil es usar el comando del Makefile que detecta tu entorno automáticamente:

```bash
make install-deps
```

Esto instalará:
- Bibliotecas de desarrollo de GTK3
- GStreamer y sus plugins
- PipeWire (si estás en Wayland) o plugins de X11
- Herramientas de compilación

### Instalación manual (alternativa)

Si prefieres instalar manualmente:

```bash
# Dependencias básicas
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

# Si estás en Wayland (GNOME por defecto):
sudo apt-get install -y pipewire gstreamer1.0-pipewire wireplumber

# Si estás en X11:
# Los plugins ya están instalados con gstreamer1.0-plugins-good
```

### 2. Compilar

```bash
make
```

Esto generará el ejecutable `ubusnappy` en el directorio actual.

## Uso

### Ejecutar la aplicación

```bash
./ubusnappy
```

O usando make:

```bash
make run
```

Al iniciar, la aplicación te mostrará en la terminal:
- El entorno detectado (Wayland o X11)
- El motor de captura que se usará (PipeWire o XImageSrc)

### Interfaz

La aplicación muestra una ventana con:

1. **Área de previsualización**: En la parte superior, donde se mostrará la captura de pantalla (800x600 píxeles con scroll)
2. **Botón "📸 Tomar Captura"**: Captura la pantalla completa y la muestra en el área de previsualización
3. **Botón "💾 Guardar"**: Guarda la captura actual en el directorio `output/` con un nombre basado en la fecha y hora

### Cómo funciona

1. Al hacer clic en "Tomar Captura":
   - La ventana se oculta temporalmente (500ms)
   - GStreamer captura un frame de la pantalla usando PipeWire o XImageSrc
   - La imagen se muestra en la previsualización
   - La ventana vuelve a aparecer

2. Al hacer clic en "Guardar":
   - Se guarda la última captura en `output/screenshot_YYYYMMDD_HHMMSS.png`
   - Aparece un diálogo confirmando la ubicación

### Capturas guardadas

Las capturas se guardan en el directorio `output/` con el formato:
```
screenshot_YYYYMMDD_HHMMSS.png
```

Ejemplo: `screenshot_20251013_235930.png`

## Comandos del Makefile

- `make` o `make all`: Compila el proyecto
- `make run`: Compila y ejecuta la aplicación
- `make clean`: Elimina el ejecutable y las capturas guardadas
- `make install-deps`: Instala todas las dependencias necesarias (requiere sudo)

## Solución de problemas

### Error: "No se pudo crear el pipeline de GStreamer"

**Si estás en Wayland:**
```bash
sudo apt-get install gstreamer1.0-pipewire pipewire wireplumber
```

Asegúrate de que PipeWire esté corriendo:
```bash
systemctl --user status pipewire
```

Si no está corriendo, inícialo:
```bash
systemctl --user start pipewire
```

**Si estás en X11:**
```bash
sudo apt-get install gstreamer1.0-plugins-good
```

### Error de compilación: "Package gstreamer-1.0 was not found"

Instala las dependencias de desarrollo:
```bash
make install-deps
```

O manualmente:
```bash
sudo apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
```

### Verificar qué tipo de sesión estás usando

```bash
echo $XDG_SESSION_TYPE
```

Responderá `wayland` o `x11`.

### La captura tarda mucho o no funciona

1. Verifica que GStreamer pueda ver tus plugins:
   ```bash
   gst-inspect-1.0 pipewiresrc  # Para Wayland
   gst-inspect-1.0 ximagesrc    # Para X11
   ```

2. Prueba manualmente el pipeline:
   ```bash
   # Para Wayland
   gst-launch-1.0 pipewiresrc ! videoconvert ! autovideosink

   # Para X11
   gst-launch-1.0 ximagesrc ! videoconvert ! autovideosink
   ```

3. Si el pipeline manual funciona pero la app no, puede ser un problema de permisos o timing.

### Error: "No se pudo obtener un frame (timeout 3s)"

Esto puede ocurrir si:
- PipeWire no está configurado correctamente (Wayland)
- No tienes permisos para acceder al display (X11)
- El compositor está bloqueando la captura

**Solución para Wayland:**
```bash
# Reinicia PipeWire
systemctl --user restart pipewire
systemctl --user restart wireplumber
```

## Arquitectura técnica

### Pipeline de GStreamer

**Wayland:**
```
pipewiresrc → videoconvert → videoscale → appsink
```

**X11:**
```
ximagesrc → videoconvert → videoscale → appsink
```

### Flujo de datos:

1. **Source**: Captura pantalla (pipewiresrc o ximagesrc)
2. **videoconvert**: Convierte el formato de video a RGB
3. **videoscale**: Escala si es necesario
4. **appsink**: Entrega el frame a la aplicación como GstSample
5. **GdkPixbuf**: Convierte el buffer a pixbuf para mostrar en GTK

## Ventajas de usar GStreamer

- **Multiplataforma**: Funciona en Wayland y X11 automáticamente
- **Profesional**: Usado en aplicaciones comerciales y de streaming
- **Robusto**: Manejo de errores y formatos de video
- **Eficiente**: Captura directa sin archivos temporales
- **Extensible**: Fácil agregar grabación de video, efectos, etc.

## Licencia

Este proyecto es de código abierto y está disponible bajo una licencia permisiva.

## Contribuciones

Las contribuciones son bienvenidas. Por favor, abre un issue o pull request en el repositorio.

## Roadmap futuro

- [ ] Captura de región seleccionada
- [ ] Grabación de video
- [ ] Anotaciones sobre capturas
- [ ] Compartir directamente
- [ ] Atajos de teclado globales
