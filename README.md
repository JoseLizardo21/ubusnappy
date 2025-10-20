# UbuSnappy - Professional Screenshot Capture Tool

<div align="center">

**A modern, cross-platform screenshot application for Linux**

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-Linux-lightgrey.svg)
![GTK](https://img.shields.io/badge/GTK-3.0-green.svg)
![GStreamer](https://img.shields.io/badge/GStreamer-1.0-orange.svg)

</div>

---

## Overview

UbuSnappy is a professional screenshot capture application built with GTK3 and GStreamer. It automatically detects your display server (Wayland or X11) and uses the most appropriate capture method, ensuring reliable screenshots across different Linux desktop environments.

### Key Features

- **Universal Compatibility**: Works seamlessly on both Wayland and X11
- **Smart Detection**: Automatically chooses the optimal capture method
- **⚡ Multiple Backends**:
  - **Wayland**: Native GNOME Screenshot API
  - **X11**: GStreamer with XImageSrc pipeline
- **Live Preview**: Real-time preview of captured screenshots
- **Easy Export**: Save captures as PNG with automatic timestamping
- **Modern UI**: Clean, intuitive GTK3 interface
- **Professional Architecture**: Built on industry-standard frameworks

---

## Technology Stack

| Component | Purpose |
|-----------|---------|
| **GTK+ 3.0** | Modern graphical interface |
| **GStreamer 1.0** | Professional video/screen capture pipeline (X11) |
| **GNOME Screenshot** | Native capture API for Wayland |
| **GdkPixbuf** | Image processing and manipulation |


---

## System Requirements

### Minimum Requirements

- **OS**: Linux (any modern distribution)
- **Display Server**: X11 or Wayland
- **Desktop Environment**: Any (GNOME, KDE, XFCE, etc.)
- **Memory**: 50 MB RAM
- **Disk Space**: 10 MB

### Dependencies

#### Core Dependencies
- GTK+ 3.0 (`libgtk-3-dev`)
- GStreamer 1.0 (`libgstreamer1.0-dev`)
- GStreamer Plugins Base (`libgstreamer-plugins-base1.0-dev`)
- Build essentials (`build-essential`, `pkg-config`)

#### Display Server Specific

**For Wayland (GNOME, Sway, etc.):**
- `gnome-screenshot` - Native screenshot tool
- `pipewire` - Modern multimedia framework
- `gstreamer1.0-pipewire` - PipeWire GStreamer plugin
- `wireplumber` - PipeWire session manager

**For X11:**
- `gstreamer1.0-plugins-good` - Contains ximagesrc plugin

---

## Installation

### Quick Install (Recommended)

The Makefile includes an automated installer that detects your environment:

```bash
# Install all dependencies automatically
make install-deps

# Compile the application
make

# Run
./ubusnappy

---

## Usage

### Running the Application

```bash
# Direct execution
./ubusnappy

# Or via Makefile
make run

```

## Architecture

### Capture Flow Diagram

```
┌─────────────────────────────────────────────────────────┐
│                    User Interface (GTK3)                 │
└──────────────────────┬──────────────────────────────────┘
                       │
                       ▼
           ┌───────────────────────┐
           │  Capture Coordinator  │
           └───────────┬───────────┘
                       │
        ┌──────────────┴──────────────┐
        │                             │
        ▼                             ▼
┌───────────────┐            ┌────────────────┐
│   Wayland?    │            │     X11?       │
└───────┬───────┘            └────────┬───────┘
        │                             │
        ▼                             ▼
┌───────────────┐            ┌────────────────┐
│ GNOME Screen- │            │  GStreamer     │
│    shot API   │            │   Pipeline     │
└───────┬───────┘            └────────┬───────┘
        │                             │
        │                             │
        │      ┌──────────────┐       │
        └─────►│  GdkPixbuf   │◄──────┘
               │  Processing  │
               └──────┬───────┘
                      │
                      ▼
               ┌──────────────┐
               │ Preview/Save │
               └──────────────┘
```

### GStreamer Pipeline (X11 Mode)

```
ximagesrc → videoconvert → videoscale → appsink
    ↓            ↓              ↓           ↓
 Capture     Convert to     Scale if    Deliver to
  screen        RGB          needed     application
```

**Pipeline Elements:**
- `ximagesrc`: Captures X11 display content
- `videoconvert`: Converts to RGB format
- `videoscale`: Ensures proper dimensions
- `appsink`: Delivers frame to the application

---

## Makefile Commands

| Command | Description |
|---------|-------------|
| `make` or `make all` | Compile the application |
| `make run` | Compile and execute |
| `make clean` | Remove binary and screenshots |
| `make install-deps` | Install all dependencies (requires sudo) |

---

## Troubleshooting

### Issue: Application doesn't capture screenshots

**For Wayland users:**

1. Verify gnome-screenshot is installed:
   ```bash
   which gnome-screenshot
   ```

2. Install if missing:
   ```bash
   sudo apt-get install gnome-screenshot
   ```

**For X11 users:**

1. Verify GStreamer plugin:
   ```bash
   gst-inspect-1.0 ximagesrc
   ```

2. Install if missing:
   ```bash
   sudo apt-get install gstreamer1.0-plugins-good
   ```

### Issue: Black screen with only cursor visible

This typically happens when:
- The capture method doesn't have proper permissions
- The compositor is blocking screen capture
- XWayland compatibility issues

**Solution:**
```bash
# Install gnome-screenshot for native Wayland support
sudo apt-get install gnome-screenshot

# Recompile and run
make clean && make run
```

### Issue: Compilation errors

**Error**: `Package gtk+-3.0 was not found`

```bash
sudo apt-get install libgtk-3-dev
```

**Error**: `Package gstreamer-1.0 was not found`

```bash
sudo apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
```

**Complete fix:**
```bash
make install-deps
```

### Issue: How to check my display server type?

```bash
echo $XDG_SESSION_TYPE
```

Output will be either `wayland` or `x11`.

### Advanced: Testing GStreamer pipeline manually

**For X11:**
```bash
gst-launch-1.0 ximagesrc ! videoconvert ! autovideosink
```

**For Wayland (if pipewiresrc is configured):**
```bash
gst-launch-1.0 pipewiresrc ! videoconvert ! autovideosink
```

If manual pipeline works but the app doesn't, file an issue on GitHub.

---

## Development

### Building from Source

```bash
# Clone the repository (if from git)
git clone https://github.com/JoseLizardo21/ubusnappy.git
cd ubusnappy

# Install dependencies
make install-deps

# Build
make

# Test
./ubusnappy
```

## Contributing

Contributions are welcome! Please follow these guidelines:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Code Style

- Follow existing indentation (4 spaces)
- Comment complex logic
- Use meaningful variable names
- Test on both Wayland and X11 if possible

---

## License

This project is open source and available under the [MIT License](LICENSE).

```
MIT License

Copyright (c) 2025 UbuSnappy Contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software...
```

---

## Acknowledgments

Built with:
- [GTK+](https://www.gtk.org/) - The GIMP Toolkit
- [GStreamer](https://gstreamer.freedesktop.org/) - Multimedia framework
- [GNOME](https://www.gnome.org/) - Desktop environment
- [PipeWire](https://pipewire.org/) - Modern multimedia server

Inspired by tools like Flameshot, Spectacle, and GNOME Screenshot.

---

## Contact & Support

- **Issues**: [GitHub Issues](https://github.com/JoseLizardo21/ubusnappy/issues)
- **Discussions**: [GitHub Discussions](https://github.com/JoseLizardo21/ubusnappy/discussions)

---

<div align="center">

**Made with ❤️ for the Linux community**

[⬆ Back to Top](#ubusnappy---professional-screenshot-capture-tool)

</div>
