/*  @file openicc_config_read.c
 *
 *  libOpenICC - OpenICC Colour Management Configuration
 *
 *  @par Copyright:
 *            2011-2018 (C) Kai-Uwe Behrmann
 *
 *  @brief    OpenICC Colour Management configuration helpers
 *  @internal
 *  @author   Kai-Uwe Behrmann <ku.b@gmx.de>
 *  @par License:
 *            MIT <http://www.opensource.org/licenses/mit-license.php>
 *  @since    2011/06/27
 */

/**
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>           /* setlocale LC_NUMERIC */

#include "openicc_config.h"
#include "openicc_config_internal.h"

int main(int argc, char ** argv)
{
  openiccConfig_s * config, * db;
  const char * file_name = argc > 1 ? argv[1] : "../test.json";
  char * text = 0;
  int size = 0;
  char            ** keys = 0;
  char            ** values = 0;
  int i,j, n = 0, devices_n, flags;
  char * json, * device_class;
  const char * devices_filter[] = {OPENICC_DEVICE_CAMERA,NULL},
             * old_device_class = NULL,
             * d = NULL;
  int output = 0;
  const char * file = NULL;
  int help = 0;
  int verbose = 0;
  const char * export = NULL;

  setlocale(LC_ALL,"");
  openiccInit();

  /* handle options */
  /* declare some option choices */
  openiccOptionChoice_s i_choices[] = {{"openicc.json", _("openicc.json"), _("openicc.json"), ""},
                                    {"","","",""}};
  openiccOptionChoice_s o_choices[] = {{"0", _("Print All"), _("Print All"), ""},
                                    {"1", _("Print Camera"), _("Print Camera JSON"), ""},
                                    {"2", _("Print None"), _("Print None"), ""},
                                    {"","","",""}};

  /* declare options - the core information; use previously declared choices */
  openiccOption_s oarray[] = {
  /* type,   flags, o,   option,    key,  name,         description,         help, value_name,    value_type,               values,                                                          variable_type, output variable */
    {"oiwi", 0,     'i', "input",   NULL, _("input"),   _("Set Input"),      NULL, _("FILENAME"), openiccOPTIONTYPE_CHOICE, {.choices.list = openiccMemDup( i_choices, sizeof(i_choices) )}, openiccSTRING, {.s = &file} },
    {"oiwi", 0,     'o', "output",  NULL, _("output"),  _("Control Output"), NULL, "0|1|2",       openiccOPTIONTYPE_CHOICE, {.choices.list = openiccMemDup( o_choices, sizeof(o_choices) )}, openiccINT, {.i = &output} },
    {"oiwi", 0,     'h', "help",    NULL, _("help"),    _("Help"),           NULL, NULL,          openiccOPTIONTYPE_NONE,   {}, openiccINT, {.i = &help} },
    {"oiwi", 0,     'v', "verbose", NULL, _("verbose"), _("verbose"),        NULL, NULL,          openiccOPTIONTYPE_NONE,   {}, openiccINT, {.i = &verbose} },
    /* default option template -X|--export */
    {"oiwi", 0,     'X', "export",  NULL, NULL,         NULL,                NULL, NULL,          openiccOPTIONTYPE_CHOICE, {}, openiccSTRING,{.s=&export} },
    {"",0,0,0,0,0,0,0, NULL, openiccOPTIONTYPE_END, {},0,{}}
  };

  /* declare option groups, for better syntax checking and UI groups */
  openiccOptionGroup_s groups[] = {
  /* type,   flags, name,      description,          help, mandatory, optional, detail */
    {"oiwg", 0,     _("Mode"), _("Actual mode"),     NULL, "i",       "ov",     "io" },
    {"oiwg", 0,     _("Misc"), _("General options"), NULL, "",        "",       "Xvh" },
    {"",0,0,0,0,0,0,0}
  };

  /* Select from *version*, *manufacturer*, *copyright*, *license*, *url*,
   * *support*, *download*, *sources*, *openicc_modules_author* and
   * *documentation* what you see fit. Add new ones as needed. */
  openiccUiHeaderSection_s sections[] = {
    /* type, nick,            label, name,      , description  */
    {"oihs", "version",       NULL,  "1.0",       NULL},
    {"oihs", "documentation", NULL,  "",          _("The example tool demontrates the usage of the libOpenIcc config and options API's.")},
    {"",0,0,0,0}};

  openiccUi_s * ui = openiccUi_Create( argc, argv,
      "openicc-config-read", "OpenICC Config Read", _("Short example tool using libOpenIcc"), "openicc-logo",
      sections, oarray, groups, NULL );
  if(!ui) return 0;
  /* done with options handling */

  /* read JSON input file */
  if(file) file_name = file;
  text = openiccOpenFile( file_name, &size );
  if(!text)
  {
    openiccOptions_PrintHelp( ui->opts, ui, verbose, "%s example tool", argv[0] );
    return 0;
  }
 

  /* parse JSON */
  db = openiccConfig_FromMem( text );
  openiccConfig_SetInfo ( db, file_name );
  devices_n = openiccConfig_DevicesCount(db, NULL);
  fprintf(stderr, "Found %d devices.\n", devices_n );

  
  /* print all found key/value pairs */
  if(output == 0)
  for(i = 0; i < devices_n; ++i)
  {
    const char * d = openiccConfig_DeviceGet( db, NULL, i,
                                              &keys, &values, malloc,free );

    if(i)
      fprintf( stderr,"\n");

    n = 0; if(keys) while(keys[n]) ++n;
    fprintf( stderr, "[%d] device class:\"%s\" with %d keys/values pairs\n", i, d, n);
    for( j = 0; j < n; ++j )
    {
      fprintf(stderr, "%s:\"%s\"\n", keys[j], values[j]);
      free(keys[j]);
      free(values[j]);
    }
    free(keys); free(values);
  }

  /* get a single JSON device */
  i = 1; /* select the second one, we start counting from zero */
  d = openiccConfig_DeviceGetJSON ( db, NULL, i, 0,
                                    old_device_class, &json, malloc,free );
  config = openiccConfig_FromMem( json );
  device_class = openiccConfig_DeviceClassGet( config, malloc );
  openiccConfig_Release( &config );
  fprintf( stderr, "\ndevice class[%d]: \"%s\"\n", i, device_class);
  if(output == 0)
    printf( "%s\n", json );
  free(json);


  /* we want a single device class DB for lets say cameras */
  devices_n = openiccConfig_DevicesCount( db, devices_filter );
  fprintf(stderr, "Found %d %s devices.\n", devices_n, devices_filter[0] );
  old_device_class = NULL;
  for(i = 0; i < devices_n; ++i)
  {
    flags = 0;
    if(i != 0) /* not the first */
      flags |= OPENICC_CONFIGS_SKIP_HEADER;
    if(i != devices_n - 1) /* not the last */
      flags |= OPENICC_CONFIGS_SKIP_FOOTER;

    d = openiccConfig_DeviceGetJSON( db, devices_filter, i, flags,
                                     old_device_class, &json, malloc,free );
    old_device_class = d;
    if(output <= 1)
      printf( "%s\n", json );
    free(json);
  }

  openiccConfig_Release( &db );

  return 0;
}

