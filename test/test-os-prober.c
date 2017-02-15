/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2017 Leslie Zhai <xiang.zhai@i-soft.com.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <string.h>
#include <glib.h>

gpointer osprober_routine(gpointer data)
{
    gboolean ret;
    gchar *out = NULL;
    gchar *err;
    int status;
    GError *error = NULL;
    gchar **tokens = NULL;
    gchar **line;
    gchar **ptr;
    gchar *part = NULL;
    gchar *name = NULL;
    gchar *shortname = NULL;
    int i;

    ret = g_spawn_command_line_sync("/usr/bin/os-prober", 
                                    &out, 
                                    &err, 
                                    &status, 
                                    &error);
    if (out) {
        tokens = g_strsplit(out, "\n", -1);
        if (tokens) {
            for (line = tokens; *line; line++) {
                if (strlen(*line) == 0) 
                    continue;
                tokens = g_strsplit(*line, ":", -1);
                if (tokens) {
                    for (ptr = tokens, i = 0; *ptr; ptr++, i++) {
                        if (i == 0)
                            part = g_strdup(*ptr);
                        else if (i == 1)
                            name = g_strdup(*ptr);
                        else if (i == 2)
                            shortname = g_strdup(*ptr);
                    }
                    g_print("Found %s (%s) at %s\n", name, shortname, part);
                    if (part) {
                        g_free(part);
                        part = NULL;
                    }
                    if (name) {
                        g_free(name);
                        name = NULL;
                    }
                    if (shortname) {
                        g_free(shortname);
                        shortname = NULL;
                    }
                    g_strfreev(tokens);
                    tokens = NULL;
                }
            }
            g_strfreev(tokens);
            tokens = NULL;
        }
    }

    if (error) {
        g_print("ERROR: %s\n", error->message);
        g_error_free(error);
        error = NULL;
    }

    return NULL;
}

int main(int argc, char *argv[]) 
{
    GThread *thread = g_thread_new(NULL, osprober_routine, NULL);
    g_thread_join(thread);
    thread = NULL;

    return 0;
}
