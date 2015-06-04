/*
 * List of timezones to be shown.
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
 * List of timezones to be shown.
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

#include "features.h"
#include "list.h"

#define LINE    (1 + MAX_TIMEZONE_LENGTH + 1 + MAX_LABEL_LENGTH + 1)

/** Update given timezone structure according to current time.
 * This function sets short and long time strings according to current time
 * in given timezone. It is supposed to be called from a loop once in a
 * second.
 *
 * @param t
 *      current time as returned by time().
 *
 * @param item
 *      timezone structure to be updated.
 *
 * @param options
 *      plugin options.
 *
 * @return
 *      nothing.
 */
static void tz_item_update(time_t t,
                           struct tz_item *item,
                           struct tz_options *options);


static FILE *
tz_list_file(const char *mode)
{
    gchar *filename;
    FILE *file = NULL;

    filename = g_build_path(G_DIR_SEPARATOR_S,
                            gkrellm_homedir(),
                            GKRELLM_DATA_DIR,
                            "gkrellm-tz",
                            NULL);

    if (filename != NULL)
        file = fopen(filename, mode);

    return file;
}


void
tz_list_load(struct tz_plugin *plugin)
{
    FILE *file;
    char line[LINE + 1];
    char *tz;
    char *lbl;
    int enabled;
    int i;
    int len;

    if ((file = tz_list_file("r")) == NULL)
        return;

    while (fgets(line, LINE, file) != NULL) {
        len = strlen(line);

        for (i = 0; i < MAX_TIMEZONE_LENGTH && line[i] != ':'; i++)
            ;
        line[i] = '\0';

        switch (*line) {
        case '-':
            enabled = 0;
            tz = line + 1;
            break;
        case '+':
            enabled = 1;
            tz = line + 1;
            break;
        default:
            enabled = 1;
            tz = line;
        }
        lbl = line + i + 1;

        if (line[len - 1] != '\n') {
            tz_list_add(plugin, enabled, lbl, tz);

            /* ignore rest of the line */
            while (fgets(line, LINE, file) != NULL) {
                len = strlen(line);
                if (line[len - 1] == '\n')
                    break;
            }
        } else {
            line[len - 1] = '\0';
            tz_list_add(plugin, enabled, lbl, tz);
        }
    }

    fclose(file);
}


void
tz_list_store(struct tz_plugin *plugin)
{
    FILE *file;
    struct tz_list_item *item;

    if ((file = tz_list_file("w")) == NULL)
        return;

    for (item = plugin->first; item != NULL; item = item->next)
        fprintf(file, "%c%s:%s\n",
                (item->tz.enabled) ? '+' : '-',
                item->tz.timezone,
                item->tz.label);

    fclose(file);
}


void
tz_plugin_update(struct tz_plugin *plugin)
{
    struct tz_list_item *item;
    gint wdecl;
    gint hdecl;
    gint wtext;
    gint offset;
    gint h;

    for (item = plugin->first; item != NULL; item = item->next) {
        if (!item->tz.enabled)
            continue;

        if (strchr(item->tz.time_short, '<') != NULL
            && !pango_parse_markup(item->tz.time_short, -1, 0,
                                   NULL, NULL, NULL, NULL))
            continue;

        offset = 0;
        if (plugin->options.align != TA_LEFT) {
            gkrellm_decal_get_size(item->decal, &wdecl, &hdecl);
            gkrellm_text_markup_extents(item->decal->text_style.font,
                                        item->tz.time_short,
                                        strlen(item->tz.time_short),
                                        &wtext, &h, NULL,
                                        &item->decal->y_ink);
            wtext += item->decal->text_style.effect;

            if (wtext < wdecl) {
                switch (plugin->options.align) {
                case TA_CENTER:
                    offset = (wdecl - wtext) / 2;
                    break;

                case TA_RIGHT:
                    offset = wdecl - wtext;
                    break;

                default:
                    offset = 0;
                }
            }
        }

        gkrellm_decal_text_set_offset(item->decal, offset, 0);
        gkrellm_draw_decal_markup(item->panel, item->decal,
                                  item->tz.time_short);
        gkrellm_draw_panel_layers(item->panel);
    }
}


void
tz_list_update(struct tz_plugin *plugin, time_t t)
{
    struct tz_list_item *item;

    for (item = plugin->first; item != NULL; item = item->next) {
        if (item->tz.enabled) {
            gchar *tmp;
            gchar *tt;

            tz_item_update(t, &item->tz, &plugin->options);

            tmp = g_strdup_printf("%s: %s",
                                  item->tz.label,
                                  item->tz.time_long);
            tt = g_locale_to_utf8(tmp, strlen(tmp), NULL, NULL, NULL);
            g_free(tmp);

#if TOOLTIP_API
            gtk_widget_set_tooltip_text(item->panel->drawing_area, tt);
#else
            gtk_tooltips_set_tip(plugin->tooltips,
                                 item->panel->drawing_area,
                                 tt, NULL);
#endif
            g_free(tt);
        }
    }
}


void
tz_list_clean(struct tz_plugin *plugin)
{
    struct tz_list_item *item;
    struct tz_list_item *p;

    for (item = plugin->first; item != NULL; ) {
        if (item->tz.enabled)
            gkrellm_panel_destroy(item->panel);
        free(item->tz.label);
        free(item->tz.timezone);
        p = item->next;
        free(item);
        item = p;
    }

    plugin->first = NULL;
    plugin->last = NULL;
}


int
tz_list_add(struct tz_plugin *plugin,
            int enabled,
            const char *label,
            const char *timezone)
{
    struct tz_list_item *item;

    if (timezone == NULL || *timezone == '\0')
        return -1;

    if (label == NULL)
        label = timezone;

    for (item = plugin->first; item != NULL; item = item->next) {
        if (strcmp(item->tz.label, label) == 0)
            return -1;
    }

    item = (struct tz_list_item *) malloc(sizeof(struct tz_list_item));
    if (item == NULL)
        return -1;

    memset((void *) item, '\0', sizeof(struct tz_list_item));
    item->tz.enabled = enabled;
    item->tz.label = strdup(label);
    item->tz.timezone = strdup(timezone);

    if (enabled) {
        item->panel = gkrellm_panel_new0();

        tz_panel_create(plugin, item);

        g_signal_connect(G_OBJECT(item->panel->drawing_area),
                         "expose_event",
                         G_CALLBACK(plugin->expose_event), NULL);
        g_signal_connect(G_OBJECT(item->panel->drawing_area),
                         "button_press_event",
                         G_CALLBACK(plugin->click_event), NULL);
    } else {
        item->panel = NULL;
    }

    item->prev = plugin->last;
    plugin->last = item;
    if (item->prev == NULL)
        plugin->first = item;
    else
        item->prev->next = item;

    return 0;
}


void
tz_panel_create(struct tz_plugin *plugin, struct tz_list_item *item)
{
    GkrellmStyle *style;
    GkrellmTextstyle *text_style;

    style = gkrellm_meter_style(plugin->style_id);
    text_style = gkrellm_meter_alt_textstyle(plugin->style_id);

    item->panel->textstyle = text_style;
    item->decal = gkrellm_create_decal_text(item->panel, "Yq", text_style,
                                            style, -1, -1, -1);

    gkrellm_panel_configure(item->panel, NULL, style);
    gkrellm_panel_create(plugin->vbox, plugin->monitor, item->panel);
}


static void
tz_item_update(time_t t,
               struct tz_item *item,
               struct tz_options *options)
{
    struct tm tm;
    char *tz_old;

    tz_old = getenv("TZ");
    setenv("TZ", item->timezone, 1);
    tzset();

    localtime_r(&t, &tm);
    strftime(item->time_short, TZ_SHORT, tz_format_short(*options), &tm);
    strftime(item->time_long, TZ_LONG, tz_format_long(*options), &tm);

    if (tz_old != NULL)
        setenv("TZ", tz_old, 1);
    else
        unsetenv("TZ");
    tzset();
}
