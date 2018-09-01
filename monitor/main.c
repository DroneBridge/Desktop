#include <stdio.h>
#include <gtk/gtk.h>

void on_main_window_destroy()
{
    gtk_main_quit();
}

int main(int argc, char *argv[])
{
    GError          *err = NULL;
    GtkBuilder      *builder;
    GtkWidget       *window;

    gtk_init(&argc, &argv);

    builder = gtk_builder_new();
    gtk_builder_add_from_file (builder, "glade/monitor_ui.glade", &err);
    if (err != NULL) {
        fprintf (stderr, "Unable to read file: %s\n", err->message);
        g_error_free(err);
        return 1;
    }

    window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
    g_signal_connect(window, "destroy", G_CALLBACK (on_main_window_destroy), NULL);

    g_object_unref(builder);

    gtk_widget_show(window);
    gtk_main();

    return 0;
}