#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include "callbacks.h"
#include "general.h"

extern int running;

void on_quit(void)
{
	running = 0;

	fprintf(stderr, "Exiting...\n");
}

void destroy_cb(GtkWidget *widget, gpointer data)
{
	(void)(widget);
	(void)(data);

	on_quit();

	gtk_main_quit();
}

void spin_value_changed_cb(GtkWidget *widget, gpointer data)
{
	float *val = (float *) data;

	*val = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));

	fprintf(stderr, "\r[LOG] energy_thresh = %f", *val);
}
