/*
 * Timezone plugin for GKrellM.
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
 * Timezone plugin for GKrellM.
 * @author Jiri Denemark
 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gkrellm2/gkrellm.h>

#include "features.h"
#include "list.h"
#include "config.h"

#define CONFIG_TAB      "Timezone"
#define CONFIG_KEYWORD  "gkrellm-tz"
#define PLACEMENT       (MON_CLOCK | MON_INSERT_AFTER)


static struct tz_plugin plugin;


static gint
panel_expose_event(GtkWidget *widget, GdkEventExpose *ev)
{
    struct tz_list_item *item;

    for (item = plugin.first; item != NULL; item = item->next) {
        if (item->tz.enabled && widget == item->panel->drawing_area) {
            gdk_draw_pixmap(widget->window,
                            widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
                            item->panel->pixmap, ev->area.x, ev->area.y,
                            ev->area.x, ev->area.y, ev->area.width,
                            ev->area.height);
        }
    }

    return FALSE;
}


static void
panel_click_event(GtkWidget *widget,
                  GdkEventButton *ev,
                  gpointer data)
{
    if (ev->button == 3)
        gkrellm_open_config_window(plugin.monitor);
}


static void
update(void)
{
    if (gkrellm_ticks()->second_tick)
        tz_list_update(&plugin, mktime(gkrellm_get_current_time()));

    tz_plugin_update(&plugin);
}


static void
create(GtkWidget *vbox, gint first_create)
{
    if (first_create) {
        plugin.vbox = vbox;

        tz_list_clean(&plugin);
        tz_list_load(&plugin);
    } else {
        struct tz_list_item *item;

        for (item = plugin.first; item != NULL; item = item->next) {
            if (item->tz.enabled)
                tz_panel_create(&plugin, item);
        }
    }
}


static void
config(GtkWidget *tab_vbox)
{
    tz_config_create_tabs(tab_vbox, &plugin);
}


static void
apply(void)
{
    tz_list_clean(&plugin);
    tz_config_apply(&plugin);
    tz_list_store(&plugin);
}


static void
save(FILE *f)
{
    fprintf(f, "%s options %d %d %d %d\n",
            CONFIG_KEYWORD,
            plugin.options.twelve_hour,
            plugin.options.seconds,
            plugin.options.custom,
            plugin.options.align);

    fprintf(f, "%s format_short \"%s\"\n",
            CONFIG_KEYWORD,
            (plugin.options.custom) ? plugin.options.format_short : "");

    fprintf(f, "%s format_long \"%s\"\n",
            CONFIG_KEYWORD,
            (plugin.options.custom) ? plugin.options.format_long : "");
}


static char *
strdup_quoted(char *str)
{
    int len;

    if (str == NULL)
        return NULL;

    if (*str == '"')
        str++;

    len = strlen(str);
    if (str[len - 1] == '"')
        str[len - 1] = '\0';

    return strdup(str);
}

static void
load(gchar *line)
{
    char config[32];
    char value[CFG_BUFSIZE];

    if (sscanf(line, "%31s %[^\n]", config, value) != 2)
        return;

    if (strcmp(config, "options") == 0) {
        int twelve_hour;
        int seconds;
        int custom;
        int align;

        sscanf(value, "%d %d %d %d", &twelve_hour, &seconds, &custom, &align);
        plugin.options.twelve_hour = twelve_hour != 0;
        plugin.options.seconds = seconds != 0;
        plugin.options.custom = custom != 0;
        switch (align) {
        case TA_LEFT:   plugin.options.align = TA_LEFT; break;
        case TA_CENTER: plugin.options.align = TA_CENTER; break;
        case TA_RIGHT:  plugin.options.align = TA_RIGHT; break;
        default:        plugin.options.align = TA_LEFT; break;
        }
    } else if (strcmp(config, "format_short") == 0) {
        if (*value != '\0')
            plugin.options.format_short = strdup_quoted(value);
    } else if (strcmp(config, "format_long") == 0) {
        if (*value != '\0')
            plugin.options.format_long = strdup_quoted(value);
    }
}


static GkrellmMonitor plugin_mon = {
    CONFIG_TAB,                 /* Title for config tab. */
    0,                          /* Id,  0 if a plugin */
    create,                     /* The create function */
    update,                     /* The update function */
    config,                     /* The config tab create function */
    apply,                      /* Apply the config function */

    save,                       /* Save user config */
    load,                       /* Load user config */
    CONFIG_KEYWORD,             /* Config file keyword */

    NULL,                       /* Undefined 2 */
    NULL,                       /* Undefined 1 */
    NULL,                       /* Undefined 0 */

    PLACEMENT,                  /* Plugin placement */

    NULL,                       /* Handle if a plugin, filled in by GKrellM */
    NULL                        /* path if a plugin, filled in by GKrellM */
};


/** Plugin's entry point. */
GkrellmMonitor *
gkrellm_init_plugin(void)
{
    plugin.options.twelve_hour = 0;
    plugin.options.seconds = 1;
    plugin.options.custom = 0;
    plugin.options.format_short = NULL;
    plugin.options.format_long = NULL;
    plugin.options.align = TA_LEFT;
    plugin.first = NULL;
    plugin.last = NULL;
    plugin.vbox = NULL;
#if !TOOLTIP_API
    plugin.tooltips = gtk_tooltips_new();
    gtk_tooltips_enable(plugin.tooltips);
    gtk_tooltips_set_delay(plugin.tooltips, 500);
#endif
    plugin.monitor = &plugin_mon;
    plugin.expose_event = panel_expose_event;
    plugin.click_event = panel_click_event;
    plugin.style_id = gkrellm_add_meter_style(&plugin_mon, CONFIG_KEYWORD);

    return plugin.monitor;
}
