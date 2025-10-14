#include <gtk/gtk.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <string>
#include <iostream>
#include <ctime>
#include <cstdlib>
#include <unistd.h>

// Variables globales
GtkWidget *image_widget = nullptr;
GtkWidget *main_window = nullptr;
GdkPixbuf *current_pixbuf = nullptr;

// Función para detectar el entorno (Wayland o X11)
bool is_wayland() {
    const char* session_type = g_getenv("XDG_SESSION_TYPE");
    return (session_type && std::string(session_type) == "wayland");
}

// Función para capturar usando método de sistema (más confiable en Wayland)
GdkPixbuf* capture_screenshot_system() {
    const char* temp_file = "/tmp/ubusnappy_capture.png";

    // Eliminar archivo temporal si existe
    unlink(temp_file);

    std::string command;

    if (is_wayland()) {
        std::cout << "Usando gnome-screenshot (método nativo para Wayland)..." << std::endl;
        // Usar gnome-screenshot que tiene permisos nativos en GNOME
        command = "gnome-screenshot -f ";
        command += temp_file;
        command += " 2>/dev/null";
    } else {
        std::cout << "Usando import de ImageMagick (método para X11)..." << std::endl;
        command = "import -window root ";
        command += temp_file;
        command += " 2>/dev/null";
    }

    int result = system(command.c_str());

    if (result != 0 || access(temp_file, F_OK) != 0) {
        std::cerr << "Error: No se pudo capturar con herramienta del sistema" << std::endl;
        return nullptr;
    }

    // Cargar la imagen con GdkPixbuf
    GError *error = nullptr;
    GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file(temp_file, &error);

    if (!pixbuf) {
        std::cerr << "Error al cargar la captura: "
                  << (error ? error->message : "desconocido") << std::endl;
        if (error) g_error_free(error);
        return nullptr;
    }

    // Eliminar archivo temporal
    unlink(temp_file);

    std::cout << "Captura completada!" << std::endl;
    return pixbuf;
}

// Función para capturar la pantalla usando GStreamer (solo para X11)
GdkPixbuf* capture_screenshot_gstreamer() {
    GstElement *pipeline = nullptr;
    GstElement *sink = nullptr;
    GstSample *sample = nullptr;
    GdkPixbuf *pixbuf = nullptr;

    std::cout << "Inicializando captura con GStreamer + ximagesrc..." << std::endl;

    // Pipeline para X11
    pipeline = gst_parse_launch(
        "ximagesrc use-damage=false show-pointer=true ! "
        "video/x-raw ! videoconvert ! videoscale ! "
        "video/x-raw,format=RGB ! appsink name=sink max-buffers=1 drop=true",
        nullptr
    );

    if (!pipeline) {
        std::cerr << "Error: No se pudo crear el pipeline de GStreamer" << std::endl;
        return nullptr;
    }

    // Obtener el appsink
    sink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    if (!sink) {
        std::cerr << "Error: No se pudo obtener el sink" << std::endl;
        gst_object_unref(pipeline);
        return nullptr;
    }

    // Iniciar el pipeline
    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Error: No se pudo iniciar el pipeline" << std::endl;
        gst_object_unref(sink);
        gst_object_unref(pipeline);
        return nullptr;
    }

    // Esperar a que el pipeline esté listo
    gst_element_get_state(pipeline, nullptr, nullptr, GST_SECOND);

    // Esperar a que haya un frame disponible
    std::cout << "Esperando frame..." << std::endl;
    sample = gst_app_sink_try_pull_sample(GST_APP_SINK(sink), GST_SECOND * 3);

    if (!sample) {
        std::cerr << "Error: No se pudo obtener un frame (timeout 3s)" << std::endl;
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(sink);
        gst_object_unref(pipeline);
        return nullptr;
    }

    std::cout << "Frame capturado, procesando..." << std::endl;

    // Obtener el buffer y la información del video
    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstCaps *caps = gst_sample_get_caps(sample);
    GstStructure *structure = gst_caps_get_structure(caps, 0);

    int width, height;
    gst_structure_get_int(structure, "width", &width);
    gst_structure_get_int(structure, "height", &height);

    std::cout << "Resolución: " << width << "x" << height << std::endl;

    // Mapear el buffer
    GstMapInfo map;
    if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        // Crear GdkPixbuf desde los datos
        pixbuf = gdk_pixbuf_new_from_data(
            map.data,
            GDK_COLORSPACE_RGB,
            FALSE,  // no alpha
            8,      // bits per sample
            width,
            height,
            width * 3,  // rowstride (RGB = 3 bytes por pixel)
            nullptr,
            nullptr
        );

        // Copiar el pixbuf para que los datos no dependan del buffer
        if (pixbuf) {
            GdkPixbuf *copy = gdk_pixbuf_copy(pixbuf);
            g_object_unref(pixbuf);
            pixbuf = copy;
        }

        gst_buffer_unmap(buffer, &map);
    }

    // Limpiar
    gst_sample_unref(sample);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(sink);
    gst_object_unref(pipeline);

    std::cout << "Captura completada con GStreamer!" << std::endl;

    return pixbuf;
}

// Función principal de captura que elige el método apropiado
GdkPixbuf* capture_screenshot() {
    if (is_wayland()) {
        // En Wayland, usar gnome-screenshot (tiene permisos nativos)
        return capture_screenshot_system();
    } else {
        // En X11, intentar GStreamer primero, fallback a sistema
        GdkPixbuf* result = capture_screenshot_gstreamer();
        if (!result) {
            std::cout << "GStreamer falló, intentando método de sistema..." << std::endl;
            result = capture_screenshot_system();
        }
        return result;
    }
}

// Función que hace la captura después del timeout
gboolean do_capture(gpointer data) {
    std::cout << "Iniciando captura..." << std::endl;

    // Capturar pantalla
    current_pixbuf = capture_screenshot();

    if (current_pixbuf) {
        // Escalar la imagen para que quepa en la ventana (máximo 800x600)
        int orig_width = gdk_pixbuf_get_width(current_pixbuf);
        int orig_height = gdk_pixbuf_get_height(current_pixbuf);

        int new_width = 800;
        int new_height = 600;

        // Calcular el ratio para mantener la proporción
        float ratio = std::min(
            (float)new_width / orig_width,
            (float)new_height / orig_height
        );

        new_width = orig_width * ratio;
        new_height = orig_height * ratio;

        GdkPixbuf* scaled_pixbuf = gdk_pixbuf_scale_simple(
            current_pixbuf,
            new_width,
            new_height,
            GDK_INTERP_BILINEAR
        );

        // Actualizar la imagen en la interfaz
        gtk_image_set_from_pixbuf(GTK_IMAGE(image_widget), scaled_pixbuf);

        g_object_unref(scaled_pixbuf);

        std::cout << "¡Captura mostrada en la interfaz!" << std::endl;
    } else {
        std::cerr << "Error: No se pudo capturar la pantalla" << std::endl;

        // Mostrar mensaje de error en la UI
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(main_window),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            "No se pudo capturar la pantalla"
        );

        gtk_message_dialog_format_secondary_text(
            GTK_MESSAGE_DIALOG(dialog),
            "Asegúrate de tener instalado:\n"
            "- gstreamer1.0-plugins-good (contiene ximagesrc)\n"
            "- xorg-server (para XWayland en Wayland)\n\n"
            "Instala con: sudo apt install gstreamer1.0-plugins-good"
        );

        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }

    // Restaurar y mostrar la ventana
    gtk_window_deiconify(GTK_WINDOW(main_window));
    gtk_window_present(GTK_WINDOW(main_window));

    return FALSE; // No repetir el timeout
}

// Callback cuando se presiona el botón
void on_capture_button_clicked(GtkWidget *widget, gpointer data) {
    (void)widget; // Suprimir warning
    (void)data;

    std::cout << "\n=== Capturando pantalla ===" << std::endl;

    // Liberar el pixbuf anterior si existe
    if (current_pixbuf) {
        g_object_unref(current_pixbuf);
        current_pixbuf = nullptr;
    }

    // Minimizar la ventana (iconify es mejor que hide para XWayland)
    gtk_window_iconify(GTK_WINDOW(main_window));

    // Forzar el procesamiento de eventos pendientes
    while (gtk_events_pending()) {
        gtk_main_iteration();
    }

    // Esperar 1500ms antes de capturar (dar tiempo al compositor)
    g_timeout_add(1500, do_capture, nullptr);
}

// Callback para guardar la imagen
void on_save_button_clicked(GtkWidget *widget, gpointer data) {
    (void)widget; // Suprimir warning
    (void)data;

    if (!current_pixbuf) {
        std::cout << "No hay captura para guardar. Toma una captura primero.\n";

        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(main_window),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_WARNING,
            GTK_BUTTONS_OK,
            "No hay captura para guardar"
        );
        gtk_message_dialog_format_secondary_text(
            GTK_MESSAGE_DIALOG(dialog),
            "Primero toma una captura de pantalla usando el botón 'Tomar Captura'."
        );
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }

    // Generar nombre de archivo con timestamp
    time_t now = time(nullptr);
    char filename[256];
    strftime(filename, sizeof(filename), "output/screenshot_%Y%m%d_%H%M%S.png", localtime(&now));

    // Guardar el pixbuf como PNG
    GError *error = nullptr;
    if (gdk_pixbuf_save(current_pixbuf, filename, "png", &error, NULL)) {
        std::cout << "Captura guardada como: " << filename << "\n";

        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(main_window),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_INFO,
            GTK_BUTTONS_OK,
            "Captura guardada"
        );
        gtk_message_dialog_format_secondary_text(
            GTK_MESSAGE_DIALOG(dialog),
            "Guardado como: %s",
            filename
        );
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    } else {
        std::cerr << "Error al guardar: " << error->message << "\n";
        g_error_free(error);
    }
}

int main(int argc, char *argv[]) {
    // Inicializar GStreamer
    gst_init(&argc, &argv);

    // Inicializar GTK
    gtk_init(&argc, &argv);

    std::cout << "=== UbuSnappy - Captura de Pantalla ===" << std::endl;
    std::cout << "Entorno: " << (is_wayland() ? "Wayland (con XWayland)" : "X11") << std::endl;
    std::cout << "Motor de captura: GStreamer + XImageSrc" << std::endl;
    std::cout << "========================================\n" << std::endl;

    // Crear ventana principal
    main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(main_window), "UbuSnappy - Captura de Pantalla");
    gtk_window_set_default_size(GTK_WINDOW(main_window), 850, 700);
    gtk_container_set_border_width(GTK_CONTAINER(main_window), 10);
    g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Crear un contenedor vertical
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(main_window), vbox);

    // Crear un marco para la imagen con scroll
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolled_window, 800, 600);

    // Crear widget de imagen
    image_widget = gtk_image_new_from_icon_name("image-x-generic", GTK_ICON_SIZE_DIALOG);
    gtk_container_add(GTK_CONTAINER(scrolled_window), image_widget);

    // Agregar el marco con scroll al contenedor
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

    // Crear contenedor horizontal para los botones
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    // Crear botón de captura
    GtkWidget *capture_button = gtk_button_new_with_label("📸 Tomar Captura");
    gtk_widget_set_size_request(capture_button, 200, 40);
    g_signal_connect(capture_button, "clicked", G_CALLBACK(on_capture_button_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), capture_button, TRUE, TRUE, 0);

    // Crear botón de guardar
    GtkWidget *save_button = gtk_button_new_with_label("💾 Guardar");
    gtk_widget_set_size_request(save_button, 200, 40);
    g_signal_connect(save_button, "clicked", G_CALLBACK(on_save_button_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), save_button, TRUE, TRUE, 0);

    // Mostrar todos los widgets
    gtk_widget_show_all(main_window);

    // Iniciar el loop principal de GTK
    gtk_main();

    // Limpiar
    if (current_pixbuf) {
        g_object_unref(current_pixbuf);
    }

    return 0;
}
