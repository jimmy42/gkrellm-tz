/*
 * Plugin configuration.
 * Copyright (C) 2005--2010 Jiri Denemark
 *
 * This file is part of gkrellm-tz.
 *
 * gkrellm-tz is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/** @file
 * Plugin configuration.
 * @author Jiri Denemark
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <gkrellm2/gkrellm.h>

#include "list.h"
#include "config.h"

static gchar about_text[] =
    "gkrellm-tz " VERSION "\n"
    "GKrellM Timezone Plugin\n"
    "\n"
    "Copyright (C) 2005--2014\n"
    "\tJiri Denemark <Jiri.Denemark@gmail.com>\n"
    "\tand contributors (see NEWS)\n"
    "http://mamuti.net/gkrellm/index.en.html\n"
    "\n"
    "Released under the GNU General Public Licence";

static gchar *info_text[] = {
    "<b>GKrellM Timezone Plugin\n",
    "\n",
    "This plugin displays current time in several configurable timezones.\n",
    "\n",
    "<b>Timezone Configuration\n",
    "<b>Checkbox\n",
    "\tSpecifies whether current time for a particular timezone will actually be shown or not.\n",
    "<b>Label\n",
    "\tA short description of a timezone. It is shown in a tooltip.\n",
    "<b>Timezone\n",
    "\tTimezone identification as can be found under /usr/share/zoneinfo/.\n",
    "\tFor example, \"Europe/Prague\" or \"UTC\"\n",
    "\n",
    "<b>Options Configuration\n",
    "<b>Custom time format\n",
    "\t\"Short\" format string is used for displaying time in panels.\n",
    "\t\"Long\" format string is used for tooltips.\n",
    "\tSee strftime(3) or date(1) man pages for format string specification.\n",
    "\n",
    "Configured timezones are stored in ~/.gkrellm2/data/gkrellm-tz file.\n"
};


static gchar *list_titles[] = { "", "Label", "Timezone" };

static struct tz_options options;
static GtkWidget *entry_label;
static GtkWidget *entry_tz;
static GtkWidget *toggle_12h;
static GtkWidget *toggle_sec;
static GtkWidget *label_short;
static GtkWidget *entry_short;
static GtkWidget *label_long;
static GtkWidget *entry_long;
static GtkTreeSelection *sel_row;
static GtkTreeIter sel_row_iter;
static GtkListStore *list_store;
static GtkTreeModel *treemodel;

static void tz_reset_entries(void);
static void tz_config_toggled(GtkCellRendererToggle *cell_renderer,
                              gchar *path,
                              gpointer user_data);
static void tz_config_selected(GtkTreeSelection *selection, gpointer data);
static void tz_config_add(GtkWidget *widget, gpointer data);
static void tz_config_set(GtkWidget *widget, gpointer data);
static void tz_config_delete(GtkWidget *widget, gpointer data);
static void tz_config_up(GtkWidget *widget, gpointer data);
static void tz_config_down(GtkWidget *widget, gpointer data);

/* options callbacks */
static void tz_config_op_12h(GtkToggleButton *toggle, gpointer data);
static void tz_config_op_seconds(GtkToggleButton *toggle, gpointer data);
static void tz_config_op_custom(GtkToggleButton *toggle, gpointer data);
static void tz_config_op_left(GtkToggleButton *toggle, gpointer data);
static void tz_config_op_center(GtkToggleButton *toggle, gpointer data);
static void tz_config_op_right(GtkToggleButton *toggle, gpointer data);


void
tz_config_apply(struct tz_plugin *plugin)
{
    GtkTreeIter iter;
    gboolean enabled;
    gchar *entry[2];
    struct tz_list_item *item;

    if (gtk_tree_model_get_iter_first(treemodel, &iter) == FALSE)
        return;

    do {
        gtk_tree_model_get(treemodel, &iter,
                           0, &enabled,
                           1, &entry[0],
                           2, &entry[1], -1);
        tz_list_add(plugin, enabled, entry[0], entry[1]);
    } while (gtk_tree_model_iter_next(treemodel, &iter) == TRUE);

    gtk_list_store_clear(list_store);
    for (item = plugin->first; item != NULL; item = item->next) {
        if (item->tz.enabled)
            enabled = TRUE;
        else
            enabled = FALSE;
        entry[0] = item->tz.label;
        entry[1] = item->tz.timezone;
        gtk_list_store_append(list_store, &iter);
        gtk_list_store_set(list_store, &iter,
                           0, enabled,
                           1, entry[0],
                           2, entry[1], -1);
    }

    if (plugin->options.format_short != NULL) {
        free(plugin->options.format_short);
        plugin->options.format_short = NULL;
    }
    if (plugin->options.format_long != NULL) {
        free(plugin->options.format_long);
        plugin->options.format_long = NULL;
    }

    plugin->options.twelve_hour = options.twelve_hour;
    plugin->options.seconds = options.seconds;
    plugin->options.custom = options.custom;

    if (options.custom) {
        plugin->options.format_short =
            strdup(gtk_entry_get_text(GTK_ENTRY(entry_short)));
        plugin->options.format_long =
            strdup(gtk_entry_get_text(GTK_ENTRY(entry_long)));
    }

    plugin->options.align = options.align;
}


static void
tz_config_tz_list(GtkWidget *vbox, struct tz_plugin *plugin)
{
    GtkWidget *scrolled;
    GtkWidget *tree;
    GtkTreeIter iter;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeSelection *select;
    gboolean enabled;
    gchar *buf[2];
    struct tz_list_item *item;

    scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
            GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);

    list_store = gtk_list_store_new(3, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING);
    for (item = plugin->first; item != NULL; item = item->next) {
        if (item->tz.enabled)
            enabled = TRUE;
        else
            enabled = FALSE;
        buf[0] = item->tz.label;
        buf[1] = item->tz.timezone;
        gtk_list_store_append(list_store, &iter);
        gtk_list_store_set(list_store, &iter,
                           0, enabled,
                           1, buf[0],
                           2, buf[1], -1);
    }

    treemodel = GTK_TREE_MODEL(list_store);
    tree = gtk_tree_view_new_with_model(treemodel);

    renderer = gtk_cell_renderer_toggle_new();
    column = gtk_tree_view_column_new_with_attributes(list_titles[0],
                    renderer, "active", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
    g_signal_connect(G_OBJECT(renderer), "toggled",
                     G_CALLBACK(tz_config_toggled), NULL);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(list_titles[1],
                    renderer, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(list_titles[2],
                    renderer, "text", 2, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

    select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
    gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);
    g_signal_connect(G_OBJECT(select), "changed",
                     G_CALLBACK(tz_config_selected), NULL);

    gtk_container_add(GTK_CONTAINER(scrolled), tree);
}


static void
tz_config_timezones(GtkWidget *vbox, struct tz_plugin *plugin)
{
    GtkWidget *label;
    GtkWidget *hbox;
    GtkWidget *button;
    GtkWidget *table;

    table = gtk_table_new(2, 2, FALSE);
    gtk_table_set_col_spacing(GTK_TABLE(table), 0, 5);
    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

    /* Label */
    label = gtk_label_new("Label");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_SHRINK, 0, 0);
    entry_label = gtk_entry_new_with_max_length(MAX_LABEL_LENGTH);
    gtk_table_attach_defaults(GTK_TABLE(table), entry_label, 1, 2, 0, 1);

    /* Timezone */
    label = gtk_label_new("Timezone");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_SHRINK, 0, 0);
    entry_tz = gtk_entry_new_with_max_length(MAX_TIMEZONE_LENGTH);
    gtk_table_attach_defaults(GTK_TABLE(table), entry_tz, 1, 2, 1, 2);

    /* Action buttons */
    hbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_START);
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(hbox), 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 5);

    button = gtk_button_new_from_stock(GTK_STOCK_ADD);
    g_signal_connect(G_OBJECT(button), "clicked",
                       G_CALLBACK(tz_config_add), NULL);
    gtk_container_add(GTK_CONTAINER(hbox), button);

    button = gtk_button_new_from_stock(GTK_STOCK_EDIT);
    g_signal_connect(G_OBJECT(button), "clicked",
                       G_CALLBACK(tz_config_set), NULL);
    gtk_container_add(GTK_CONTAINER(hbox), button);

    button = gtk_button_new_from_stock(GTK_STOCK_DELETE);
    g_signal_connect(G_OBJECT(button), "clicked",
                     G_CALLBACK(tz_config_delete), NULL);
    gtk_container_add(GTK_CONTAINER(hbox), button);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);
    tz_config_tz_list(vbox, plugin);

    vbox = gtk_vbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(vbox), GTK_BUTTONBOX_START);
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(vbox), 5);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, TRUE, 5);

    button = gtk_button_new_from_stock(GTK_STOCK_GO_UP);
    g_signal_connect(G_OBJECT(button), "clicked",
                     G_CALLBACK(tz_config_up), NULL);
    gtk_container_add(GTK_CONTAINER(vbox), button);

    button = gtk_button_new_from_stock(GTK_STOCK_GO_DOWN);
    g_signal_connect(G_OBJECT(button), "clicked",
                     G_CALLBACK(tz_config_down), NULL);
    gtk_container_add(GTK_CONTAINER(vbox), button);
}


static void
tz_config_options(GtkWidget *vbox, struct tz_plugin *plugin)
{
    GtkWidget *hbox;
    GtkWidget *bbox;
    GtkWidget *button;
    GtkWidget *label;
    GtkWidget *table;
    GtkWidget *entry;

    options = plugin->options;

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* Options checkboxes */
    bbox = gtk_vbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_START);
    gtk_box_pack_start(GTK_BOX(hbox), bbox, FALSE, FALSE, 0);

    button = gtk_check_button_new_with_label(
                    "Display 12 hour instead of 24 hour time");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
                                 options.twelve_hour);
    g_signal_connect(G_OBJECT(button), "toggled",
                     G_CALLBACK(tz_config_op_12h), NULL);
    gtk_container_add(GTK_CONTAINER(bbox), button);
    toggle_12h = button;

    button = gtk_check_button_new_with_label("Show seconds");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
                                 options.seconds);
    g_signal_connect(G_OBJECT(button), "toggled",
                     G_CALLBACK(tz_config_op_seconds), NULL);
    gtk_container_add(GTK_CONTAINER(bbox), button);
    toggle_sec = button;

    /* Custom time formats */
    button = gtk_check_button_new_with_label("Custom time format:");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
                                 options.custom);
    gtk_container_add(GTK_CONTAINER(bbox), button);

    table = gtk_table_new(2, 3, FALSE);
    gtk_table_set_col_spacing(GTK_TABLE(table), 0, 15);
    gtk_table_set_col_spacing(GTK_TABLE(table), 1, 5);
    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 0);

    label = gtk_label_new("Short");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table), label, 1, 2, 0, 1, GTK_FILL, GTK_SHRINK, 0, 0);
    label_short = label;
    entry = gtk_entry_new_with_max_length(TZ_SHORT);
    gtk_table_attach_defaults(GTK_TABLE(table), entry, 2, 3, 0, 1);
    entry_short = entry;

    label = gtk_label_new("Long");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table), label, 1, 2, 1, 2, GTK_FILL, GTK_SHRINK, 0, 0);
    label_long = label;
    entry = gtk_entry_new_with_max_length(TZ_LONG);
    gtk_table_attach_defaults(GTK_TABLE(table), entry, 2, 3, 1, 2);
    entry_long = entry;

    g_signal_connect(G_OBJECT(button), "toggled",
                     G_CALLBACK(tz_config_op_custom), NULL);
    tz_config_op_custom(GTK_TOGGLE_BUTTON(button), NULL);

    options.format_short = NULL;
    options.format_long = NULL;

    /* Time alignment */
    hbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_START);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new("Time alignment:");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_container_add(GTK_CONTAINER(hbox), label);

    button = gtk_radio_button_new_with_label(NULL, "left");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
                                 options.align == TA_LEFT);
    g_signal_connect(G_OBJECT(button), "toggled",
                     G_CALLBACK(tz_config_op_left), NULL);
    gtk_container_add(GTK_CONTAINER(hbox), button);

    button = gtk_radio_button_new_with_label(
                    gtk_radio_button_get_group(GTK_RADIO_BUTTON(button)),
                    "center");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
                                 options.align == TA_CENTER);
    g_signal_connect(G_OBJECT(button), "toggled",
                     G_CALLBACK(tz_config_op_center), NULL);
    gtk_container_add(GTK_CONTAINER(hbox), button);

    button = gtk_radio_button_new_with_label(
                    gtk_radio_button_get_group(GTK_RADIO_BUTTON(button)),
                    "right");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
                                 options.align == TA_RIGHT);
    g_signal_connect(G_OBJECT(button), "toggled",
                     G_CALLBACK(tz_config_op_right), NULL);
    gtk_container_add(GTK_CONTAINER(hbox), button);
}


void
tz_config_create_tabs(GtkWidget *tab_vbox, struct tz_plugin *plugin)
{
    int i;
    GtkWidget *tabs;
    GtkWidget *label;
    GtkWidget *vbox;
    GtkWidget *text;

    tabs = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(tabs), GTK_POS_TOP);
    gtk_box_pack_start(GTK_BOX(tab_vbox), tabs, TRUE, TRUE, 0);

    /* Timezones configuration */
    vbox = gkrellm_gtk_framed_notebook_page(tabs, "Timezones");
    tz_config_timezones(vbox, plugin);

    /* Options */
    vbox = gkrellm_gtk_framed_notebook_page(tabs, "Options");
    tz_config_options(vbox, plugin);

    /* Info tab */
    vbox = gkrellm_gtk_framed_notebook_page(tabs, "Info");
    text = gkrellm_gtk_scrolled_text_view(vbox, NULL, GTK_POLICY_AUTOMATIC,
                                          GTK_POLICY_AUTOMATIC);
    for (i = 0; i < sizeof(info_text) / sizeof(gchar *); i++)
        gkrellm_gtk_text_view_append(text, info_text[i]);

    /* About tab */
    label = gtk_label_new("About");
    gtk_notebook_append_page(GTK_NOTEBOOK(tabs),
                             gtk_label_new(about_text), label);
}


static void
tz_reset_entries(void)
{
    gtk_entry_set_text(GTK_ENTRY(entry_label), "");
    gtk_entry_set_text(GTK_ENTRY(entry_tz), "");
}


static void
tz_config_toggled(GtkCellRendererToggle *cell_renderer,
                  gchar *path,
                  gpointer user_data)
{
    gboolean enabled;
    GtkTreeIter iter;

    gtk_tree_model_get_iter_from_string(treemodel, &iter, path);
    gtk_tree_model_get(treemodel, &iter, 0, &enabled, -1);
    gtk_list_store_set(list_store, &iter, 0, !enabled, -1);
}


static void
tz_config_selected(GtkTreeSelection *selection, gpointer data)
{
    GtkTreeModel *treemodel;
    GtkTreeIter iter;
    gchar *entry[2];

    if (gtk_tree_selection_get_selected(selection, &treemodel, &iter)) {
        gtk_tree_model_get(treemodel, &iter, 1, &entry[0], 2, &entry[1], -1);
        gtk_entry_set_text(GTK_ENTRY(entry_label), entry[0]);
        gtk_entry_set_text(GTK_ENTRY(entry_tz), entry[1]);
        g_free(entry[0]);
        g_free(entry[1]);

        sel_row_iter = iter;
        sel_row = selection;
    } else {
        tz_reset_entries();
    }
}


static void
tz_config_delete(GtkWidget *widget, gpointer data)
{
    tz_reset_entries();
    gtk_list_store_remove(list_store, &sel_row_iter);
}


static void
tz_config_add(GtkWidget *widget, gpointer data)
{
    gchar *entry[2];

    entry[0] = g_strdup(gkrellm_gtk_entry_get_text(&entry_label));
    g_strstrip(entry[0]);
    entry[1] = g_strdup(gkrellm_gtk_entry_get_text(&entry_tz));
    g_strstrip(entry[1]);

    if (strlen(entry[0]) == 0 || strlen(entry[1]) == 0)
        goto cleanup;

    gtk_list_store_append(list_store, &sel_row_iter);
    gtk_list_store_set(list_store, &sel_row_iter,
                       0, TRUE,
                       1, entry[0],
                       2, entry[1], -1);

cleanup:
    tz_reset_entries();

    g_free(entry[0]);
    g_free(entry[1]);
}


static void
tz_config_set(GtkWidget *widget, gpointer data)
{
    gchar *entry[2];

    entry[0] = g_strdup(gkrellm_gtk_entry_get_text(&entry_label));
    g_strstrip(entry[0]);
    entry[1] = g_strdup(gkrellm_gtk_entry_get_text(&entry_tz));
    g_strstrip(entry[1]);

    if (strlen(entry[0]) == 0 || strlen(entry[1]) == 0)
        goto cleanup;

    gtk_list_store_set(list_store, &sel_row_iter,
                       0, TRUE,
                       1, entry[0],
                       2, entry[1], -1);

cleanup:
    tz_reset_entries();

    g_free(entry[0]);
    g_free(entry[1]);
}


static void
tz_config_up(GtkWidget *widget, gpointer data)
{
    GtkTreeIter iter;
    GtkTreeIter iter2;

    if (GTK_IS_TREE_SELECTION(sel_row)
        && gtk_tree_selection_iter_is_selected(sel_row, &sel_row_iter)) {
        gtk_tree_model_get_iter_first(treemodel, &iter);

        if (!gtk_tree_selection_iter_is_selected(sel_row, &iter)) {
            while (!gtk_tree_selection_iter_is_selected(sel_row, &iter)) {
                iter2 = iter;
                gtk_tree_model_iter_next(treemodel, &iter);
            }
            iter = iter2;
            gtk_list_store_swap(list_store, &iter, &sel_row_iter);
        }
    }
}


static void
tz_config_down(GtkWidget *widget, gpointer data)
{
    GtkTreeIter iter;

    if (GTK_IS_TREE_SELECTION(sel_row)
        && gtk_tree_selection_iter_is_selected(sel_row, &sel_row_iter)) {
        iter = sel_row_iter;
        if (gtk_tree_model_iter_next(treemodel, &iter))
            gtk_list_store_swap(list_store, &iter, &sel_row_iter);
    }
}


static void
tz_config_op_12h(GtkToggleButton *toggle, gpointer data)
{
    options.twelve_hour = gtk_toggle_button_get_active(toggle);
}


static void
tz_config_op_seconds(GtkToggleButton *toggle, gpointer data)
{
    options.seconds = gtk_toggle_button_get_active(toggle);
}


static void
tz_config_op_custom(GtkToggleButton *toggle, gpointer data)
{
    options.custom = gtk_toggle_button_get_active(toggle);

    if (options.custom) {
        gtk_entry_set_text(GTK_ENTRY(entry_short), tz_format_short(options));
        gtk_entry_set_text(GTK_ENTRY(entry_long), tz_format_long(options));
    } else {
        gtk_entry_set_text(GTK_ENTRY(entry_short), "");
        gtk_entry_set_text(GTK_ENTRY(entry_long), "");
    }

    gtk_widget_set_sensitive(toggle_12h, !options.custom);
    gtk_widget_set_sensitive(toggle_sec, !options.custom);
    gtk_widget_set_sensitive(label_short, options.custom);
    gtk_widget_set_sensitive(entry_short, options.custom);
    gtk_widget_set_sensitive(label_long, options.custom);
    gtk_widget_set_sensitive(entry_long, options.custom);
}


static void
tz_config_op_left(GtkToggleButton *toggle, gpointer data)
{
    if (gtk_toggle_button_get_active(toggle))
        options.align = TA_LEFT;
}


static void
tz_config_op_center(GtkToggleButton *toggle, gpointer data)
{
    if (gtk_toggle_button_get_active(toggle))
        options.align = TA_CENTER;
}


static void
tz_config_op_right(GtkToggleButton *toggle, gpointer data)
{
    if (gtk_toggle_button_get_active(toggle))
        options.align = TA_RIGHT;
}
