/* Copyright (c) 2022 M.A.X. Port Team
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "file-max.h"

#include <errno.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <stdio.h>
#include <string.h>

#include "palette.h"

#define MAX_PLUGIN_VERSION "0.1"

#define LOAD_THUMB_PROC "file-max-load-thumb"
#define LOAD_PROC "file-max-load"
#define SAVE_PROC "file-max-save"
#define PLUG_IN_BINARY "file-max"
#define PLUG_IN_ROLE "gimp-file-max"

#define MAX_SAVE_GUI                                                                                                 \
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?><interface><requires lib=\"gtk+\" version=\"2.24\"/><!-- "            \
    "interface-naming-policy project-wide --><object class=\"GtkVBox\" id=\"vbox-frame\"><property "                 \
    "name=\"visible\">True</property><property name=\"border_width\">12</property><property "                        \
    "name=\"spacing\">12</property><child><object class=\"GimpFrame\" id=\"export\"><property "                      \
    "name=\"visible\">True</property><property name=\"label\" translatable=\"yes\">Select Image "                    \
    "Format</property><child><object class=\"GtkVBox\" id=\"vbox-format\"><property "                                \
    "name=\"visible\">True</property><property name=\"spacing\">2</property><child><object "                         \
    "class=\"GtkComboBoxText\" id=\"format-combo\"><property name=\"visible\">True</property><property "             \
    "name=\"can_focus\">True</property><items><item translatable=\"yes\">Automatic Format</item><item "              \
    "translatable=\"yes\">MAX Simple</item><item translatable=\"yes\">MAX Big</item><item translatable=\"yes\">MAX " \
    "Multi</item><item translatable=\"yes\">MAX "                                                                    \
    "Shadow</item></items></object></child></object></child></object></child></object></interface>"

#define PALETTE_COLORS 256
#define PALETTE_SIZE PALETTE_COLORS *(sizeof(guchar) + sizeof(guchar) + sizeof(guchar))
#define RLE_BREAK_EVEN (2 * sizeof(gint16) + sizeof(guchar))

struct MaxMultiImage {
    gint32 file_offset;
    gint16 width;
    gint16 height;
    gint16 hotx;
    gint16 hoty;
    gint32 *rows;
    guchar *pixels;
};

enum MaxFormatTypes {
    MAX_FORMAT_AUTO,
    MAX_FORMAT_SIMPLE,
    MAX_FORMAT_BIG,
    MAX_FORMAT_MULTI,
    MAX_FORMAT_SHADOW,
};

struct MaxPluginSettings {
    gint file_type;
    gint16 ulx;
    gint16 uly;
};

static void query(void);
static void run(const gchar *name, gint nparams, const GimpParam *param, gint *nreturn_vals, GimpParam **return_vals);
static gint32 load_thumbnail(const gchar *filename, gint *width, gint *height, GError **error);
static gboolean image_rle_decode(FILE *fd, gint data_size, guchar *pixels, gint width, gint height);
static gboolean image_rle_encode_emit(FILE *fd, guchar *buffer, gint size, gboolean repeat_mode);
static gboolean image_rle_encode(FILE *fd, guchar *buffer, gint rows, gint rowstride);
static gint32 load_image(const gchar *filename, GError **error);
static gint32 load_max_simple(FILE *fd, GError **error);
static gint32 load_max_big(FILE *fd, gint file_size, GError **error);
static gboolean decode_max_multi_image(FILE *fd, struct MaxMultiImage *image);
static gboolean decode_max_multi_shadow(FILE *fd, struct MaxMultiImage *image);
static gpointer read_max_multi_image(FILE *fd, guint32 address);
static gint32 load_max_multi(FILE *fd, GError **error);
static void on_dialog_response(GtkWidget *widget, gint response_id, gpointer data);
static gboolean save_dialog(gint32 image_ID, GError **error);
static GimpPDBStatusType save_image(const gchar *filename, gint32 image, gint32 drawable_ID, GimpRunMode run_mode,
                                    GError **error);
// static gint32 save_max_simple(FILE *fd, GError **error);
// static gint32 save_max_big(FILE *fd, GError **error);
// static gint32 save_max_multi(FILE *fd, GError **error);

static guchar max_default_palette[PALETTE_SIZE] = {PALETTE_INIT};

static struct MaxPluginSettings max_settings = {MAX_FORMAT_AUTO};

const GimpPlugInInfo PLUG_IN_INFO = {
    NULL,
    NULL,
    query,
    run,
};

MAIN()

static void query(void) {
    static const GimpParamDef thumb_args[] = {{GIMP_PDB_STRING, "filename", "The name of the file to load"},
                                              {GIMP_PDB_INT32, "thumb-size", "Preferred thumbnail size"}};

    static const GimpParamDef thumb_return_vals[] = {{GIMP_PDB_IMAGE, "image", "Thumbnail image"},
                                                     {GIMP_PDB_INT32, "image-width", "Width of full-sized image"},
                                                     {GIMP_PDB_INT32, "image-height", "Height of full-sized image"}};

    static const GimpParamDef load_args[] = {
        {GIMP_PDB_INT32, "run-mode", "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }"},
        {GIMP_PDB_STRING, "filename", "The name of the file to load"},
        {GIMP_PDB_STRING, "raw-filename", "The name entered"},
    };

    static const GimpParamDef load_return_vals[] = {
        {GIMP_PDB_IMAGE, "image", "Output image"},
    };

    static const GimpParamDef save_args[] = {
        {GIMP_PDB_INT32, "run-mode", "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }"},
        {GIMP_PDB_IMAGE, "image", "Input image"},
        {GIMP_PDB_DRAWABLE, "drawable", "Drawable to save"},
        {GIMP_PDB_STRING, "filename", "The name of the file to save the image in"},
        {GIMP_PDB_STRING, "raw-filename", "The name entered"},
    };

    gimp_install_procedure(LOAD_THUMB_PROC, "Loads a preview of a M.A.X. graphics file",
                           "Plug-In version: " MAX_PLUGIN_VERSION, "M.A.X. Port Team", "M.A.X. Port Team", "2022", NULL,
                           NULL, GIMP_PLUGIN, G_N_ELEMENTS(thumb_args), G_N_ELEMENTS(thumb_return_vals), thumb_args,
                           thumb_return_vals);

    gimp_register_thumbnail_loader(LOAD_PROC, LOAD_THUMB_PROC);

    gimp_install_procedure(LOAD_PROC, "Loads M.A.X. graphics files", "Plug-In version: " MAX_PLUGIN_VERSION,
                           "M.A.X. Port Team", "M.A.X. Port Team", "2022", "MAX Image", NULL, GIMP_PLUGIN,
                           G_N_ELEMENTS(load_args), G_N_ELEMENTS(load_return_vals), load_args, load_return_vals);

    gimp_register_file_handler_mime(LOAD_PROC, "image/max");

    gimp_register_magic_load_handler(LOAD_PROC, "", "", "");

    gimp_install_procedure(SAVE_PROC, "Saves M.A.X. graphics files", "Plug-In version: " MAX_PLUGIN_VERSION,
                           "M.A.X. Port Team", "M.A.X. Port Team", "2022", "MAX Image", "INDEXED", GIMP_PLUGIN,
                           G_N_ELEMENTS(save_args), 0, save_args, NULL);

    gimp_register_file_handler_mime(SAVE_PROC, "image/max");

    gimp_register_save_handler(SAVE_PROC, "", "");
}

static void run(const gchar *name, gint nparams, const GimpParam *param, gint *nreturn_vals, GimpParam **return_vals) {
    static GimpParam values[4];
    GimpRunMode run_mode;
    GimpPDBStatusType status = GIMP_PDB_SUCCESS;
    GError *error = NULL;

    gegl_init(NULL, NULL);

    run_mode = param[0].data.d_int32;

    *nreturn_vals = 1;
    *return_vals = values;

    values[0].type = GIMP_PDB_STATUS;
    values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

    if (strcmp(name, LOAD_THUMB_PROC) == 0) {
        if (nparams < 2) {
            status = GIMP_PDB_CALLING_ERROR;
        } else {
            const gchar *filename = param[0].data.d_string;
            gint width = param[1].data.d_int32;
            gint height = param[1].data.d_int32;
            gint32 image_ID;

            image_ID = load_thumbnail(filename, &width, &height, &error);

            if (image_ID != -1) {
                *nreturn_vals = 4;

                values[1].type = GIMP_PDB_IMAGE;
                values[1].data.d_image = image_ID;
                values[2].type = GIMP_PDB_INT32;
                values[2].data.d_int32 = width;
                values[3].type = GIMP_PDB_INT32;
                values[3].data.d_int32 = height;
            } else {
                status = GIMP_PDB_EXECUTION_ERROR;
            }
        }
    } else if (strcmp(name, LOAD_PROC) == 0) {
        switch (run_mode) {
            case GIMP_RUN_INTERACTIVE: {
            } break;

            case GIMP_RUN_NONINTERACTIVE: {
                if (nparams != 3) status = GIMP_PDB_CALLING_ERROR;
            } break;
        }

        if (status == GIMP_PDB_SUCCESS) {
            gint32 image_ID = load_image(param[1].data.d_string, &error);

            if (image_ID != -1) {
                *nreturn_vals = 2;
                values[1].type = GIMP_PDB_IMAGE;
                values[1].data.d_image = image_ID;
            } else {
                status = GIMP_PDB_EXECUTION_ERROR;
            }
        }
    } else if (strcmp(name, SAVE_PROC) == 0) {
        gint32 image_ID = param[1].data.d_int32;
        gint32 drawable_ID = param[2].data.d_int32;
        GimpExportReturn export = GIMP_EXPORT_CANCEL;

        export = gimp_export_image(&image_ID, &drawable_ID, "M.A.X. Formats",
                                   GIMP_EXPORT_CAN_HANDLE_ALPHA | GIMP_EXPORT_CAN_HANDLE_INDEXED);

        if (export == GIMP_EXPORT_CANCEL) {
            values[0].data.d_status = GIMP_PDB_CANCEL;
            return;
        }

        switch (run_mode) {
            case GIMP_RUN_INTERACTIVE: {
                gimp_get_data(SAVE_PROC, &max_settings);

                if (!save_dialog(image_ID, &error)) {
                    values[0].data.d_status = GIMP_PDB_CANCEL;
                    return;
                }
            } break;

            case GIMP_RUN_WITH_LAST_VALS: {
            } break;

            case GIMP_RUN_NONINTERACTIVE: {
                if (nparams != 5) status = GIMP_PDB_CALLING_ERROR;
            } break;
        }

        if (status == GIMP_PDB_SUCCESS) {
            status = save_image(param[3].data.d_string, image_ID, drawable_ID, run_mode, &error);
        }

        if (export == GIMP_EXPORT_EXPORT) {
            gimp_image_delete(image_ID);
        }

    } else {
        status = GIMP_PDB_CALLING_ERROR;
    }

    if (status != GIMP_PDB_SUCCESS && error) {
        *nreturn_vals = 2;
        values[1].type = GIMP_PDB_STRING;
        values[1].data.d_string = error->message;
    }

    values[0].data.d_status = status;
}

gint32 load_thumbnail(const gchar *filename, gint *width, gint *height, GError **error) { return -1; }

gboolean image_rle_decode(FILE *fd, gint data_size, guchar *pixels, gint width, gint height) {
    gpointer buffer = NULL;
    guchar *pointer = NULL;
    gint16 option_word = 0;
    gint image_size = width * height;

    buffer = g_malloc(data_size);
    if (!buffer) {
        return FALSE;
    }

    if (data_size != fread(buffer, sizeof(guchar), data_size, fd)) {
        g_free(buffer);
        return FALSE;
    }

    pointer = buffer;

    while (image_size > 0) {
        option_word = GINT16_FROM_LE(*(gint16 *)pointer);
        pointer += sizeof(option_word);

        if (option_word > 0) {
            memcpy(pixels, pointer, option_word);

            pointer += option_word;
        } else {
            option_word = -option_word;

            memset(pixels, pointer[0], option_word);

            pointer += sizeof(guchar);
        }

        pixels += option_word;
        image_size -= option_word;
    }

    g_free(buffer);
    return TRUE;
}

gboolean image_rle_encode_emit(FILE *fd, guchar *buffer, gint size, gboolean repeat_mode) {
    if (repeat_mode) {
        gint16 option_word = -G_MAXINT16;

        while (size) {
            if (size >= G_MAXINT16) {
                size -= G_MAXINT16;
            } else {
                option_word = -size;
                size = 0;
            }

            if (1 != fwrite(&option_word, sizeof(option_word), 1, fd)) {
                return FALSE;
            }

            if (1 != fwrite(buffer, sizeof(guchar), 1, fd)) {
                return FALSE;
            }
        }
    } else {
        gint16 option_word = G_MAXINT16;
        gint offset = 0;

        while (size) {
            if (size >= G_MAXINT16) {
                size -= G_MAXINT16;
            } else {
                option_word = size;
                size = 0;
            }

            if (1 != fwrite(&option_word, sizeof(option_word), 1, fd)) {
                return FALSE;
            }

            if (option_word != fwrite(&buffer[offset], sizeof(guchar), option_word, fd)) {
                return FALSE;
            }

            offset += option_word;
        }
    }

    return TRUE;
}

static inline gboolean image_rle_find_pattern(guchar *buffer) {
    return buffer[0] == buffer[1] && buffer[1] == buffer[2] && buffer[2] == buffer[3] && buffer[3] == buffer[4];
}

gboolean image_rle_encode(FILE *fd, guchar *buffer, gint rows, gint rowstride) {
    if (!fd) {
        return FALSE;
    }

    for (int i = 0; i < rows; ++i) {
        gboolean repeat_mode = FALSE;
        gint start_position = i * rowstride;

        for (gint j = i * rowstride; j < i * rowstride + rowstride; ++j) {
            if (repeat_mode) {
                if (buffer[j - 1] != buffer[j]) {
                    if (!image_rle_encode_emit(fd, &buffer[start_position], j - start_position, repeat_mode)) {
                        return FALSE;
                    }

                    repeat_mode = FALSE;
                    start_position = j;
                }

            } else if (rowstride > RLE_BREAK_EVEN && j > i * rowstride && image_rle_find_pattern(&buffer[j - 1])) {
                if (!image_rle_encode_emit(fd, &buffer[start_position], j - start_position - 1, repeat_mode)) {
                    return FALSE;
                }

                repeat_mode = TRUE;
                start_position = j - 1;
            }

            if (j == i * rowstride + rowstride - 1) {
                if (!image_rle_encode_emit(fd, &buffer[start_position], i * rowstride + rowstride - start_position,
                                           repeat_mode)) {
                    return FALSE;
                }
            }
        }
    }

    return TRUE;
}

gint32 load_image(const gchar *filename, GError **error) {
    FILE *fd = NULL;
    gsize file_size = 0;
    gint32 image_ID = -1;
    gboolean result;

    gimp_progress_init_printf("Opening '%s'", gimp_filename_to_utf8(filename));
    result = gimp_progress_update(0.0);
    g_assert(result);

    fd = g_fopen(filename, "rb");

    if (!fd) {
        g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno), "Could not open '%s' for reading: %s",
                    gimp_filename_to_utf8(filename), g_strerror(errno));
        return image_ID;
    }

    if (0 != fseek(fd, 0, SEEK_END)) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_IO, "File seek error.");
        fclose(fd);
        return image_ID;
    }

    file_size = ftell(fd);
    if (file_size == -1) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_IO, "File tell error.");
        fclose(fd);
        return image_ID;
    }

    if (0 != fseek(fd, 0, SEEK_SET)) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_IO, "File seek error.");
        fclose(fd);
        return image_ID;
    }

    {
        gint16 width;
        gint16 height;
        gint16 hotx;
        gint16 hoty;

        if (1 == fread(&width, sizeof(width), 1, fd) && 1 == fread(&height, sizeof(height), 1, fd) &&
            1 == fread(&hotx, sizeof(hotx), 1, fd) && 1 == fread(&hoty, sizeof(hoty), 1, fd)) {
            width = GINT16_FROM_LE(width);
            height = GINT16_FROM_LE(height);
            hotx = GINT16_FROM_LE(hotx);
            hoty = GINT16_FROM_LE(hoty);

            if (width * height + sizeof(width) + sizeof(height) + sizeof(hotx) + sizeof(hoty) == file_size) {
                image_ID = load_max_simple(fd, error);
            }
        }
    }

    if (image_ID == -1) {
        gint16 hotx;
        gint16 hoty;
        gint16 width;
        gint16 height;

        if (0 != fseek(fd, 0, SEEK_SET)) {
            g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_IO, "File seek error.");
            fclose(fd);
            return image_ID;
        }

        if (1 == fread(&hotx, sizeof(hotx), 1, fd) && 1 == fread(&hoty, sizeof(hoty), 1, fd) &&
            1 == fread(&width, sizeof(width), 1, fd) && 1 == fread(&height, sizeof(height), 1, fd)) {
            hotx = GINT16_FROM_LE(hotx);
            hoty = GINT16_FROM_LE(hoty);
            width = GINT16_FROM_LE(width);
            height = GINT16_FROM_LE(height);

            if (hotx == 0 && hoty == 0 && width > 0 && height > 0) {
                image_ID = load_max_big(fd, file_size, error);
            }
        }
    }

    if (image_ID == -1) {
        gint16 image_count;
        guint32 firt_image_offset;

        if (0 != fseek(fd, 0, SEEK_SET)) {
            g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_IO, "File seek error.");
            fclose(fd);
            return image_ID;
        }

        if (1 == fread(&image_count, sizeof(image_count), 1, fd) &&
            1 == fread(&firt_image_offset, sizeof(firt_image_offset), 1, fd)) {
            image_count = GINT16_FROM_LE(image_count);
            firt_image_offset = GUINT32_FROM_LE(firt_image_offset);

            if (image_count > 0 && firt_image_offset < file_size) {
                image_ID = load_max_multi(fd, error);
            }
        }
    }

    if (image_ID == -1) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_FAILED, "Image format not recognized.");
        fclose(fd);
        return image_ID;
    }

    /** \todo Save format type for later export */

    if (image_ID != -1) {
        result = gimp_image_set_filename(image_ID, filename);
        g_assert(result);
    }

    if (EOF == fclose(fd)) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_FAILED, "Failed to close '%s'.", gimp_filename_to_utf8(filename));
    }

    result = gimp_progress_update(100.0);
    g_assert(result);

    return image_ID;
}

gint32 load_max_simple(FILE *fd, GError **error) {
    gpointer pixels = NULL;
    gint pixel_count = -1;
    gint32 image_ID = -1;
    gint32 layer;
    GeglBuffer *gbuffer;
    gint16 width;
    gint16 height;
    gint16 hotx;
    gint16 hoty;
    gboolean result;

    if (0 != fseek(fd, 0, SEEK_SET)) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_IO, "File seek error.");
        return image_ID;
    }

    if (1 != fread(&width, sizeof(width), 1, fd) || 1 != fread(&height, sizeof(height), 1, fd) ||
        1 != fread(&hotx, sizeof(hotx), 1, fd) || 1 != fread(&hoty, sizeof(hoty), 1, fd)) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_IO, "File read error.");
        return image_ID;
    }

    width = GINT16_FROM_LE(width);
    height = GINT16_FROM_LE(height);
    hotx = GINT16_FROM_LE(hotx);
    hoty = GINT16_FROM_LE(hoty);

    pixel_count = width * height;
    g_assert(pixel_count > 0);

    pixels = g_malloc(pixel_count);
    if (!pixels) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_NOMEM, "Not enough memory.");
        return image_ID;
    }

    if (pixel_count != fread(pixels, sizeof(guchar), pixel_count, fd)) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_IO, "File read error.");
        g_free(pixels);
        return image_ID;
    }

    image_ID = gimp_image_new(width, height, GIMP_INDEXED);
    g_assert(image_ID != -1);

    layer = gimp_layer_new(image_ID, "Background", width, height, GIMP_INDEXED_IMAGE, 100,
                           gimp_image_get_default_new_layer_mode(image_ID));
    result = gimp_image_insert_layer(image_ID, layer, -1, 0);
    g_assert(result);

    gbuffer = gimp_drawable_get_buffer(layer);
    gegl_buffer_set(gbuffer, GEGL_RECTANGLE(0, 0, width, height), 0, NULL, pixels, GEGL_AUTO_ROWSTRIDE);
    g_object_unref(gbuffer);

    result = gimp_image_set_colormap(image_ID, max_default_palette, PALETTE_COLORS);
    g_assert(result);

    g_free(pixels);

    return image_ID;
}

gint32 load_max_big(FILE *fd, gint file_size, GError **error) {
    gpointer pixels = NULL;
    gpointer palette = NULL;
    gint pixel_count = -1;
    gint32 image_ID = -1;
    gint32 layer;
    GeglBuffer *gbuffer;
    gint16 width;
    gint16 height;
    gint16 hotx;
    gint16 hoty;
    gboolean result;

    if (0 != fseek(fd, 0, SEEK_SET)) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_IO, "File seek error.");
        return image_ID;
    }

    if (1 != fread(&hotx, sizeof(hotx), 1, fd) || 1 != fread(&hoty, sizeof(hoty), 1, fd) ||
        1 != fread(&width, sizeof(width), 1, fd) || 1 != fread(&height, sizeof(height), 1, fd)) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_IO, "File read error.");
        return image_ID;
    }

    hotx = GINT16_FROM_LE(hotx);
    hoty = GINT16_FROM_LE(hoty);
    width = GINT16_FROM_LE(width);
    height = GINT16_FROM_LE(height);

    pixel_count = width * height;
    g_assert(pixel_count > 0);

    pixels = g_malloc(pixel_count);
    palette = g_malloc(PALETTE_SIZE);

    if (!pixels) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_NOMEM, "Not enough memory.");
        return image_ID;
    }

    if (!palette) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_NOMEM, "Not enough memory.");
        g_free(pixels);
        return image_ID;
    }

    if (PALETTE_SIZE != fread(palette, sizeof(guchar), PALETTE_SIZE, fd)) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_IO, "File read error.");
        g_free(pixels);
        g_free(palette);
        return image_ID;
    }

    if (!image_rle_decode(fd, file_size - 8 - PALETTE_SIZE, pixels, width, height)) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_IO, "File decode error.");
        g_free(pixels);
        g_free(palette);
        return image_ID;
    }

    image_ID = gimp_image_new(width, height, GIMP_INDEXED);
    g_assert(image_ID != -1);

    layer = gimp_layer_new(image_ID, "Background", width, height, GIMP_INDEXED_IMAGE, 100,
                           gimp_image_get_default_new_layer_mode(image_ID));
    result = gimp_image_insert_layer(image_ID, layer, -1, 0);
    g_assert(result);

    gbuffer = gimp_drawable_get_buffer(layer);
    gegl_buffer_set(gbuffer, GEGL_RECTANGLE(0, 0, width, height), 0, NULL, pixels, GEGL_AUTO_ROWSTRIDE);
    g_object_unref(gbuffer);

    result = gimp_image_set_colormap(image_ID, palette, PALETTE_COLORS);
    g_assert(result);

    g_free(pixels);
    g_free(palette);
    return image_ID;
}

gboolean decode_max_multi_image(FILE *fd, struct MaxMultiImage *image) {
    for (gint i = 0; i < image->height; ++i) {
        guchar transparent_count = 0;
        guchar pixel_count = 0;
        gint offset = 0;

        if (ftell(fd) != image->rows[i]) {
            return FALSE;
        }

        for (;;) {
            fread(&transparent_count, sizeof(transparent_count), 1, fd);
            if (transparent_count == 0xFF) {
                break;
            }

            fread(&pixel_count, sizeof(pixel_count), 1, fd);
            offset += transparent_count;
            fread(&image->pixels[offset + i * image->width], sizeof(guchar), pixel_count, fd);
            offset += pixel_count;
        }
    }

    return TRUE;
}

gboolean decode_max_multi_shadow(FILE *fd, struct MaxMultiImage *image) {
    for (gint i = 0; i < image->height; ++i) {
        guchar transparent_count = 0;
        guchar shadow_count = 0;
        gint offset = 0;

        if (ftell(fd) != image->rows[i]) {
            return FALSE;
        }

        for (;;) {
            fread(&transparent_count, sizeof(transparent_count), 1, fd);
            if (transparent_count == 0xFF) {
                break;
            }

            fread(&shadow_count, sizeof(shadow_count), 1, fd);
            offset += transparent_count;

            if (offset + i * image->width + shadow_count > image->width * image->height) {
                return FALSE;
            }

            memset(&image->pixels[offset + i * image->width], 20, shadow_count);
            offset += shadow_count;
        }
    }

    return TRUE;
}

gpointer read_max_multi_image(FILE *fd, guint32 address) {
    static gboolean decoder_mode = TRUE;
    struct MaxMultiImage *image;

    image = g_malloc(sizeof(struct MaxMultiImage));
    if (!image) {
        return NULL;
    }

    memset(image, 0, sizeof(struct MaxMultiImage));

    image->file_offset = address;

    if (0 != fseek(fd, image->file_offset, SEEK_SET)) {
        g_free(image);
        return NULL;
    }

    if (1 != fread(&image->width, sizeof(image->width), 1, fd) ||
        1 != fread(&image->height, sizeof(image->height), 1, fd) ||
        1 != fread(&image->hotx, sizeof(image->hotx), 1, fd) || 1 != fread(&image->hoty, sizeof(image->hoty), 1, fd)) {
        g_free(image);
        return NULL;
    }

    image->width = GINT16_FROM_LE(image->width);
    image->height = GINT16_FROM_LE(image->height);
    image->hotx = GINT16_FROM_LE(image->hotx);
    image->hoty = GINT16_FROM_LE(image->hoty);

    image->rows = g_malloc(sizeof(gint32) * image->height);
    if (!image->rows) {
        g_free(image);
        return NULL;
    }

    for (gint i = 0; i < image->height; ++i) {
        gint32 row_address;

        if (1 != fread(&row_address, sizeof(row_address), 1, fd)) {
            g_free(image->rows);
            g_free(image);
            return NULL;
        }

        image->rows[i] = GUINT32_FROM_LE(row_address);
    }

    image->pixels = g_malloc(image->width * image->height);
    if (!image->pixels) {
        g_free(image->rows);
        g_free(image);
        return NULL;
    }

    memset(image->pixels, 0, image->width * image->height);

    if (decoder_mode) {
        gint32 restore_point;

        restore_point = ftell(fd);
        if (restore_point == -1) {
            g_free(image);
            return NULL;
        }

        if (!decode_max_multi_shadow(fd, image)) {
            if (0 != fseek(fd, restore_point, SEEK_SET)) {
                g_free(image);
                return NULL;
            }
            decoder_mode = FALSE;
        }
    }

    if (!decoder_mode) {
        if (!decode_max_multi_image(fd, image)) {
            g_free(image->pixels);
            g_free(image->rows);
            g_free(image);
            return NULL;
        }
    }

    return image;
}

gint32 load_max_multi(FILE *fd, GError **error) {
    guint32 *offsets = NULL;
    struct MaxMultiImage **images = NULL;
    gint pixel_count = -1;
    gint32 image_ID = -1;
    gint32 layer;
    GeglBuffer *gbuffer;
    gint16 image_count = 0;
    gboolean result;

    gint32 image_ulx = 0;
    gint32 image_uly = 0;
    gint32 image_lrx = 0;
    gint32 image_lry = 0;

    gint palette_colors = 0;
    guchar *palette = NULL;
    GimpRGB transparent_color;

    if (0 != fseek(fd, 0, SEEK_SET)) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_IO, "File seek error.");
        return image_ID;
    }

    if (1 != fread(&image_count, sizeof(image_count), 1, fd)) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_IO, "File read error.");
        return image_ID;
    }

    image_count = GINT32_FROM_LE(image_count);

    offsets = g_malloc(image_count * sizeof(guint32));
    if (!offsets) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_NOMEM, "Not enough memory.");
        return image_ID;
    }

    if (image_count != fread(offsets, sizeof(guint32), image_count, fd)) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_IO, "File read error.");
        g_free(offsets);
        return image_ID;
    }

    images = g_malloc(image_count * sizeof(struct MaxMultiImage *));
    if (!images) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_NOMEM, "Not enough memory.");
        g_free(offsets);
        return image_ID;
    }

    for (gint i = 0; i < image_count; ++i) {
        if (ftell(fd) != offsets[i]) {
            g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_FAILED, "Image format error.");
            /** \todo Free images */
            g_free(offsets);
            return -1;
        }

        images[i] = read_max_multi_image(fd, offsets[i]);
        if (!images[i]) {
            g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_FAILED, "Image format error.");
            /** \todo Free images */
            g_free(offsets);
            return -1;
        }
    }

    for (int i = 0; i < image_count; ++i) {
        image_ulx = MAX(images[i]->hotx, image_ulx);
        image_uly = MAX(images[i]->hoty, image_uly);
        image_lrx = MAX(images[i]->width - images[i]->hotx, image_lrx);
        image_lry = MAX(images[i]->height - images[i]->hoty, image_lry);
    }

    image_ID = gimp_image_new(image_ulx + image_lrx, image_uly + image_lry, GIMP_INDEXED);
    g_assert(image_ID != -1);

    result = gimp_image_set_colormap(image_ID, max_default_palette, PALETTE_COLORS);
    g_assert(result);

    palette = gimp_image_get_colormap(image_ID, &palette_colors);
    transparent_color.r = palette[0];
    transparent_color.g = palette[1];
    transparent_color.b = palette[2];
    transparent_color.a = 0;

    for (int i = 0; i < image_count; ++i) {
        gchar layer_name[10];
        GeglBuffer *gbuffer_layer;
        gint palette_colors;

        snprintf(layer_name, sizeof(layer_name), "layer %i", i);

        layer = gimp_layer_new(image_ID, layer_name, image_ulx + image_lrx, image_uly + image_lry, GIMP_INDEXED_IMAGE,
                               100, gimp_image_get_default_new_layer_mode(image_ID));
        result = gimp_image_insert_layer(image_ID, layer, -1, i);
        g_assert(result);

        gbuffer = gimp_drawable_get_buffer(layer);
        gbuffer_layer = gegl_buffer_linear_new_from_data(images[i]->pixels, gimp_drawable_get_format(layer),
                                                         GEGL_RECTANGLE(0, 0, images[i]->width, images[i]->height),
                                                         GEGL_AUTO_ROWSTRIDE, NULL, NULL);

        gegl_buffer_copy(gbuffer_layer, GEGL_RECTANGLE(0, 0, images[i]->width, images[i]->height), GEGL_ABYSS_NONE,
                         gbuffer,
                         GEGL_RECTANGLE(image_ulx - images[i]->hotx, image_uly - images[i]->hoty, images[i]->width,
                                        images[i]->height));
        g_object_unref(gbuffer_layer);
        g_object_unref(gbuffer);

        gimp_layer_add_alpha(layer);
        if (gimp_image_select_color(image_ID, GIMP_CHANNEL_OP_REPLACE, layer, &transparent_color)) {
            //            gimp_drawable_edit_clear(layer);
        }
    }

    /** \todo Free images */
    g_free(offsets);

    return image_ID;
}

void on_dialog_response(GtkWidget *widget, gint response_id, gpointer data) {
    if (response_id == GTK_RESPONSE_OK) {
        *(gboolean *)data = TRUE;
    }

    gtk_widget_destroy(widget);
}

void on_combo_changed(GtkComboBox *combo_box) { max_settings.file_type = gtk_combo_box_get_active(combo_box); }

gboolean save_dialog(gint32 image_ID, GError **error) {
    GtkWidget *dialog = NULL;
    GtkBuilder *builder = NULL;
    GtkWidget *frame = NULL;
    GtkWidget *combo = NULL;
    gboolean result = FALSE;

    gimp_ui_init(PLUG_IN_BINARY, FALSE);

    dialog = gimp_export_dialog_new("M.A.X. Formats", PLUG_IN_BINARY, SAVE_PROC);
    g_signal_connect(dialog, "response", G_CALLBACK(on_dialog_response), &result);
    g_signal_connect(dialog, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    builder = gtk_builder_new();
    if (0 == gtk_builder_add_from_string(builder, MAX_SAVE_GUI, -1, error)) {
        g_object_unref(builder);
        gtk_widget_destroy(dialog);
        return FALSE;
    }

    frame = GTK_WIDGET(gtk_builder_get_object(builder, "vbox-frame"));
    if (!frame) {
        g_object_unref(builder);
        gtk_widget_destroy(dialog);
        return FALSE;
    }

    gtk_box_pack_start(GTK_BOX(gimp_export_dialog_get_content_area(dialog)), frame, TRUE, TRUE, 0);

    combo = GTK_WIDGET(gtk_builder_get_object(builder, "format-combo"));
    if (!combo) {
        g_object_unref(builder);
        gtk_widget_destroy(dialog);
        return FALSE;
    }

    max_settings.file_type = MAX_FORMAT_AUTO;
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), max_settings.file_type);
    g_signal_connect(combo, "changed", G_CALLBACK(on_combo_changed), &max_settings);

    gtk_widget_show(dialog);
    gtk_main();

    g_object_unref(builder);

    return result;
}

GimpPDBStatusType save_max_simple(const gchar *filename, gint32 image, gint32 drawable_ID, GimpRunMode run_mode,
                                  GError **error) {
    FILE *fd;
    gpointer buffer = NULL;
    GeglBuffer *gbuffer = NULL;
    GimpImageType drawable_type;
    gint drawable_width = -1;
    gint drawable_height = -1;
    const Babl *format = NULL;
    guchar *g_palette;
    gint num_colors = -1;
    gint buffer_size = 0;
    gushort width;
    gushort height;
    gushort hotx;
    gushort hoty;

    gbuffer = gimp_drawable_get_buffer(drawable_ID);
    g_assert(gbuffer);

    drawable_width = gimp_drawable_width(drawable_ID);
    drawable_height = gimp_drawable_height(drawable_ID);

    if (drawable_width > G_MAXINT16 || drawable_height > G_MAXINT16) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_FAILED, "Image format error (width: %i, height: %i).",
                    drawable_width, drawable_height);
        return GIMP_PDB_EXECUTION_ERROR;
    }

    width = drawable_width;
    height = drawable_height;

    /** \todo Implement image settings */
    hotx = 0;
    hoty = 0;

    drawable_type = gimp_drawable_type(drawable_ID);

    switch (drawable_type) {
        case GIMP_RGB_IMAGE: {
        } break;

        case GIMP_RGBA_IMAGE: {
        } break;

        case GIMP_GRAY_IMAGE: {
        } break;

        case GIMP_GRAYA_IMAGE: {
        } break;

        case GIMP_INDEXED_IMAGE: {
            format = gimp_drawable_get_format(drawable_ID);
            g_palette = gimp_image_get_colormap(image, &num_colors);

        } break;

        case GIMP_INDEXEDA_IMAGE: {
        } break;

        default: {
            g_assert_not_reached();
        }
    }

    gimp_progress_init_printf("Exporting '%s'", gimp_filename_to_utf8(filename));

    fd = g_fopen(filename, "wb");
    if (!fd) {
        g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno), "Could not open '%s' for writing: %s",
                    gimp_filename_to_utf8(filename), g_strerror(errno));
        return GIMP_PDB_EXECUTION_ERROR;
    }

    buffer_size = drawable_width * drawable_height * sizeof(guchar);
    buffer = g_malloc(buffer_size);
    if (!buffer) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_NOMEM, "Not enough memory (%i).", buffer_size);
    } else {
        gegl_buffer_get(gbuffer, GEGL_RECTANGLE(0, 0, drawable_width, drawable_height), 1.0, format, buffer,
                        GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
        g_object_unref(gbuffer);

        if (1 == fwrite(&width, sizeof(width), 1, fd) && 1 == fwrite(&height, sizeof(height), 1, fd) &&
            1 == fwrite(&hotx, sizeof(hotx), 1, fd) && 1 == fwrite(&hoty, sizeof(hoty), 1, fd) &&
            buffer_size == fwrite(buffer, sizeof(guchar), buffer_size, fd)) {
        } else {
            g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_IO, "File write error.");
        }
    }

    g_free(buffer);
    if (EOF == fclose(fd)) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_FAILED, "Failed to close '%s'.", gimp_filename_to_utf8(filename));
    }

    return GIMP_PDB_SUCCESS;
}

GimpPDBStatusType save_max_big(const gchar *filename, gint32 image, gint32 drawable_ID, GimpRunMode run_mode,
                               GError **error) {
    FILE *fd;
    gpointer buffer = NULL;
    GeglBuffer *gbuffer = NULL;
    GimpImageType drawable_type;
    gint drawable_width = -1;
    gint drawable_height = -1;
    const Babl *format = NULL;
    guchar *g_palette;
    gint num_colors = -1;
    gint buffer_size = 0;
    gushort width;
    gushort height;
    gushort hotx;
    gushort hoty;

    gbuffer = gimp_drawable_get_buffer(drawable_ID);
    g_assert(gbuffer);

    drawable_width = gimp_drawable_width(drawable_ID);
    drawable_height = gimp_drawable_height(drawable_ID);

    if (drawable_width > G_MAXINT16 || drawable_height > G_MAXINT16) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_FAILED, "Image format error (width: %i, height: %i).",
                    drawable_width, drawable_height);
        return GIMP_PDB_EXECUTION_ERROR;
    }

    width = drawable_width;
    height = drawable_height;

    /** \todo Implement image settings */
    hotx = 0;
    hoty = 0;

    drawable_type = gimp_drawable_type(drawable_ID);

    switch (drawable_type) {
        case GIMP_RGB_IMAGE: {
        } break;

        case GIMP_RGBA_IMAGE: {
        } break;

        case GIMP_GRAY_IMAGE: {
        } break;

        case GIMP_GRAYA_IMAGE: {
        } break;

        case GIMP_INDEXED_IMAGE: {
            format = gimp_drawable_get_format(drawable_ID);
            g_palette = gimp_image_get_colormap(image, &num_colors);

        } break;

        case GIMP_INDEXEDA_IMAGE: {
        } break;

        default: {
            g_assert_not_reached();
        }
    }

    gimp_progress_init_printf("Exporting '%s'", gimp_filename_to_utf8(filename));

    fd = g_fopen(filename, "wb");
    if (!fd) {
        g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno), "Could not open '%s' for writing: %s",
                    gimp_filename_to_utf8(filename), g_strerror(errno));
        return GIMP_PDB_EXECUTION_ERROR;
    }

    buffer_size = drawable_width * drawable_height * sizeof(guchar);
    buffer = g_malloc(buffer_size);
    if (!buffer) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_NOMEM, "Not enough memory (%i).", buffer_size);
    } else {
        gegl_buffer_get(gbuffer, GEGL_RECTANGLE(0, 0, drawable_width, drawable_height), 1.0, format, buffer,
                        GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
        g_object_unref(gbuffer);

        if (1 == fwrite(&hotx, sizeof(hotx), 1, fd) && 1 == fwrite(&hoty, sizeof(hoty), 1, fd) &&
            1 == fwrite(&width, sizeof(width), 1, fd) && 1 == fwrite(&height, sizeof(height), 1, fd) &&
            num_colors * 3 == fwrite(g_palette, sizeof(guchar), num_colors * 3, fd) &&
            image_rle_encode(fd, buffer, drawable_height, drawable_width)) {
        } else {
            g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_IO, "File write error.");
        }
    }

    g_free(buffer);
    if (EOF == fclose(fd)) {
        g_clear_error(error);
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_FAILED, "Failed to close '%s'.", gimp_filename_to_utf8(filename));
    }

    return GIMP_PDB_SUCCESS;
}

gint32 save_max_multi(FILE *fd, GError **error) {}

GimpPDBStatusType save_image(const gchar *filename, gint32 image, gint32 drawable_ID, GimpRunMode run_mode,
                             GError **error) {
    GimpPDBStatusType result = GIMP_PDB_EXECUTION_ERROR;

    switch (max_settings.file_type) {
        case MAX_FORMAT_AUTO: {
        } break;
        case MAX_FORMAT_SIMPLE: {
            result = save_max_simple(filename, image, drawable_ID, run_mode, error);
        } break;
        case MAX_FORMAT_BIG: {
            result = save_max_big(filename, image, drawable_ID, run_mode, error);
        } break;
        case MAX_FORMAT_MULTI: {
        } break;
        case MAX_FORMAT_SHADOW: {
        } break;
        default: {
        } break;
    }

    return result;
}
