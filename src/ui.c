#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <pthread.h>
#include <time.h>

#include <gtk/gtk.h>

#include "callbacks.h"
#include "ui.h"
#include "general.h"

extern int running;

#define GLADE_FILE "salut-ui.glade"

int init_ui(void *arg)
{
	GtkBuilder *builder;
	gchar *filename;
	GError *error = NULL;

	Options *o = (Options *) arg;

	GtkWidget *window, *quit;
	GtkWidget *spin_energy;

	/* open glade file */
	builder = gtk_builder_new();
	filename = g_build_filename(GLADE_FILE, NULL);

	/* load glade file */
	gtk_builder_add_from_file(builder, filename, &error);
	g_free(filename);

	if (error) {
		gint code = error->code;
		g_printerr("%s\n", error->message);
		g_error_free(error);

		return code;
	}

	window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
	quit = GTK_WIDGET(gtk_builder_get_object(builder, "quit"));
	spin_energy = GTK_WIDGET(gtk_builder_get_object(builder, "spin_energy"));

	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(destroy_cb), NULL);
	g_signal_connect(G_OBJECT(quit), "activate", G_CALLBACK(destroy_cb), NULL);

	g_signal_connect(G_OBJECT(spin_energy), "value-changed",
			G_CALLBACK(spin_value_changed_cb), &o->energy_thresh);

	gtk_widget_show_all(window);

	return 0;
}

