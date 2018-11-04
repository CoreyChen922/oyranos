/** @file oyranos_device.cpp
 *
 *  Oyranos is an open source Color Management System 
 *
 *  @par Copyright:
 *            2012-2018 (C) Kai-Uwe Behrmann
 *
 *  @brief    device manipulation tool
 *  @internal
 *  @author   Kai-Uwe Behrmann <ku.b@gmx.de>
 *  @par License:
 *            new BSD <http://www.opensource.org/licenses/BSD-3-Clause>
 *  @since    2012/12/07
 *
 */
/* !cc -Wall -g test_device.c -o test_device2 -L. `oyranos-config --cflags --ldstaticflags` -lm -lltdl */

/* cc -Wall -g test_device.c -o test_device2 -L. `oyranos-config --cflags --ldstaticflags` `pkg-config --cflags --libs libxml-2.0` -lm -I ../../ -I ../../API_generated/ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "oyranos.h"
#include "oyranos_db.h"
#include "oyranos_devices.h"
#include "oyranos_helper.h"
#include "oyranos_helper_macros_cli.h"
#include "oyranos_i18n.h"
#include "oyranos_icc.h"
#include "oyranos_module_internal.h"
#include "oyranos_sentinel.h"
#include "oyranos_string.h"
#include "oyranos_texts.h"
#include "oyranos_config_internal.h"
#include "oyProfiles_s.h"

#include <locale.h>

void* oyAllocFunc(size_t size) {return malloc (size);}
void  oyDeAllocFunc ( oyPointer ptr) { if(ptr) free (ptr); }


void displayHelp(char ** argv)
{
  printf("\n");
  printf("oyranos-device v%d.%d.%d %s\n",
         OYRANOS_VERSION_A,OYRANOS_VERSION_B,OYRANOS_VERSION_C,
         _("is a color profile administration tool for color devices"));
  printf("%s:\n",                 _("Usage"));
  printf("  %s\n",               _("Assign profile to device:"));
  printf("      %s -a -c class -d number [--system-wide] -p file.icc\n", argv[0]);
  printf("         -p %s\t%s\n",    _("FILE"),   _("profile file name"));
  printf("\n");
  printf("  %s\n",               _("Unassign profile from device:"));
  printf("      %s -e -c class -d number [--system-wide]\n", argv[0]);
  printf("\n");
  printf("  %s\n",               _("Setup device:"));
  printf("      %s -s -c class -d number\n", argv[0]);
  printf("\n");
  printf("  %s\n",               _("Unset device:"));
  printf("      %s -u -c class -d number\n", argv[0]);
  printf("\n");
  printf("  %s\n",               _("List device classes:"));
  printf("      %s -l [-v | --short | --class]\n", argv[0]);
  printf("         -v      \t%s\n", _("print the full module name"));
  printf("         --short \t%s\n", _("print the module ID"));
  printf("         --class \t%s\n", _("print the modules device class"));
  printf("\n");
  printf("  %s\n",               _("List devices:"));
  printf("      %s -l -c class [-d %s | --device-name %s] [-v | --short | --path] [-r]\n", argv[0], _("NUMBER"), _("NAME"));
                        /* --short argument */
  printf("         --short \t%s\n", _("print only the profile name"));
                        /* --path argument */
  printf("         --path  \t%s\n", _("print the full file name"));
  printf("\n");
  printf("  %s\n",               _("List local DB profiles for selected device:"));
  printf("      %s --list-profiles -c class -d number [--show-non-device-related]\n", argv[0]);
  printf("         --show-non-device-related\t%s\n",_("show as well non matching profiles"));
  printf("\n");
  printf("  %s\n",               _("List Taxi DB profiles for selected device:"));
  printf("      %s --list-taxi-profiles -c class -d number [--show-non-device-related]\n", argv[0]);
  printf("         --show-non-device-related\t%s\n",_("show as well non matching profiles"));
  printf("\n");
  printf("  %s\n",               _("Dump device color state:"));
  printf("      %s -f=[icc|openicc+rank-map|openicc|openicc-rank-map] [-o=file.json] -c class -d number | -j device.json [--only-db] [-m]\n", argv[0]);
  printf("         -f icc  \t%s\n",              _("write assigned ICC profile"));
  printf("         -f fallback-icc  \t%s\n",     _("create fallback ICC profile"));
  printf("         -f openicc+rank-map\t%s\n",   _("create OpenICC device color state JSON including the rank map"));
  printf("         -f openicc\t%s\n",            _("create OpenICC device color state JSON"));
  printf("            --only-db\t%s\n",_("use only DB keys for -f=openicc"));
  printf("         -f openicc-rank-map\t%s\n",   _("create OpenICC device color state rank map JSON"));
  printf("         -o %s\t%s\n",    _("FILE"),   _("write to specified file"));
  printf("         -j %s\t%s\n",    _("FILE"),   _("use device JSON alternatively to -c and -d options"));
  printf("         -k %s\t%s\n",    _("FILE"),   _("use rank map JSON alternatively to -c and -d options"));
  printf("         -m \t%s\n",       _("embedd device and driver information into ICC meta tag"));
  printf("\n");
  printf("  %s\n",               _("Show Help:"));
  printf("      %s [-h]\n", argv[0]);
  printf("\n");
  printf("  %s\n",               _("General options:"));
  printf("         -v      \t%s\n", _("verbose"));
  printf("         -c %s\t%s\n",    _("CLASS"),  _("device class"));
  printf("         -d %s\t%s\n",    _("NUMBER"), _("device position start from zero"));
  printf("         --device-name %s\t%s\n",    _("NAME"), _("alternatively specify the name of a device"));
  printf("         -r      \t%s\n", _("skip X Color Management device profile"));
  printf("         -2      \t%s\n", _("Select ICC v2 Profiles"));
  printf("         -4      \t%s\n", _("Select ICC v4 Profiles"));
  printf("\n");
  printf(_("For more information read the man page:"));
  printf("\n");
  printf("      man oyranos-device\n");
}

int main(int argc, char *argv[])
{
  int error = 0;

  /* the functional switches */
  int assign = 0;
  int erase = 0;
  int unset = 0;
  int list = 0;
  int list_profiles = 0;
  int list_taxi_profiles = 0;
  int list_rank_paths = 0;
  int show_non_device_related = 0;
  int setup = 0;
  int device_pos = -1;
  char * format = 0;
  char * output = 0;
  int only_db = 0;
  int skip_x_color_region_target = 0;
  int device_meta_tag = 0;
  char * prof_name = 0,
       * new_profile_name = 0;
  const char * device_class = 0,
             * device_json = 0,
             * rank_json = NULL;
  int verbose = 0;
  int simple = 0;

  oyProfile_s * prof = 0;
  oyConfigs_s * devices = 0;
  oyOptions_s * options = 0;
  oyConfig_s * c = 0,
             * dt = 0;
  size_t size = 0;
  const char * filename = 0,
             * device_name = 0;
  char * data = 0, *t;
  uint32_t n = 0;
  unsigned i;
  uint32_t flags = 0;
  oySCOPE_e scope = oySCOPE_USER;

  if(getenv(OY_DEBUG))
  {
    int value = atoi(getenv(OY_DEBUG));
    if(value > 0)
      oy_debug += value;
  }

#ifdef USE_GETTEXT
  setlocale(LC_ALL,"");
#endif
  oyI18NInit_();

  {
    int pos = 1;
    const char *wrong_arg = 0;
    while(pos < argc)
    {
      switch(argv[pos][0])
      {
        case '-':
            for(i = 1; i < strlen(argv[pos]); ++i)
            switch (argv[pos][i])
            {
              case '2': flags |= OY_ICC_VERSION_2; break;
              case '4': flags |= OY_ICC_VERSION_4; break;
              case 'a': assign = 1; break;
              case 'e': erase = 1; break;
              case 'c': OY_PARSE_STRING_ARG( device_class ); break;
              case 'd': OY_PARSE_INT_ARG(    device_pos ); break;
              case 'j': OY_PARSE_STRING_ARG( device_json ); break;
              case 'k': OY_PARSE_STRING_ARG( rank_json ); break;
              case 'f': OY_PARSE_STRING_ARG( format ); break;
              case 'l': list = 1; break;
              case 'm': device_meta_tag = 1; break;
              case 'o': OY_PARSE_STRING_ARG( output ); break;
              case 'p': OY_PARSE_STRING_ARG( prof_name ); break;
              case 'r': skip_x_color_region_target = 1; break;
              case 'u': unset = 1; break;
              case 'v': if(verbose) oy_debug += 1; verbose = 1; break;
              case 's': setup = 1; break;
              case 'h':
              case '-':
                        if(i == 1)
                        {
                             if(OY_IS_ARG("assign"))
                        { assign = 1; i=100; break; }
                        else if(OY_IS_ARG("erase"))
                        { erase = 1; i=100; break; }
                        else if(OY_IS_ARG("unset"))
                        { unset = 1; i=100; break; }
                        else if(OY_IS_ARG("skip-x-color-region-target"))
                        { skip_x_color_region_target = 1; i=100; break; }
                        else if(OY_IS_ARG("setup"))
                        { setup = 1; i=100; break; }
                        else if(OY_IS_ARG("format"))
                        { OY_PARSE_STRING_ARG2(format, "format"); break; }
                        else if(OY_IS_ARG("output"))
                        { OY_PARSE_STRING_ARG2(output, "output"); break; }
                        else if(OY_IS_ARG("only-db"))
                        { only_db = 1; i=100; break; }
                        else if(OY_IS_ARG("name"))
                        { OY_PARSE_STRING_ARG2(new_profile_name, "name"); break; }
                        else if(OY_IS_ARG("device-name"))
                        { OY_PARSE_STRING_ARG2(device_name, "device-name"); break; }
                        else if(OY_IS_ARG("profile"))
                        { OY_PARSE_STRING_ARG2(prof_name, "profile"); break; }
                        else if(OY_IS_ARG("list"))
                        { list = 1; i=100; break; }
                        else if(OY_IS_ARG("list-profiles"))
                        { list_profiles = 1; i=100; break; }
                        else if(OY_IS_ARG("list-taxi-profiles"))
                        { list_taxi_profiles = 1; i=100; break; }
                        else if(OY_IS_ARG("show-non-device-related"))
                        { show_non_device_related = 1; i=100; break; }
                        else if(OY_IS_ARG("class"))
                        { simple = 2; i=100; break;}
                        else if(OY_IS_ARG("path"))
                        { simple = 2; list_rank_paths = 1; i=100; break;}
                        else if(OY_IS_ARG("short"))
                        { simple = 1; i=100; break;}
                        else if(OY_IS_ARG("verbose"))
                        { if(verbose) oy_debug += 1; verbose = 1; i=100; break;}
                        else if(OY_IS_ARG("system-wide"))
                        { scope = oySCOPE_SYSTEM; i=100; break; }
                        } OY_FALLTHROUGH
              default:
                        displayHelp(argv);
                        exit (0);
                        break;
            }
            break;
        default:
                        displayHelp(argv);
                        exit (0);
      }
      if( wrong_arg )
      {
        printf("%s %s\n", _("wrong argument to option:"), wrong_arg);
        exit(1);
      }
      ++pos;
    }
    if(oy_debug) printf( "%s\n", argv[1] );
  }
  if(argc == 1)
  {
                        displayHelp(argv);
                        exit (0);
  }

#define TEST_CAT_FILE( string, fn ) {\
  size_t size = 0; \
  char * text = oyReadFileToMem_( fn, &size, oyAllocateFunc_ ); \
  if( !string || (text && strstr(text, string) != NULL)) \
  { \
    if(text) \
      fprintf( stdout, "%s \n", text ); \
    else \
      fprintf( stderr, "%s %s\n", _("File not loaded!"), fn ); \
    done = 1; \
  } \
  oyFree_m_( text ); \
  }

  if(format && strcmp(format, "openicc-rank-map") == 0 && 
     (device_class || list_rank_paths))
  {
     int files_n = 0, i, done = 0;
     const char * subdir = "color/rank-map";
     int data = oyYES,
         owner = oySCOPE_USER_SYS;
     const char * dir_string = NULL,
                * string = "config.icc_profile",
                * suffix = "json";

     char ** files = oyDataFilesGet_( &files_n, subdir, data, owner,
                                      dir_string, string, suffix,
                                      oyAllocateFunc_ );
     if(list_rank_paths)
     {
       int path_names_n = 0;
       char ** path_names = oyDataPathsGet_( &path_names_n, subdir, oyALL,
                                             owner, oyAllocateFunc_ );
       for( i = 0; i < path_names_n; ++i )
         fprintf(stdout, "%s\n", path_names[i] );

       oyStringListRelease_( &path_names, path_names_n, free );

       oyFinish_( FINISH_IGNORE_I18N | FINISH_IGNORE_CACHES );
       exit( 0 );
     }

     if(verbose)
       fprintf(stderr, "found rank maps: %d\n", files_n);
     for( i = 0; i < files_n; ++i )
     {
       const char * file = files[i];

       if(verbose)
         fprintf(stderr, "%d: %s\n", i, files[i]);

       if(strstr(file, device_class) != NULL)
       {
         char * no = NULL;
         TEST_CAT_FILE( no, file )
         done = 1;
         break;
       }
     }

     if(!done)
     for( i = 0; i < files_n; ++i )
     {
       const char * file = files[i];
       {
         TEST_CAT_FILE( device_class, file )
         if(done)
           break;
       }
     }
     oyFinish_( FINISH_IGNORE_I18N | FINISH_IGNORE_CACHES );
     exit(0);
  }

  /* resolve device_class */
  if(device_class)
  {
    /* get XCM_ICC_COLOR_SERVER_TARGET_PROFILE_IN_X_BASE */
    if(!skip_x_color_region_target)
      error = oyOptions_SetFromString( &options,
              "//"OY_TYPE_STD"/config/icc_profile.x_color_region_target", "yes",
                                     OY_CREATE_NEW );
    if(verbose)
      error = oyOptions_SetFromString( &options, "//" OY_TYPE_STD "/config/command",
                                     "properties", OY_CREATE_NEW );
    else
      error = oyOptions_SetFromString( &options, "//" OY_TYPE_STD "/config/command",
                                     "list", OY_CREATE_NEW );
    error = oyOptions_SetFromInt( &options,
                                    "//" OY_TYPE_STD "/icc_profile_flags",
                                    flags, 0, OY_CREATE_NEW );

    if(device_name)
      error = oyDeviceGet( OY_TYPE_STD, device_class, device_name, options, &c );
    else
      error = oyDevicesGet( OY_TYPE_STD, device_class, options, &devices );

    oyOptions_Release( &options );

    n = oyConfigs_Count( devices );
    if( device_pos != -1 &&
        (device_pos >= (int)n || device_pos < -1) )
    {
      device_name = 0;
      fprintf( stderr, "%s %s\n  device_class: \"%s\" device_pos: \"%d\"  %s: %d\n", _("!!! ERROR"), _("Could not resolve device."),
               device_class?device_class:"????", device_pos,
               _("Available devices"), n);
      oyFinish_( FINISH_IGNORE_I18N | FINISH_IGNORE_CACHES );
      exit(1);
    }
  }

  /* resolve device_name */
  if(device_pos != -1)
  {
    char * t;

    c = oyConfigs_Get( devices, device_pos );
    if(!c)
    {
      fprintf( stderr, "%s %s\n  device_class: \"%s\" device_pos: \"%d\"  %s: %d\n", _("!!! ERROR"), _("Could not resolve device."),
               device_class?device_class:"????", device_pos,
               _("Available devices"), n);
      oyFinish_( FINISH_IGNORE_I18N | FINISH_IGNORE_CACHES );
      exit(1);
    }

    device_name = oyConfig_FindString( c, "device_name", 0 );
    if(device_name)
    {
      t = strdup(device_name);
      device_name = t;
    }
    else
    {
      fprintf( stderr, "%s %s: %s %d. %s: %d\n", _("!!! ERROR"), _("Could not resolve device_name"),
               device_class?device_class:"????", device_pos,
               _("Available devices"), n);
      oyFinish_( FINISH_IGNORE_I18N | FINISH_IGNORE_CACHES );
      exit(1);
    }
  }

  if(device_json)
  {
    size_t json_size = 0;
    char * json_text = oyReadFileToMem_( device_json, &json_size, oyAllocateFunc_ );
    if(json_text)
    {
      error = oyOptions_SetFromString( &options, "//" OY_TYPE_STD "/config/underline_key_suffix",
                                     "xxx", OY_CREATE_NEW );
      error = oyDeviceFromJSON( json_text, options, &c );
      oyDeAllocateFunc_( json_text );
      oyOptions_Release( &options );
    }
    if(!c)
    {
      fprintf( stderr, OY_DBG_FORMAT_ "%s %s: %s  %d\n", OY_DBG_ARGS_, _("!!! ERROR"), _("Could not resolve device_json"),
               device_json, error);
      oyFinish_( FINISH_IGNORE_I18N | FINISH_IGNORE_CACHES );
      exit(1);
    }
    device_class = oyStringCopy_( oyConfig_FindString( c, "device_class", 0 ), NULL );
    if(!device_class)
      device_class = oyStringCopy_( oyConfig_FindString( c, "device_type", 0 ), NULL );
    device_name = oyConfig_FindString( c, "device_name", 0 );
    if(!device_name)
      device_name = device_json;

    if(rank_json || !oyConfig_GetRankMap(c))
    {
      oyRankMap * rank_map = NULL;
      const char * rankj = rank_json ? rank_json : device_json;

      json_size = 0;
      json_text = oyReadFileToMem_( rankj, &json_size, oyAllocateFunc_ );
      error = oyRankMapFromJSON ( json_text, NULL, &rank_map, oyAllocateFunc_ );
      oyDeAllocateFunc_( json_text );
      if(!rank_map || error || !rank_map[0].key)
      {
        char ** list = NULL;
        fprintf( stderr, OY_DBG_FORMAT_ "%s: %s: %s  %d\n", OY_DBG_ARGS_, _("WARNING"), _("Creation of rank_map failed from"), rankj, error );
        error = oyRankMapList( device_class, NULL, &list, oyAllocateFunc_ );
        if(error)
          fprintf( stderr, OY_DBG_FORMAT_ "%s: %s: %s  %d\n", OY_DBG_ARGS_, _("WARNING"), _("Creation of rank_map failed from"), device_class, error );
        if(list && list[0])
        { json_size = 0;
          json_text = oyReadFileToMem_( list[0], &json_size, oyAllocateFunc_ );
          error = oyRankMapFromJSON ( json_text, NULL, &rank_map, oyAllocateFunc_ );
          if(error)
            fprintf( stderr, OY_DBG_FORMAT_ "%s: %s: %s  %d\n", OY_DBG_ARGS_, _("WARNING"), _("Creation of rank_map failed from"), device_class, error );
        }
      }

      if(rank_map)
        oyConfig_SetRankMap( c, rank_map );

      if( (list_profiles || list_taxi_profiles) && rank_json )
      {
        fprintf( stderr, "%s: %s  %d\n", _("!!! ERROR"),
                 _("Could not resolve rank_json"), error );
        displayHelp(argv);
        exit(1);
      }
    }

    if(verbose)
    {
      fprintf( stderr, "%s: %s %s\n", _("Using JSON"), device_class, device_name );
      fprintf( stderr, "%s\n", oyOptions_GetText( *oyConfig_GetOptions( c, "db"), oyNAME_NAME ) );
    }
  }

  if(list && device_class)
  {
    char * text = NULL,
         * report = NULL;
    int i,n;

    if(!skip_x_color_region_target)
      error = oyOptions_SetFromString( &options,
              "//"OY_TYPE_STD"/config/icc_profile.x_color_region_target", "yes",
                                     OY_CREATE_NEW );
    error = oyOptions_SetFromInt( &options,
                                    "//" OY_TYPE_STD "/icc_profile_flags",
                                    flags, 0, OY_CREATE_NEW );
    n = oyConfigs_Count( devices );
    if(error <= 0)
    {
      for(i = 0; i < n || (n == 0 && c); ++i)
      {
        if(n != 0)
          c = oyConfigs_Get( devices, i );

        if( device_pos != -1 && device_pos != i )
        {
          oyConfig_Release( &c );
          continue;
        }

        if(verbose)
        printf("------------------------ %d ---------------------------\n",i);

        error = oyDeviceGetInfo( c, oyNAME_NICK, options, &text,
                                 oyAllocFunc );
        oyDeviceAskProfile2( c, options, &prof );

        if(!simple)
        {
          oyStringAddPrintf_( &report, oyAllocFunc, oyDeAllocFunc,
                              "%d: ", i );
          oyStringAddPrintf_( &report, oyAllocFunc, oyDeAllocFunc,
                              "\"%s\" ", text ? text : "???" );
          error = oyDeviceGetInfo( c, oyNAME_NAME, options, &text,
                                   oyAllocFunc );
          oyStringAddPrintf_( &report, oyAllocFunc, oyDeAllocFunc,
                              "%s%s", text ? text : "???",
                              (i+1 == n) || device_pos != -1 ? "" : "\n" );
        } else
        {
          data = oyProfile_GetMem( prof, &size, 0, oyAllocFunc);
          if(size && data)
            oyDeAllocFunc( data );
          data = 0;
          filename = oyProfile_GetFileName( prof, -1 );
          oyStringAddPrintf_( &report, oyAllocFunc, oyDeAllocFunc,
                              "%s%s", filename ? (simple == 1)?(strrchr(filename,OY_SLASH_C) ? strrchr(filename,OY_SLASH_C)+1:filename):filename : OY_PROFILE_NONE,
                              (i+1 == n) || device_pos != -1 ? "" : "\n" );
        }
        if(verbose)
        {
          error = oyDeviceGetInfo( c, oyNAME_DESCRIPTION, options, &text,
                                   oyAllocFunc );
          printf( "%s\n", text ? text : "???" );
        }

        if(text)
          free( text );

        /* verbose adds */
        if(verbose)
        {
          data = oyProfile_GetMem( prof, &size, 0, oyAllocFunc);
          if(size && data)
            oyDeAllocFunc( data );
          data = 0;
          filename = oyProfile_GetFileName( prof, -1 );
          printf( " server profile \"%s\" size: %d\n",
                  filename?filename:OY_PROFILE_NONE, (int)size );

          text = 0;
          oyDeviceProfileFromDB( c, &text, oyAllocFunc );
          printf( " DB profile \"%s\"\n  keys: %s\n",
                  text?text:OY_PROFILE_NONE,
                  oyConfig_FindString( c, "key_set_name", 0 ) ?
                      oyConfig_FindString( c, "key_set_name", 0 ) :
                      OY_PROFILE_NONE );

          oyDeAllocFunc( text );
          text = 0;

          if(oyConfig_FindString( c, "supported_devices_info", NULL ))
          {
            oyOption_s * o = oyConfig_Find( c, "supported_devices_info" );
            int j = 0;
            const char * t = NULL;
            oyStringAddPrintf_( &report, oyAllocFunc, oyDeAllocFunc, "\n\n" );
            while((t = oyOption_GetValueString( o, j++ )) != NULL)
            {
              oyStringAddPrintf_( &report, oyAllocFunc, oyDeAllocFunc,
                                  "%s\n", t );
            }
          }
        }

        oyProfile_Release( &prof );
        if(n != 0)
          oyConfig_Release( &c );
        else
          break;
      }

      if(report)
        fprintf( stdout, "%s\n", report );
      oyDeAllocFunc( report ); report = 0;
    }
    oyConfigs_Release( &devices );
    oyOptions_Release( &options );

    oyFinish_( FINISH_IGNORE_I18N | FINISH_IGNORE_CACHES );
    exit(0);

  } else
  if( list )
  {
    uint32_t count = 0,
           * rank_list = 0;
    char ** texts = 0,
          * temp = 0,
         ** attributes = 0,
          * device_class = 0;
    int i,j, attributes_n;
    char separator;

    /* get all configuration filters */
    oyConfigDomainList("//"OY_TYPE_STD"/device", &texts, &count,&rank_list ,0 );
    for( i = 0; i < (int)count; ++i )
    {
      attributes_n = 0;
 
      /* pick the filters name and remove the common config part */
      temp = oyFilterRegistrationToText( texts[i], oyFILTER_REG_APPLICATION,
                                         malloc );
      attributes = oyStringSplit( temp, '.', &attributes_n, malloc );
      free(temp);
      temp = malloc(1024); temp[0] = 0;
      for(j = 0; j < attributes_n; ++j)
      {
        if(strcmp(attributes[j], "config") == 0)
          continue;

        if(j && temp[0])
          sprintf( &temp[strlen(temp)], "." );
        sprintf( &temp[strlen(temp)], "%s", attributes[j]);
      }

      /*  The string in temp can be passed as the device_class argument to
       *  oyDevicesGet().
       */
      if(verbose)
        fprintf( stdout, "%d: %s \"%s\"\n", i, texts[i], temp);
      else
      {
        if(simple)
          separator = '.';
        else
          separator = '/';

        if(simple == 2 && strrchr(texts[i],separator))
        {
          oyConfDomain_s * domain = oyConfDomain_FromReg( texts[i], 0 );
          const char * device_class = oyConfDomain_GetText( domain, "device_class", oyNAME_NICK );
          oyConfDomain_Release( &domain );
		
          fprintf( stdout, "%s\n", device_class );
        }
        else if(strrchr(texts[i],separator))
          fprintf( stdout, "%s\n", strrchr(texts[i],separator) + 1);
        else
          fprintf( stdout, "%s\n", texts[i]);
      }

      oyStringListRelease_( &attributes, attributes_n, free );
      free (device_class);
      free(temp);
    }

    oyFinish_( FINISH_IGNORE_I18N | FINISH_IGNORE_CACHES );
    exit(0);

  } else
  if( (setup || unset || erase || assign) &&
      c )
  {
    oyProfile_s * profile = 0;
    const char * tmp = 0;

    error = oyOptions_SetFromInt( &options,
                                    "//" OY_TYPE_STD "/icc_profile_flags",
                                    flags, 0, OY_CREATE_NEW );
    error = oyDeviceAskProfile2( c, options, &profile );
    oyOptions_Release( &options );

    if(profile)
      tmp = oyProfile_GetFileName( profile, -1 );

    fprintf( stdout, "%s %s %s %s%s%s\n",
            device_class, device_name, prof_name, error?"wrong":"good",
            tmp?"\n has already a profile: ":"", tmp?tmp:"" );

    if(assign && prof_name)
    {
      if(strcmp(prof_name,"") == 0 ||
         strcmp(prof_name,"automatic") == 0)
      {
        if(!device_json)
        {
          /* start with complete device info */
          oyConfig_Release( &c );
          if(!skip_x_color_region_target)
            oyOptions_SetFromString( &options,
                   "//"OY_TYPE_STD"/config/icc_profile.x_color_region_target",
                         "yes", OY_CREATE_NEW );
          error = oyOptions_SetFromString( &options, "//" OY_TYPE_STD "/config/command",
                                   "properties", OY_CREATE_NEW );  
          error = oyOptions_SetFromInt( &options,
                                    "//" OY_TYPE_STD "/icc_profile_flags",
                                    flags, 0, OY_CREATE_NEW );
          error = oyDeviceGet( 0, device_class, device_name, options, &dt );
          if(dt)
          {
            oyConfig_Release( &c );
            c = dt; dt = 0;
          }
          oyOptions_Release( &options );
        }

        /*error = oyDeviceSetProfile( c, scope, NULL ); no profile name not supported*/
        error = oyDeviceUnset( c );
        error = oyConfig_EraseFromDB( c, scope );

        if(!device_json)
        {
          oyConfig_Release( &c );
          if(!skip_x_color_region_target)
            oyOptions_SetFromString( &options,
                   "//"OY_TYPE_STD"/config/icc_profile.x_color_region_target",
                         "yes", OY_CREATE_NEW );
          error = oyOptions_SetFromString( &options, "//" OY_TYPE_STD "/config/command",
                                   "properties", OY_CREATE_NEW );  
          error = oyOptions_SetFromInt( &options,
                                    "//" OY_TYPE_STD "/icc_profile_flags",
                                    flags, 0, OY_CREATE_NEW );
          error = oyDeviceGet( 0, device_class, device_name, options, &dt );
          if(dt)
          {
            oyConfig_Release( &c );
            c = dt; dt = 0;
          }
          oyOptions_Release( &options );
        }
      } else
      {
        error = oyDeviceSetProfile( c, scope, prof_name );
        if(error)
          fprintf( stdout, "profile assignment failed\n" );
        error = oyDeviceUnset( c );
      }

      error = oyOptions_SetFromInt( &options,
                                    "//" OY_TYPE_STD "/icc_profile_flags",
                                    flags, 0, OY_CREATE_NEW );
      error = oyOptions_SetFromString( &options,
                                      "//"OY_TYPE_STD"/config/skip_ask_for_profile", "yes", OY_CREATE_NEW );
      error = oyDeviceSetup( c, options );
    }
    else if(assign)
    {
      displayHelp(argv);
      exit (1);
    }

    if(unset || erase)
      oyDeviceUnset( c );

    if(setup)
    {
      error = oyOptions_SetFromInt( &options,
                                    "//" OY_TYPE_STD "/icc_profile_flags",
                                    flags, 0, OY_CREATE_NEW );
      error = oyOptions_SetFromString( &options,
                                      "//"OY_TYPE_STD"/config/skip_ask_for_profile", "yes", OY_CREATE_NEW );
      oyDeviceSetup( c, options );
    }

    if(erase)
      oyConfig_EraseFromDB( c, scope );

    oyConfig_Release( &c );
    oyFinish_( FINISH_IGNORE_I18N | FINISH_IGNORE_CACHES );
    exit(0);

  } else
  if((list_profiles || list_taxi_profiles) && (c || (device_class && device_name)))
  {
    oyProfile_s * profile = 0;
    const char * tmp = 0;
    icSignature profile_class = icSigDisplayClass;
    oyOptions_s * options = 0;

    {
      oyConfDomain_s * d = oyConfDomain_FromReg( device_class, 0 );
      const char * icc_profile_class = oyConfDomain_GetText( d,
                                             "icc_profile_class", oyNAME_NICK );
      if(icc_profile_class && strcmp(icc_profile_class,"display") == 0)
        profile_class = icSigDisplayClass;
      else if(icc_profile_class && strcmp(icc_profile_class,"output") == 0)
        profile_class = icSigOutputClass;
      else if(icc_profile_class && strcmp(icc_profile_class,"input") == 0)
        profile_class = icSigInputClass;
      else if(strcmp(device_class,"monitor") == 0)
        profile_class = icSigDisplayClass;
      else if(strcmp(device_class,"printer") == 0)
        profile_class = icSigOutputClass;
      else if(strcmp(device_class,"camera") == 0)
        profile_class = icSigInputClass;
      else if(strcmp(device_class,"scanner") == 0)
        profile_class = icSigInputClass;

      if(verbose)
        fprintf( stderr, "icc_profile_class: %s\n", icc_profile_class ? icc_profile_class : device_class );
      oyConfDomain_Release( &d );
    }

    if(!device_json)
    {
      /* get all device informations from the module */
      oyConfig_Release( &c );
      if(!skip_x_color_region_target)
        oyOptions_SetFromString( &options,
                   "//"OY_TYPE_STD"/config/icc_profile.x_color_region_target",
                         "yes", OY_CREATE_NEW );
      error = oyOptions_SetFromString( &options, "//" OY_TYPE_STD "/config/command",
                                     "properties", OY_CREATE_NEW );  
      error = oyOptions_SetFromInt( &options,
                                    "//" OY_TYPE_STD "/icc_profile_flags",
                                    flags, 0, OY_CREATE_NEW );
      error = oyDeviceGet( 0, device_class, device_name, options, &c );
      oyOptions_Release( &options );
    }
    if(!c)
    {
      fprintf( stderr, "%s %s\n  device_class: \"%s\" device_name: \"%s\"  %s: %d\n", _("!!! ERROR"), _("Could not resolve device."),
               device_class?device_class:"????", device_name,
               _("Available devices"), n);
      
      exit (1);
    }

    if(!skip_x_color_region_target)
    oyOptions_SetFromString( &options,
                   "//"OY_TYPE_STD"/config/icc_profile.x_color_region_target",
                         "yes", OY_CREATE_NEW );
    error = oyOptions_SetFromInt( &options,
                                    "//" OY_TYPE_STD "/icc_profile_flags",
                                    flags, 0, OY_CREATE_NEW );

    error = oyDeviceGetProfile( c, options, &profile );

    if(profile)
      tmp = oyProfile_GetFileName( profile, -1 );

    if(verbose)
      fprintf( stderr, "%s %s %s%s%s\n",
               device_class, device_name, error?"":"good",
               tmp?"\nassigned profile: ":"", tmp?tmp:"" );

    oyProfile_Release( &profile );

    if(list_profiles)
    {
      int size, i, current = -1, current_tmp = 0, pos = 0;
      oyProfile_s * profile = 0, * temp_profile = 0;
      oyProfiles_s * patterns = 0, * iccs = 0;
      const char * profile_file_name = 0;
      int32_t * rank_list;
      int empty_added;

      profile = oyProfile_FromSignature( profile_class, oySIGNATURE_CLASS, 0 );
      patterns = oyProfiles_New( 0 );
      oyProfiles_MoveIn( patterns, &profile, -1 );

      iccs = oyProfiles_Create( patterns, flags, 0 );
      oyProfiles_Release( &patterns );


      size = oyProfiles_Count(iccs);
      if(verbose)
        fprintf( stderr, "%s %s: %d\n",
                 _("found profiles for device class"), device_class, size );
      rank_list = (int32_t*) malloc( size * sizeof(int32_t) );
      oyProfiles_DeviceRank( iccs, c, rank_list );

      size = oyProfiles_Count(iccs);

      error = oyDeviceGetProfile( c, options, &profile );
      profile_file_name = oyProfile_GetFileName( profile, 0 );

      empty_added = -1;

      for( i = 0; i < size; ++i)
      {
        const char * temp_profile_file_name, * description;
        {
           temp_profile = oyProfiles_Get( iccs, i );
           description = oyProfile_GetText( temp_profile, oyNAME_DESCRIPTION );
           temp_profile_file_name = oyProfile_GetFileName( temp_profile, 0);

           current_tmp = -1;

           if(profile_file_name && temp_profile_file_name &&
              strcmp( profile_file_name, temp_profile_file_name ) == 0)
             current_tmp = pos;

           if(current == -1 && current_tmp != -1)
             current = current_tmp;

           if(empty_added == -1 &&
              rank_list[i] < 1)
           {
             fprintf(stdout, "automatic\n");
             empty_added = pos;
             if(current != -1 &&
                current == pos)
               ++current;
             ++pos;
           }

           if(show_non_device_related == 1 ||
              rank_list[i] > 0 ||
              current_tmp != -1)
           {
             fprintf( stdout, "[%d] %s (%s)\n",
                  rank_list[i], description, temp_profile_file_name);
 
             ++pos;
           }
        }
        oyProfile_Release( &temp_profile );
      }
      if(empty_added == -1)
      {
        ++pos;
        if(current == -1 && current_tmp != -1)
          current = pos;
      }
      if(verbose)
        fprintf( stderr, "current: %d\n", current );

      oyProfile_Release( &profile );
      oyProfiles_Release( &iccs );
      oyOptions_Release( &options );

    } else if(list_taxi_profiles)
    {
      int size, i, current = -1, current_tmp = 0, pos = 0;
      oyProfile_s * profile = 0;
      oyConfigs_s * taxi_devices = 0;
      oyConfig_s * device = c;

      if(verbose)
        fprintf( stderr, "%s [%s] \"%s\"\n", _("Taxi ID"),
                 _("match value"), _("description") );

      oyDevicesFromTaxiDB( device, 0, &taxi_devices, 0 );

      size = oyConfigs_Count( taxi_devices );

      error = oyDeviceGetProfile( device, options, &profile );

      for( i = 0; i < size; ++i)
      {
        {
           oyConfig_s * taxi_dev = oyConfigs_Get( taxi_devices, i );
           int32_t rank = 0;
           error = oyDeviceCompare( device, taxi_dev, &rank );

           current_tmp = -1;

           if(current == -1 && current_tmp != -1)
             current = current_tmp;


           if(show_non_device_related == 1 ||
              rank > 0 ||
              current_tmp != -1)
           {
             fprintf(stdout, "%s/0 [%d] ", oyNoEmptyString_m_(
                    oyConfig_FindString(taxi_dev, "TAXI_id", 0)), rank);
             fprintf(stdout, "\"%s\"\n", oyNoEmptyString_m_(
                    oyConfig_FindString(taxi_dev, "TAXI_profile_description", 0)));
             ++pos;
           }

           oyConfig_Release( &taxi_dev );
        }  
      } 
      oyProfile_Release( &profile );                                                
      oyConfigs_Release( &taxi_devices );
      oyOptions_Release( &options );
    }

    oyConfig_Release( &c );
    oyFinish_( FINISH_IGNORE_I18N | FINISH_IGNORE_CACHES );
    exit(0);


  } else if(format && c)
  {
    oyConfDomain_s * d = oyConfDomain_FromReg( device_class, 0 );
    char * json = 0;
    char * profile_name = 0;
    char * out_name = 0;
    oyjl_val rank_root = 0,
             rank_map = 0,
             device = 0;

    if(!device_json)
    {
      /* get all device informations from the module */
      if(!skip_x_color_region_target)
        oyOptions_SetFromString( &options,
                   "//"OY_TYPE_STD"/config/icc_profile.x_color_region_target",
                         "yes", OY_CREATE_NEW );
      error = oyOptions_SetFromInt( &options,
                                    "//" OY_TYPE_STD "/icc_profile_flags",
                                    flags, 0, OY_CREATE_NEW );
      error = oyOptions_SetFromString( &options, "//" OY_TYPE_STD "/config/command",
                                     "properties", OY_CREATE_NEW );  
      error = oyDeviceGet( 0, device_class, device_name, options, &dt );
      if(dt)
      {
        oyConfig_Release( &c );
        c = dt; dt = 0;
      }
      oyOptions_Release( &options );
    }
    if(!c)
    {
      fprintf( stderr, "%s %s\n  device_class: \"%s\" device_name: \"%s\"  %s: %d\n", _("!!! ERROR"), _("Could not resolve device."),
               device_class?device_class:"????", device_name,
               _("Available devices"), n);
      
      exit (1);
    }

    /* query the full device information from DB */
    error = oyDeviceProfileFromDB( c, &profile_name, oyAllocFunc );
    if(profile_name) { oyDeAllocFunc( profile_name ); profile_name = 0; }

    if(strcmp(format,"openicc") == 0 ||
       strcmp(format,"openicc+rank-map") == 0 ||
       strcmp(format,"openicc-rank-map") == 0)
    {
      if(strcmp(format,"openicc") == 0 ||
         strcmp(format,"openicc+rank-map") == 0)
      {
        oyOptions_s * parsed = NULL;
        char * path = NULL;
        oyStringAddPrintf( &path, 0,0, "org/freedesktop/openicc/device/%s", device_class );
        error = oyOptions_SetFromString( &options, "//" OY_TYPE_STD "/options/source",
                                   "db", OY_CREATE_NEW );  
        error = oyOptions_SetFromString( &options, "//" OY_TYPE_STD "/key_path",
                                       path, OY_CREATE_NEW );  
        error = oyDeviceToJSON( c, options, &json, oyAllocFunc );
        if(oyOptions_FromJSON( json, options, &parsed, "org/freedesktop/openicc/device/%s/[0]", device_class ))
          WARNc1_S( "%s\n", _("found issues parsing JSON") );
        if(json) oyFree_m_( json );

        /* it is possible that no DB keys are available; use all others */
        if(!only_db)
        {
          error = oyOptions_SetFromString( &options, "//" OY_TYPE_STD "/options/source",
                                         "backend_core", OY_CREATE_NEW );
          error = oyDeviceToJSON( c, options, &json, oyAllocFunc );
          if(oyOptions_FromJSON( json, options, &parsed, "org/freedesktop/openicc/device/%s/[0]", device_class ))
            WARNc1_S( "%s\n", _("found issues parsing JSON") );
          if(json) oyFree_m_( json );
        }
        oyOptions_Release( &options );

        json = oyStringCopy( oyOptions_GetText( parsed, oyNAME_JSON ), 0 );

        if(!json)
        {
          fprintf( stderr, "no DB data available\n" );
          oyFinish_( FINISH_IGNORE_I18N | FINISH_IGNORE_CACHES );
          exit(0);
        }

        t = oyAllocateFunc_(256);
        device = oyjlTreeParse( json, t, 256 );
        if(t[0])
          WARNc2_S( "%s: %s\n", _("found issues parsing JSON"), t );
        oyFree_m_(t);
      }
      if(strcmp(format,"openicc-rank-map") == 0 ||
         strcmp(format,"openicc+rank-map") == 0)
      {
        const oyRankMap * map = oyConfig_GetRankMap( c );
        oyConfDomain_s * domain = oyConfDomain_FromReg( device_class, 0 );
        const char * device_class = oyConfDomain_GetText( domain, "device_class", oyNAME_NICK );
        oyConfDomain_Release( &domain );

        if(!map)
        { fprintf( stderr, "no RankMap found\n" ); exit(0);
        }

        error = oyOptions_SetFromString( &options, "//" OY_TYPE_STD "/options/device_class",
                                       device_class, OY_CREATE_NEW );  
        oyRankMapToJSON( map, options, &json, oyAllocFunc );
        oyOptions_Release( &options );
        if(!json)
        { fprintf( stderr, "no JSON from RankMap available\n" ); exit(0);
        }

        t = oyAllocateFunc_(256);
        rank_root = oyjlTreeParse( json, t, 256 );
        if(t[0])
          WARNc2_S( "%s: %s\n", _("found issues parsing JSON"), t );
        oyFree_m_(t);
      }

      if(strcmp(format,"openicc+rank-map") == 0)
      {
        const char * xpath = "org/freedesktop/openicc/rank_map";

        /*oyjl_message_func_set( (oyjl_message_f)oyMessageFunc );*/

        rank_map = oyjlTreeGetValue( rank_root, 0, xpath );
        if(rank_map && rank_map->type == oyjl_t_object)
        {
          oyjl_val openicc = oyjlTreeGetValue( device, 0, "org/freedesktop/openicc" );
          /* copy the rank_map into the openicc node */
          if(openicc && openicc->type == oyjl_t_object)
          {
            oyjl_val * values;
            int level = 0;
            char ** keys;

            oyDeAllocFunc( json ); json = 0;
            oyjlTreeToJson( rank_map, &level, &json );
            rank_map = oyjlTreeParse( json, 0,0 );
            if(json){ free(json); json = 0; }

            keys = openicc->u.object.keys;
            values = openicc->u.object.values;

            openicc->u.object.keys = malloc( sizeof(char*) * openicc->u.object.len + 1 );
            openicc->u.object.values = malloc( sizeof(oyjl_val) * openicc->u.object.len + 1 );

            if(rank_map && rank_map->type == oyjl_t_object &&
               openicc->u.object.values && openicc->u.object.keys)
            {
              openicc->u.object.values[0] = rank_map;
              memcpy( &openicc->u.object.values[1], values, sizeof(oyjl_val) * openicc->u.object.len );
              openicc->u.object.keys[0] = oyStringCopy_("rank_map", oyAllocFunc);
              memcpy( &openicc->u.object.keys[1], keys, sizeof(char*) * openicc->u.object.len );

              ++openicc->u.object.len;

              /* level = 0; */
              oyjlTreeToJson( device, &level, &json );
            }
          }
        }

        oyjlTreeFree( rank_root ); rank_root = 0;
        oyjlTreeFree( device ); device = 0;
      }

      if(output)
        error = oyWriteMemToFile2_( output,
                                    json, strlen(json), 0x01,
                                    &out_name, oyAllocFunc );
      else
        fprintf( stdout, "%s", json );

      if(json)
        size = strlen(json);
      oyDeAllocFunc( json ); json = 0;

    } else
    if(strcmp(format,"icc") == 0 ||
       strcmp(format,"fallback-icc") == 0)
    {
      if(strcmp(format,"fallback-icc") == 0)
        oyOptions_SetFromString( &options,
                   "//"OY_TYPE_STD"/config/icc_profile.fallback",
                         "yes", OY_CREATE_NEW );
      if(!skip_x_color_region_target)
        oyOptions_SetFromString( &options,
                   "//"OY_TYPE_STD"/config/icc_profile.x_color_region_target",
                         "yes", OY_CREATE_NEW );
      error = oyOptions_SetFromInt( &options,
                                    "//" OY_TYPE_STD "/icc_profile_flags",
                                    flags, 0, OY_CREATE_NEW );
      error = oyOptions_SetFromString( &options, "//" OY_TYPE_STD "/config/command",
                                     "properties", OY_CREATE_NEW );  
      error = oyDeviceGet( 0, device_class, device_name, options, &dt );
      if(dt)
      {
        oyConfig_Release( &c );
        c = dt; dt = 0;
      }
      if(strcmp(format,"fallback-icc") == 0)
      {
        icHeader * header;
        oyOption_s * o;

        o = oyOptions_Find( *oyConfig_GetOptions(c, "data"),
                            "icc_profile.fallback", oyNAME_PATTERN );
        if( o )
        {
          prof = (oyProfile_s*) oyOption_GetStruct( o, oyOBJECT_PROFILE_S );
          oyOption_Release( &o );
        } else
          error = oyDeviceAskProfile2( c, options, &prof );

        if(prof)
        {
          uint32_t model_id = 0;
          const char * t = 0;
          error = oyProfile_AddTagText( prof, icSigProfileDescriptionTag,
                                        (char*) output );
          t = oyConfig_FindString( c, "manufacturer", 0 );
          if(t)
            error = oyProfile_AddTagText( prof, icSigDeviceMfgDescTag, t );
          t =  oyConfig_FindString( c, "model", 0 );
          if(t)
            error = oyProfile_AddTagText( prof, icSigDeviceModelDescTag, t);

          if(device_meta_tag)
          {
            oyOptions_s * opts = 0;
            t = oyConfig_FindString( c, "prefix", 0 );
            error = oyOptions_SetFromString( &opts, "///key_prefix_required",
                                                  t, OY_CREATE_NEW );
            oyProfile_AddDevice( prof, c, opts );
            oyOptions_Release( &opts );
          }

          data = oyProfile_GetMem( prof, &size, 0, oyAllocFunc );
          header = (icHeader*) data;
          t = oyConfig_FindString( c, "mnft", 0 );
          if(t)
            sprintf( (char*)&header->manufacturer, "%s", t );
          t = oyConfig_FindString( c, "model_id", 0 );
          if(t)
            model_id = atoi( t );
          model_id = oyValueUInt32( model_id );
          memcpy( &header->model, &model_id, 4 );
          oyOption_Release( &o );
        }
      } else /* strcmp(format,"icc") == 0 */
        oyDeviceAskProfile2( c, options, &prof );

      oyOptions_Release( &options );

      data = oyProfile_GetMem( prof, &size, 0, oyAllocFunc);
      if(size && data)
      {
        if(output)
          error = oyWriteMemToFile2_( output,
                                      data, size, 0x01,
                                      &out_name, oyAllocFunc );
        else
          fwrite( data, sizeof(char), size, stdout );

        oyDeAllocFunc( data ); data = 0;

      }
      oyOptions_Release( &options );

    } else {
      fprintf( stderr, "%s unsupported format: %s\n", _("!!! ERROR"), format );
      exit (1);
    }

    if(verbose || (error && !size) || !size)
      fprintf( stderr, "  written %d bytes to %s\n", (int)size,
               out_name ? out_name : "stdout" );

    if(out_name){ oyDeAllocFunc(out_name); out_name = 0; }
    oyConfDomain_Release( &d );

    oyFinish_( FINISH_IGNORE_I18N | FINISH_IGNORE_CACHES );
    exit(0);
  }


  /* This point should not be reached */
  displayHelp(argv);
  exit (1);

  /* device profile */
  if(0)
  {
    int i,n, pos = 0;
    oyProfileTag_s * tag_ = 0;
    oyConfig_s * oy_device = 0;
    oyOption_s * o = 0;
    char * text = 0, * name = 0;
    icSignature vs;
    char * v = 0;
    icTagTypeSignature texttype;
    oyProfile_s * p = 0;
    oyConfig_s * device;
    oyProfiles_s * p_list;
    int32_t * rank_list;

    error = oyDeviceGet( 0, device_class, device_name, 0, &oy_device );
    /* pick expensive informations */
    oyDeviceGetInfo( oy_device, oyNAME_DESCRIPTION, 0, &text, oyAllocateFunc_);
    oyDeAllocateFunc_( text );
    error = oyDeviceGetProfile( oy_device, 0, &p );

    vs = oyValueUInt32( oyProfile_GetSignature(p,oySIGNATURE_VERSION) );
    v = (char*)&vs;
    if(v[0] <= 2)
      texttype = icSigTextDescriptionType;
    else
      texttype = (icTagTypeSignature) icSigMultiLocalizedUnicodeType;

    n = oyOptions_Count( *oyConfig_GetOptions( oy_device,"backend_core") );

    for(i = 0; i < n; ++i)
    {
      o = oyOptions_Get( *oyConfig_GetOptions( oy_device,"backend_core"), i );

      text = oyOption_GetValueText( o, oyAllocateFunc_ );
      if(!text) continue;

      name = oyFilterRegistrationToText( oyOption_GetRegistration(o),
                                         oyFILTER_REG_MAX, oyAllocateFunc_ );
      if(strstr(name, "manufacturer"))
      {
        /* add a Manufacturer desc tag */
        tag_ = oyProfileTag_CreateFromText( text, texttype,
                                            icSigDeviceMfgDescTag, 0 );
        error = !tag_;
        if(tag_)
          error = oyProfile_TagMoveIn ( p, &tag_, -1 );

        oyDeAllocateFunc_( name );
        oyDeAllocateFunc_( text );
        continue;

      } else if(strstr(name, "model"))
      {

        /* add a Device Model desc tag */
        tag_ = oyProfileTag_CreateFromText( text, texttype,
                                            icSigDeviceModelDescTag, 0 );
        error = !tag_;
        if(tag_)
          error = oyProfile_TagMoveIn ( p, &tag_, -1 );

        oyDeAllocateFunc_( name );
        oyDeAllocateFunc_( text );
        continue;

      }

      oyDeAllocateFunc_( text );

      text = oyFilterRegistrationToText( oyOption_GetRegistration(o),
                                         oyFILTER_REG_MAX, oyAllocateFunc_ );
      oyDeAllocateFunc_( text );
      ++pos;
    }

    oyProfile_Release( &p );

    device = oyConfig_FromRegistration( "//" OY_TYPE_STD "/config", 0 );
    oyProfile_GetDevice( p, device );

    printf("following key/values are in the devices backend_core set:\n");
    n = oyOptions_Count( *oyConfig_GetOptions( oy_device,"backend_core") );
    for(i = 0; i < n; ++i)
    {
      o = oyOptions_Get( *oyConfig_GetOptions( oy_device,"backend_core"), i );

      text = oyOption_GetValueText( o, oyAllocateFunc_ );
      if(!text) continue;

      printf("%s: %s\n", oyOption_GetRegistration(o), text);

      oyOption_Release( &o );
    }

    printf("\ngoing to rank installed profiles according to the device[\"%s\",\"%s\"]:\n", device_class, device_name );
    p_list = oyProfiles_ForStd( oyASSUMED_RGB, flags, 0,0 );
    rank_list = (int32_t*) oyAllocateFunc_( oyProfiles_Count(p_list)
                                                        * sizeof(int32_t) );
    oyProfiles_DeviceRank( p_list, oy_device, rank_list );
    n = oyProfiles_Count( p_list );
    for(i = 0; i < n; ++i)
    {
      prof = oyProfiles_Get( p_list, i );
      printf("%d %d: \"%s\" %s\n", rank_list[i], i,
             oyProfile_GetText( prof, oyNAME_DESCRIPTION ),
             oyProfile_GetFileName(prof, 0));
      oyProfile_Release( &prof );
    }
  }

  oyFinish_( FINISH_IGNORE_I18N | FINISH_IGNORE_CACHES );

  return 0;
}

