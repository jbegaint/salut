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

#ifndef _CALLBACKS_H_
#define _CALLBACKS_H_

void on_quit(void);
void destroy_cb(GtkWidget *widget, gpointer data);

void spin_value_changed_cb(GtkWidget *widget, gpointer data);

#endif
