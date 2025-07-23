#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/X.h>
#include <iostream>
#include <cstdlib>

int main() {
    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        std::cerr << "No se pudo abrir la pantalla X11.\n";
        return 1;
    }

    Window root = DefaultRootWindow(display);

    XWindowAttributes gwa;
    XGetWindowAttributes(display, root, &gwa);

    int width = gwa.width;
    int height = gwa.height;

    XImage* image = XGetImage(
        display, root, 
        0, 0, 
        width, height, 
        AllPlanes, ZPixmap
    );

    if (!image) {
        std::cerr << "Error: XGetImage devolvió NULL\n";
        XCloseDisplay(display);
        return 1;
    }

    // Guardar usando ImageMagick
    FILE* fp = fopen("captura.ppm", "wb");
    fprintf(fp, "P6\n%d %d\n255\n", width, height);
    fwrite(image->data, 1, width * height * 4, fp);
    fclose(fp);

    std::cout << "Captura guardada como captura.ppm\n";

    XDestroyImage(image);
    XCloseDisplay(display);

    // Convertir a PNG usando ImageMagick
    system("convert captura.ppm captura.png && rm captura.ppm");
    std::cout << "Guardado como captura.png\n";

    return 0;
}

