#include <glib.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int SCALE=512;
int overlap_solid=1;
static int   author_mode = 0;
static int   y_shift = 0;

/* we expect to find these, in this order at the beginning of the
 * palette
 */
enum {
  C_BLANK=0,
  C_SOLID
};

typedef struct Mapping {
  gchar  *name;
  int     type;
  gchar   ascii;
} Mapping;

static Mapping catalog[256]={{0,},};
static int     n_catalog = 0;

const char *mapping2str (int type)
{
  if (type < n_catalog)
    return catalog[type].name;
  return NULL;
}

int resolve_mapping_str (const char *str)
{
  int i;
  for (i = 0; i < n_catalog; i++)
    if (!strcmp (str, catalog[i].name))
      return i;
  return 0;
}

int catalog_add (const gchar *name)
{
  int i;
  for (i = 0; i < n_catalog; i++)
    if (!strcmp (name, catalog[i].name))
      {
        fprintf (stderr, "%s already exists!\n");
        return i;
      }
  catalog[n_catalog++].name = g_strdup (name);
  return i;
}

static Mapping map[256]={{0,},};
static int     mappings=0;

gchar *component_name_tmp = NULL;
GString *component_str = NULL;

void write_component (const char *name, const char *curve_xml);

void finalize_component (void)
{
  if (!component_name_tmp)
    return;
  g_string_append_printf (component_str, "</contour>\n");

  write_component (component_name_tmp, component_str->str);
  g_free (component_name_tmp);
  g_string_free (component_str, TRUE);
  component_name_tmp = NULL;
  component_str = NULL;
}

void add_component (const char *component_name)
{
  finalize_component ();
  component_name_tmp = g_strdup (component_name);
  component_str = g_string_new ("<contour>\n");
}

void add_point (char type, float x, float y)
{
  g_assert (component_str);
  switch (type)
    {
      case 'L':
        g_string_append_printf (component_str,
            "    <point type='line' x='%d' y='%d'/>\n",
            (int)(SCALE * x), (int)(SCALE * y));
      break;
      case 'c':
        g_string_append_printf (component_str,
            "    <point x='%d' y='%d'/>\n",
            (int)(SCALE * x), (int)(SCALE * y));
      break;
      case 'C':
        g_string_append_printf (component_str,
            "    <point type='curve' x='%d' y='%d'/>\n",
            (int)(SCALE * x), (int)(SCALE * y));
      break;
        break;
    }
}

void add_gray_block (float fill_ratio, float paramA, float paramB)
{
  add_point ('L', 0, 0.5 + fill_ratio/2);
  add_point ('L', 1, 0.5 + fill_ratio/2);
  add_point ('L', 1, 0.5 - fill_ratio/2);
  add_point ('L', 0, 0.5 - fill_ratio/2);
}

void add_subpath (void)
{
  g_string_append_printf (component_str, "</contour><contour>");
}

char *mem_read (char *start,
                char *linebuf,
                int  *len);


static int map_pix (char pix)
{
  int i;
  for (i = 0; i < mappings; i++)
    if (map[i].ascii == pix)
      return map[i].type;
  return C_BLANK;
}


static const char *font_name = NULL;
static const char *font_variant = NULL;

static char *asc_source = NULL;

int rw, rh;
unsigned int *fb;
int stride;

static char ufo_path[2048];

const char *glyphs = "";

gunichar *uglyphs = NULL;
glong n_glyphs;

GString *contents_plist = NULL;
GString *str = NULL;

int glyph_height;

void write_glyph (const char *name, int advance,
                  signed long unicode,
                  const char *inner_outline)
{
  GString *str;
  char buf[1024];
  str = g_string_new ("");

  g_string_append_printf (str, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  g_string_append_printf (str, "<glyph name=\"%s\" format=\"1\">\n", name);
  g_string_append_printf (str, "  <advance width=\"%d\"/>\n", advance);
  if (unicode>= 0)
    g_string_append_printf (str, "  <unicode hex=\"%04X\"/>\n", unicode);
  g_string_append_printf (str, "  <outline>%s</outline>\n", inner_outline);
  g_string_append_printf (str, "</glyph>\n");
  sprintf (buf, "%s/glyphs/%s.glif", ufo_path, name);
  g_file_set_contents (buf, str->str, str->len, NULL);
  g_string_free (str, TRUE);
}

void gen_ref_glyph (Mapping *mapping, int xw, int xh)
{
  GString *str;
  char name[8];
  sprintf (name, "%04X", mapping->ascii);
  str = g_string_new ("");
  g_string_append_printf (str, "<component base=\"%s\" xOffset=\"0\" yOffset=\"0\" xScale=\"%d\" yScale=\"%d\"/>\n",
       mapping->name, xw, xh);
  write_glyph (name, (xw) * SCALE, mapping->ascii, str->str);
  g_string_free (str, TRUE);
}

void gen_glyph (int glyph_no, int x0, int y0, int x1, int y1)
{
  GString *str;
  gchar utf8_chr[8]={0,0,0,0,};
  int x, y;
  x0++;
  x1++;

  if (glyph_no >= n_glyphs)
    return;

  if (y1 - y0 > glyph_height)
    glyph_height = y1 - y0 - 1;
  char name[8];
  sprintf (name, "%04X", uglyphs[glyph_no]);

  g_unichar_to_utf8 (uglyphs[glyph_no], utf8_chr);
  str = g_string_new ("");

  if (overlap_solid)
  for (y = y0; y <= y1; y++)
    for (x = x0; x <= x1; x++)
      {
        int *pix = &fb[stride * y+x];
        int u = x - x0;
        int v = y1 - y -1 + y_shift;
        if (*pix == C_SOLID)
          {
            if (y <y1 && fb[stride *(y+1)+x] == C_SOLID)
              {
          g_string_append_printf (str, "  <component base=\"solidv\" xOffset=\"%d\" yOffset=\"%d\"/>\n", u * SCALE, v * SCALE); 
          if (x <x1 && fb[stride *(y)+(x+1)] == C_SOLID)
            g_string_append_printf (str, "  <component base=\"solidh\" xOffset=\"%d\" yOffset=\"%d\"/>\n", u * SCALE, v * SCALE); 
              }
            else
              {
          if (x <x1 && fb[stride *(y)+(x+1)] == C_SOLID)
            g_string_append_printf (str, "  <component base=\"solidh\" xOffset=\"%d\" yOffset=\"%d\"/>\n", u * SCALE, v * SCALE); 
          else
            g_string_append_printf (str, "  <component base=\"solid\" xOffset=\"%d\" yOffset=\"%d\"/>\n", u * SCALE, v * SCALE); 
              }
          }
      }

  for (y = y0; y <= y1; y++)
    for (x = x0; x <= x1; x++)
      {
        int *pix = &fb[stride * y+x];
        int u = x - x0;
        int v = y1 - y -1 + y_shift;
        const char *component = NULL;

        component = mapping2str (*pix);
        if (overlap_solid)
        {
          if (*pix == C_SOLID) component = NULL;
        }

        if (component)
          g_string_append_printf (str, "  <component base=\"%s\" xOffset=\"%d\" yOffset=\"%d\"/>\n", component, u * SCALE, v * SCALE);
      }

  write_glyph (name, (x1-x0+1) * SCALE, uglyphs[glyph_no], str->str);
  g_string_append_printf (contents_plist, "<key>%s</key><string>%s.glif</string>\n", name, name);

  g_string_free (str, TRUE);
}


void gen_blocks ();
void gen_fontinfo (int glyph_height);

static void str_chomp (char *string)
{
  char *p;
  
  /* trim comment */
  p = strchr (string, '#');
  if (p)
   *p = '\0';

  /* trim trailing spaces, */
  p = &string[strlen(string)-1];
  p--;
  while (p && p>string && *p == ' ')
    {
      *p = '\0';
      p--;
    }
}

void import_includes (char **asc_source)
{
  GString *new = g_string_new ("");
  char linebuf[1024];
  int len;
  char *p = *asc_source;

  do {
    p = mem_read (p, linebuf, &len);
    if (p)
      {
        if (g_str_has_prefix (linebuf, "include "))
        {
          char *read = NULL;

          str_chomp (linebuf);

          g_file_get_contents (&linebuf[strlen("include ")], &read, NULL, NULL);
          if (read)
            {
              g_string_append_printf (new, "%s", read);
              g_free (read);
            }
        }
        else if (g_str_has_prefix (linebuf, "authormode"))
        {
          author_mode = 1;
        }
        else if (g_str_has_prefix (linebuf, "y_shift "))
        {
          y_shift = atoi (&linebuf[strlen("y_shift ")]);
        }
        else if (g_str_has_prefix (linebuf, "overlap_solid "))
        {
          overlap_solid = atoi (&linebuf[strlen("overlap_solid ")]);
        }
        else if (g_str_has_prefix (linebuf, "scale "))
        {
          SCALE = atoi (&linebuf[strlen("scale ")]);
        }
        else if (g_str_has_prefix (linebuf, "variant "))
        {
          font_variant = g_strdup (&linebuf[strlen("variant ")]);
        }
        else if (g_str_has_prefix (linebuf, "fontname "))
        {
          font_name = g_strdup (&linebuf[strlen("fontname ")]);
        }
        else
        {
          g_string_append_printf (new, "%s\n", linebuf);
        }
      }
  } while (p);

  *asc_source = new->str;
  g_string_free (new, FALSE);
}

int main (int argc, char **argv)
{
  int y0 = 0, y1 = 0;

  if (argc != 2)
    {
      fprintf (stderr, "Usage: %s <fontsource.asc>\n", argv[0]);
      return -1;
    }
  if (!g_file_get_contents (argv[1], &asc_source, NULL, NULL))
    {
      fprintf (stderr, "failed to load ascii font\n");
      return -1;
    }

  import_includes (&asc_source);

  uglyphs = NULL;
  glyphs    = NULL;

  sprintf (ufo_path, "%s.ufo", font_name);
  char buf[2048];
  sprintf (buf, "mkdir %s > /dev/null 2>&1", ufo_path); system (buf);
  sprintf (buf, "mkdir %s/glyphs > /dev/null 2>&1", ufo_path); system (buf);
  sprintf (buf, "%s/metainfo.plist", ufo_path);

  g_file_set_contents (buf,
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
"<plist version=\"1.0\"> <dict> <key>creator</key> <string>org.gimp.pippin</string> <key>formatVersion</key> <integer>2</integer> </dict> </plist>", -1, NULL);


  int y, x0;
  int glyph_no = 0;

  contents_plist = g_string_new (
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\"\n"
"\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
"<plist version=\"1.0\">\n"
"<dict>\n");

  gen_blocks ();
  /* read mapping table */

  {
    char linebuf[1024];
    int len;
    char *p = asc_source;

    do {
      p = mem_read (p, linebuf, &len);
      if (len)
        {


          if (linebuf[0] == '!' && linebuf[1] == '!')
            goto mappings_done;

          switch (linebuf[0])
          {
            case '{': /* new component definition */
              {
                add_component (&linebuf[2]);
                break;
              }
            case 'L': /* line-to */
            case 'c': /* curve-to */
            case 'C': /* curve-to */
              {
                int type;
                float x, y;
                sscanf (&linebuf[0], "%c %f %f", &type, &x, &y);
                add_point (type, x, y);
                break;
              }
            case 'Z': /* new sub-path */
              add_subpath ();
              break;
            case 'G':
              {
                float arg1;
                float arg2;
                float ratio;
                sscanf (&linebuf[2], "%f %f", &ratio, &arg1, &arg2);
                add_gray_block (ratio, arg1, arg2);
                break;
              }
              break;

            default:
              finalize_component ();
              map[mappings].ascii = linebuf[0];
              if (strchr (&linebuf[2], ' '))
                *strchr (&linebuf[2], ' ')=0;
              map[mappings].name = strdup (&linebuf[2]);
              map[mappings].type = resolve_mapping_str (&linebuf[2]);
              mappings++;
          }
        }
    } while (p);

  mappings_done:
    if (0);

    int lineno = 0;
    int gotone = 0;

    int maxx = 0;
    int maxy = 0;

    do {
      p = mem_read (p, linebuf, &len);
      //str_chomp (linebuf);
      if (len)
        {
          if (linebuf[0] == '(' && linebuf[1] == ' ')
            {
              if (maxy>0)
                {
                  gen_glyph (0, 0, 0, maxx, maxy-1);
                }
              maxx = 0;
              maxy = 0;
              glyph_no = 0;
              lineno = 0;
              glyphs = &linebuf[2];
              if (uglyphs)
                free (uglyphs);
  uglyphs = g_utf8_to_ucs4 (glyphs, -1, &n_glyphs, NULL, NULL);
              if (fb)
                free (fb);
              fb = g_malloc0 (256*256 * sizeof(int));
              stride = 256;
            }
          else
            {
              int x;
              for (x = 0; linebuf[x]; x ++)
                {
                  fb[maxy * stride + x + 1] = map_pix (linebuf[x]);
                  if (x>maxx)
                    maxx=x;
                }
              maxy++;
            }
        }
    } while (p);
    if (maxy>0)
      {
        gen_glyph (0, 0, 0, maxx, maxy-1);
      }


  if (author_mode)
    {
      int i;
      for (i = 0; map[i].ascii; i++)
        {
          gen_ref_glyph (&map[i], maxy-1, maxy-1);
        }
    }
  }
 
  g_string_append (contents_plist, "</dict>\n</plist>\n");
  sprintf (buf, "%s/glyphs/contents.plist", ufo_path);
  g_file_set_contents (buf, contents_plist->str, contents_plist->len, NULL);
  gen_fontinfo (glyph_height);

  return 0;
}

void write_component (const char *name, const char *curve_xml)
{
  write_glyph (name, SCALE, -1, curve_xml);
  g_string_append_printf (contents_plist, "<key>%s</key><string>%s.glif</string>\n", name, name);
  catalog_add (name);
}

void gen_solid_block ()
{
  write_component ("blank", "");
  add_component ("solid");
  add_point ('L', 0, 1);
  add_point ('L', 1, 1);
  add_point ('L', 1, 0);
  add_point ('L', 0, 0);

  add_component ("solidv");
  add_point ('L', 0, 1);
  add_point ('L', 1, 1);
  add_point ('L', 1, -1);
  add_point ('L', 0, -1);

  add_component ("solidh");
  add_point ('L', 0, 1);
  add_point ('L', 2, 1);
  add_point ('L', 2, 0);
  add_point ('L', 0, 0);
  finalize_component ();
}

void gen_gray (GString *str, int step, int mod)
{
  int i;
  int no = 0;

#define GO 2
#define NSCALE  (SCALE + GO * 2)

  for (i = 0; i < NSCALE * 2; i++)
  {
    no ++;
    if (no % mod == 0)
    {
      g_string_append_printf (str, "    <contour>\n");
        if (i < NSCALE)
          {
            g_string_append_printf (str, "    <point type='line' x='%d' y='%d'/>\n", i - GO,   0 - GO);
            g_string_append_printf (str, "    <point type='line' x='%d' y='%d'/>\n", 0 - GO,   i - GO);
          }
        else
          {
            g_string_append_printf (str, "    <point type='line' x='%d' y='%d'/>\n", NSCALE - GO, i - NSCALE - GO);
            g_string_append_printf (str, "    <point type='line' x='%d' y='%d'/>\n", i - NSCALE - GO, NSCALE - GO);
          }

        if (i+step < NSCALE)
          {
            g_string_append_printf (str, "    <point type='line' x='%d' y='%d'/>\n", 0 - GO,   i+step - GO);
            g_string_append_printf (str, "    <point type='line' x='%d' y='%d'/>\n", i+step - GO, 0 - GO);
          }
        else
          {
            g_string_append_printf (str, "    <point type='line' x='%d' y='%d'/>\n", i+step - NSCALE - GO, NSCALE - GO);
            g_string_append_printf (str, "    <point type='line' x='%d' y='%d'/>\n", NSCALE - GO, i+step - NSCALE - GO);
          }
      g_string_append_printf (str, "    </contour>\n");
    }
  }
}

/* XXX: the invocation of these should move to the font itself */
void gen_dia_grays ()
{
  int step = 7;
  char buf[1024];
  const char *name;
  GString *str;
  name = "light";
  str = g_string_new ("");
  gen_gray (str, step, 18);
  write_component (name, str->str);
  g_string_free (str, TRUE);

  name = "strong";
  str = g_string_new ("");
  gen_gray (str, step, 10);
  write_component (name, str->str);
  g_string_free (str, TRUE);

  name = "medium";
  str = g_string_new ("");
  gen_gray (str, step, 14);
  write_component (name, str->str);
  g_string_free (str, TRUE);
}

void gen_blocks ()
{
  gen_solid_block ();
  gen_dia_grays ();
}

void gen_fontinfo (int glyph_height)
{
  char buf[2048];
  sprintf (buf, "%s/fontinfo.plist", ufo_path);
  GString *str = g_string_new ("");

  g_string_append_printf (str, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  g_string_append_printf (str, "<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n");
  g_string_append_printf (str, "<plist version=\"1.0\">\n");
  g_string_append_printf (str, "    <dict>\n");
  g_string_append_printf (str, "	<key>copyright</key>\n");
  g_string_append_printf (str, "	<string>OEyvind Kolaas pippin@gimp.org</string>\n");
  g_string_append_printf (str, "	<key>unitsPerEm</key>\n");
  g_string_append_printf (str, "	<integer>%i</integer>\n", SCALE * glyph_height);
  g_string_append_printf (str, "	<key>openTypeNameLicenseURL</key>\n");
  g_string_append_printf (str, "	<string>http://scripts.sil.org/OFL</string>\n");
  g_string_append_printf (str, "	<key>openTypeNameVersion</key>\n");
  g_string_append_printf (str, "	<string>Version 0.1</string>\n");
  g_string_append_printf (str, "	<key>postscriptFontName</key>\n");
  g_string_append_printf (str, "	<string>%s</string>\n", font_name);
  g_string_append_printf (str, "	<key>postscriptFullName</key>\n");
  g_string_append_printf (str, "	<string>%s</string>\n", font_name);
  g_string_append_printf (str, "	<key>postscriptWeightName</key>\n");
  g_string_append_printf (str, "	<string>%s</string>\n", font_variant);
  g_string_append_printf (str, "    </dict>\n");
  g_string_append_printf (str, "</plist>\n");

  g_file_set_contents (buf, str->str, str->len, NULL);
  g_string_free (str, TRUE);
}

char *mem_read (char *start,
                char *linebuf,
                int  *len)
{
    int  int_len;
    char *p = start;
    if (!len)
      len = &int_len;
    *len = 0;
    for (p = start; *p ;p++)
      {
        switch (*p)
         {
           case '#':
            while (*p && *p!='\n')p++;
            if (*p == '\n') p--;
             break;
           case '\n':
             {
               linebuf[*len]=0;
               return p+1;
             }
             break;
           case '\\':
             p++;
             if (*p)
               {
                 linebuf[(*len)++]=*p;
               }
             else
               p--;
             break;
           default:
             linebuf[(*len)++]=*p;
             break;
         }
      }
  return NULL;
}
