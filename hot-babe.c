/* Hot-babe 
 * Copyright (C) 2002 DindinX <David@dindinx.org>
 * Copyright (C) 2002 Bruno Bellamy.
 *
 * This program is free software; you can redistribute it and/or modify
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Please note that part of this code is from wmbubble, and such should be
 * copyrighted by <timecop@japan.co.jp>
 * 
 */

/* general includes */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/* x11 includes */
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "hb01/hb01_0.xpm"
#include "hb01/hb01_1.xpm"
#include "hb01/hb01_2.xpm"
#include "hb01/hb01_3.xpm"
#include "hb01/hb01_4.xpm"

static int system_cpu(void);
static void hotbabe_setup_samples(void);
static void hotbabe_update(void);
static void create_hotbabe_window(void);
static void print_usage(void);

/* global variables */

typedef struct
{
  /* X11 stuff */
  gint       height, width;
  GdkWindow *win;        /* main window */
  GdkPixbuf *current_pixbuf;
  GdkPixbuf *pixbuf[10];

  int samples;

  /* CPU percentage stuff.  soon to go away */
  int loadIndex;
  u_int64_t *load, *total;
  guint threshold;

  /* optional stuff */
  gboolean incremental;
  gboolean noNice;
  guint    delay;  
} HotBabeData;

int nb_xpm;

HotBabeData bm;

/* returns current CPU load in percent, 0 to 256 */
static int system_cpu(void)
{
  unsigned int  cpuload;
  u_int64_t     load, total, oload, ototal;
  u_int64_t     ab, ac, ad, ae;
  int           i;
  FILE         *stat;

  stat = fopen("/proc/stat", "r");
  fscanf(stat, "%*s %Ld %Ld %Ld %Ld", &ab, &ac, &ad, &ae);
  fclose(stat);

  /* Find out the CPU load */
  /* user + sys = load
   * total = total */
  load = ab + ad;  /* cpu.user + cpu.sys; */
  if(!bm.noNice) load += ac;
  total = ab + ac + ad + ae;  /* cpu.total; */

  i = bm.loadIndex;
  oload = bm.load[i];
  ototal = bm.total[i];

  bm.load[i] = load;
  bm.total[i] = total;
  bm.loadIndex = (i + 1) % bm.samples;

  /*
   *   Because the load returned from libgtop is a value accumulated
   *   over time, and not the current load, the current load percentage
   *   is calculated as the extra amount of work that has been performed
   *   since the last sample. yah, right, what the fuck does that mean?
   */
  if (ototal == 0)    /* ototal == 0 means that this is the first time we get here */
    cpuload = 0;
  else
    cpuload = (256 * (load - oload)) / (total - ototal);

  return cpuload;
}



/* This is the function that actually creates the display widgets */
static void create_hotbabe_window(void)
{
#define MASK GDK_BUTTON_PRESS_MASK | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK
  GdkWindowAttr  attr;
  GdkPixmap     *pixmap;
  GdkBitmap     *mask;

  bm.current_pixbuf = gdk_pixbuf_new_from_xpm_data((const char**)hb01_4_xpm);
  bm.pixbuf[0] = gdk_pixbuf_new_from_xpm_data((const char **)hb01_4_xpm);
  bm.pixbuf[1] = gdk_pixbuf_new_from_xpm_data((const char **)hb01_3_xpm);
  bm.pixbuf[2] = gdk_pixbuf_new_from_xpm_data((const char **)hb01_2_xpm);
  bm.pixbuf[3] = gdk_pixbuf_new_from_xpm_data((const char **)hb01_1_xpm);
  bm.pixbuf[4] = gdk_pixbuf_new_from_xpm_data((const char **)hb01_0_xpm);
  nb_xpm = 5;
  bm.width = gdk_pixbuf_get_width(bm.pixbuf[0]);
  bm.height = gdk_pixbuf_get_height(bm.pixbuf[0]);
 
  attr.width = bm.width;
  attr.height = bm.height;
  attr.title = "hot-babe";
  attr.event_mask = MASK;
  attr.wclass = GDK_INPUT_OUTPUT;
  attr.visual = gdk_visual_get_system();
  attr.colormap = gdk_colormap_get_system();
  attr.wmclass_name = "hot-babe";
  attr.wmclass_class = "hot-babe";
  attr.window_type = GDK_WINDOW_TOPLEVEL;

  bm.win = gdk_window_new(NULL, &attr,
                          GDK_WA_TITLE | GDK_WA_WMCLASS |
                          GDK_WA_VISUAL | GDK_WA_COLORMAP);
  if (!bm.win)
  {
    fprintf(stderr, "Cannot make toplevel window\n");
    exit (-1);
  }
  gdk_window_set_decorations(bm.win, 0);

  pixmap =
    gdk_pixmap_create_from_xpm_d(bm.win, &mask, NULL, hb01_4_xpm);

  gdk_window_shape_combine_mask(bm.win, mask, 0, 0);
  gdk_window_set_back_pixmap(bm.win, pixmap, False);

  gdk_window_show(bm.win);

  hotbabe_setup_samples();
#undef MASK
}

static void hotbabe_update(void)
{
  unsigned int loadPercentage;
  static unsigned int old_percentage = 0;
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  guint i;
  guchar *pixels, *pixels1, *pixels2;
  GdkPixbuf *bla;
  static  gint robinet = 0;

  /* Find out the CPU load */
  loadPercentage = system_cpu();

  if (bm.threshold)
  {
    if (loadPercentage < bm.threshold || bm.threshold>255)
      loadPercentage = 0;
    else
      loadPercentage = (loadPercentage-bm.threshold)*256/(256-bm.threshold);
  }
  
  robinet +=loadPercentage/50-3;

  robinet = CLAMP(robinet, 0, 256);

  if (bm.incremental)
    loadPercentage = robinet;
  
  if (loadPercentage != old_percentage)
  {
    gint range = 256  / (nb_xpm-1);
    gint index = loadPercentage/range;

    old_percentage = loadPercentage;
    if  (index>nb_xpm-1) index = nb_xpm-1;
    pixels  = gdk_pixbuf_get_pixels(bm.current_pixbuf);
    pixels1 = gdk_pixbuf_get_pixels(bm.pixbuf[index]);
    if (index  == nb_xpm-1)
      pixels2 = gdk_pixbuf_get_pixels(bm.pixbuf[index]);
    else
      pixels2 = gdk_pixbuf_get_pixels(bm.pixbuf[index+1]);

    loadPercentage = loadPercentage % range;
    for (i=0  ;  i<bm.height*bm.width ;  i++)
    {
      guint val;
      val = ((guint)pixels2[i*4+0])*loadPercentage+((guint)pixels1[i*4+0])*(range-loadPercentage);
      val /= range;
      pixels[i*4+0] = val;
      val = ((guint)pixels2[i*4+1])*loadPercentage+((guint)pixels1[i*4+1])*(range-loadPercentage);
      val /= range;
      pixels[i*4+1] = val;
      val = ((guint)pixels2[i*4+2])*loadPercentage+((guint)pixels1[i*4+2])*(range-loadPercentage);
      val /= range;
      pixels[i*4+2] = val;
      val = ((guint)pixels2[i*4+3])*loadPercentage+((guint)pixels1[i*4+3])*(range-loadPercentage);
      val /= range;
      pixels[i*4+3] = val;
    }
    bla = gdk_pixbuf_new_from_data(pixels, GDK_COLORSPACE_RGB,
                                   gdk_pixbuf_get_has_alpha(bm.pixbuf[0]),
                                   gdk_pixbuf_get_bits_per_sample(bm.pixbuf[0]),
                                   gdk_pixbuf_get_width(bm.pixbuf[0]),
                                   gdk_pixbuf_get_height(bm.pixbuf[0]),
                                   gdk_pixbuf_get_rowstride(bm.pixbuf[0]), NULL, NULL);
    
    gdk_pixbuf_render_pixmap_and_mask(bla, &pixmap, &mask, 127);
    gdk_pixbuf_unref(bla);
    gdk_window_set_back_pixmap(bm.win, pixmap, False);
    gdk_pixmap_unref(pixmap);
    gdk_bitmap_unref(mask);
    gdk_window_clear(bm.win);
  }
}

static void hotbabe_setup_samples(void)
{
  int       i;
  u_int64_t load = 0, total = 0;

  bm.loadIndex = 0;
  bm.load = malloc(bm.samples * sizeof(u_int64_t));
  bm.total = malloc(bm.samples * sizeof(u_int64_t));
  for (i = 0; i < bm.samples;i++)
  {
    bm.load[i] = load;
    bm.total[i] = total;
  }
}


static void print_usage(void)
{
  g_print("Usage: hot-babe [OPTIONS]\n\n");
  g_print("OPTIONS are from the following:\n\n");
  g_print(" -t, --threshold n    use only the first picture before n%%.\n");
  g_print(" -i, --incremental    incremental (slow) mode.\n");
  g_print(" -d, --delay  n       update every n millisecondes.\n");
  g_print(" -h, --help           show this message and exit.\n");
  g_print(" -N, --noNice         don't count nice time in usage.\n");
}

int main(int argc, char **argv)
{
  GdkEvent *event;
  gint      i;

  /* initialize GDK */
  if (!gdk_init_check(&argc, &argv))
  {
    fprintf(stderr,
            "GDK init failed, bye bye.  Check \"DISPLAY\" variable.\n");
    exit(-1);
  }
  gdk_rgb_init();

  /* zero data structure */
  memset(&bm, 0, sizeof(bm));

  bm.samples     = 16;
  bm.incremental = FALSE;
  bm.delay       = 15000;
  bm.noNice      = FALSE;

  for (i=1 ; i<argc ; i++)
  {
    if (!strcmp(argv[i], "--threshold") || !strcmp(argv[i], "-t"))
    {
      i++;
      if  (i<argc)
      {
        bm.threshold = atoi(argv[i])*256/100;
      }        
    } else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h"))
    {
      print_usage();
      exit(0);
    } else if (!strcmp(argv[i], "--incremental") || !strcmp(argv[i], "-i"))
    {
      bm.incremental = TRUE;
    } else if (!strcmp(argv[i], "--noNice") || !strcmp(argv[i], "-N"))
    {
      bm.noNice = TRUE;
    } else if (!strcmp(argv[i], "--delay") || !strcmp(argv[i], "-d"))
    {
      i++;
      if  (i<argc)
      {
        bm.delay = atoi(argv[i])*1000;
      }        
    }
  }
 
  create_hotbabe_window();

  while (1)
  {
    while (gdk_events_pending())
    {
      event = gdk_event_get();
      if (event)
      {
        switch (event->type)
        {
          case GDK_DESTROY:
            gdk_exit(0);
            exit(0);
            break;
          case GDK_BUTTON_PRESS:
            if (event->button.button == 3)
            {
              exit(0);
              break;
            }
            break;
          default:
            break;
        }
      }
    }
    usleep(bm.delay);
    hotbabe_update();
  }
  return 0;
}

