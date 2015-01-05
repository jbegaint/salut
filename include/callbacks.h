#ifndef _CALLBACKS_H_
#define _CALLBACKS_H_

void on_quit(void);
void destroy_cb(GtkWidget *widget, gpointer data);

void spin_value_changed_cb(GtkWidget *widget, gpointer data);

#endif
