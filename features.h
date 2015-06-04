/*
 * Compile time features.
 * Copyright (C) 2010 Jiri Denemark
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

#ifndef FEATURES_H
#define FEATURES_H

#include <gtk/gtk.h>

#if (GKRELLM_VERSION_MAJOR < 2)
# error This plugin requires GKrellM version >= 2.0
#endif

#if GTK_CHECK_VERSION(2,12,0)
# define TOOLTIP_API 1
#else
# define TOOLTIP_API 0
#endif

#define GTK_DISABLE_DEPRECATED 1

#endif
