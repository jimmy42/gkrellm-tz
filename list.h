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

#ifndef LIST_H
#define LIST_H

#include <time.h>


#define MAX_LABEL_LENGTH    60
#define MAX_TIMEZONE_LENGTH 60

/** Short time format (used for gkrellm panel) -- 24h+s. */
#define TZ_SHORT_FORMAT_24s "%T %Z"
/** Short time format (used for gkrellm panel) -- 24h-s. */
#define TZ_SHORT_FORMAT_24  "%R %Z"
/** Short time format (used for gkrellm panel) -- 12h+s. */
#define TZ_SHORT_FORMAT_12s "%r %Z"
/** Short time format (used for gkrellm panel) -- 12h-s. */
#define TZ_SHORT_FORMAT_12  "%I:%M %p %Z"
/** Length of a buffer for short time string. */
#define TZ_SHORT            255

/** Get short time format according to plugin options.
 *
 * @param options
 *      plugin options itself (the structure, not a pointer to it).
 *
 * @return
 *      short time format.
 */
#define tz_format_short(options)    \
    (((options).custom && (options).format_short != NULL)   \
        ? (options).format_short                            \
     : ((!(options).twelve_hour && (options).seconds)       \
        ? TZ_SHORT_FORMAT_24s                               \
     : ((!(options).twelve_hour && !(options).seconds)      \
        ? TZ_SHORT_FORMAT_24                                \
     : (((options).twelve_hour && (options).seconds)        \
        ? TZ_SHORT_FORMAT_12s                               \
        : TZ_SHORT_FORMAT_12))))


/** Long time format (used for tooltip message). */
#define TZ_LONG_FORMAT      "%c %Z (%z)"
/** Length of a buffer for long time string. */
#define TZ_LONG             100

/** Get long time format according to plugin options.
 *
 * @param options
 *      plugin options itself (the structure, not a pointer to it).
 *
 * @return
 *      long time format.
 */
#define tz_format_long(options)     \
    (((options).custom && (options).format_long != NULL)    \
        ? (options).format_long                             \
        : TZ_LONG_FORMAT)


/** Timezone structure. */
struct tz_item {
    /** Nonzero if enabled. */
    int enabled;
    /** Label to be shown for a given time.
     * In case it is NULL, timezone field is used as a label. */
    char *label;
    /** Timezone in a form usable for TZ environment variable.
     * E.g. "US/Central". */
    char *timezone;
    /** Buffer for short time string. */
    char time_short[TZ_SHORT];
    /** Buffer for long time string. */
    char time_long[TZ_LONG];
};


/** Timezone list item. */
struct tz_list_item {
    /** Pointer to the previous item in the list. */
    struct tz_list_item *prev;
    /** Pointer to the next item in the list. */
    struct tz_list_item *next;
    /** GKrellM panel dedicated for this timezone. */
    GkrellmPanel *panel;
    /** GKrellM decal containing short time string. */
    GkrellmDecal *decal;
    /** Timezone description and current time. */
    struct tz_item tz;
};


/** Text alignment. */
enum tz_align {
    TA_LEFT,
    TA_CENTER,
    TA_RIGHT
};


/** Plugin options. */
struct tz_options {
    /** 12 hour time instead of 24 hour time. */
    int twelve_hour;
    /** Show seconds in krells. */
    int seconds;
    /** Use custom formats. */
    int custom;
    /** Custom format for time in krells. */
    char *format_short;
    /** Custom format for time in tooltips. */
    char *format_long;
    /** Alignmet of the text in krells. */
    enum tz_align align;
};


/** Plugin data. */
struct tz_plugin {
    /** Plugin options. */
    struct tz_options options;
    /** Pointer to the first item in the list. */
    struct tz_list_item *first;
    /** Pointer to the last item in the list. */
    struct tz_list_item *last;
    /** Plugin's vbox. */
    GtkWidget *vbox;
    /** Plugin's description structure. */
    GkrellmMonitor *monitor;
#if !TOOLTIP_API
    /** Tooltips for labels and long time strings. */
    GtkTooltips *tooltips;
#endif
    /** Handler for expose_event. */
    gint (*expose_event)(GtkWidget *widget, GdkEventExpose *ev);
    /** Handler for button_press_event. */
    void (*click_event)(GtkWidget *widget, GdkEventButton *ev, gpointer data);
    /** Pointer to a panel style. */
    gint style_id;
};


void tz_plugin_update(struct tz_plugin *plugin);
void tz_panel_create(struct tz_plugin *plugin, struct tz_list_item *item);


void tz_list_load(struct tz_plugin *plugin);
void tz_list_store(struct tz_plugin *plugin);
void tz_list_clean(struct tz_plugin *plugin);
int tz_list_add(struct tz_plugin *plugin,
                int enabled,
                const char *label,
                const char *timezone);
void tz_list_update(struct tz_plugin *plugin, time_t t);
int tz_list_remove();
int tz_list_move_up();
int tz_list_move_down();

#endif
