/*
 * Copyright (c) 2014 - 2015 Jean Bégaint
 *
 * This file is part of salut.
 *
 * salut is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * salut is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with salut.  If not, see <http://www.gnu.org/licenses/>.
 */

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

