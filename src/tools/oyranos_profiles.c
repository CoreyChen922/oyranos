/** @file oyranos_profiles.c
 *
 *  Oyranos is an open source Color Management System 
 *
 *  @par Copyright:
 *            2010-2016 (C) Kai-Uwe Behrmann
 *
 *  @brief    ICC profile informations - on the command line
 *  @internal
 *  @author   Kai-Uwe Behrmann <ku.b@gmx.de>
 *  @par License:
 *            new BSD <http://www.opensource.org/licenses/BSD-3-Clause>
 *  @since    2010/11/11
 *
 *  The program informs about installed ICC profiles.
 */

#include "oyProfiles_s.h"

#include "oyranos.h"
#include "oyranos_debug.h"
#include "oyranos_helper.h"
#include "oyranos_helper_macros.h"
#include "oyranos_internal.h"
#include "oyranos_icc.h"
#include "oyranos_io.h"
#include "oyranos_config.h"
#include "oyranos_sentinel.h"
#include "oyranos_string.h"
#include "oyranos_version.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


int    installProfile                ( oyProfile_s       * ip,
                                       const char        * path,
                                       int                 is_device_profile,
                                       char              * show_text,
                                       int                 show_gui );
void* oyAllocFunc(size_t size) {return malloc (size);}

void  printfHelp (int argc OY_UNUSED, char** argv)
{
  char * version = oyVersionString(1,0);
  char * id = oyVersionString(2,0);
  char * cfg_date =  oyVersionString(3,0);
  char * devel_time = oyVersionString(4,0);

  fprintf( stderr, "\n");
  fprintf( stderr, "oyranos-profiles %s\n",
                                _("is a ICC profile information tool"));
  fprintf( stderr, "  Oyranos v%s config: %s devel period: %s\n",
                  oyNoEmptyName_m_(version),
                  oyNoEmptyName_m_(cfg_date), oyNoEmptyName_m_(devel_time) );
  if(id)
  fprintf( stderr, "  Oyranos git id %s\n", oyNoEmptyName_m_(id) );
  fprintf( stderr, "\n");
  fprintf( stderr, "  %s\n",
                                           _("Hint: search paths are influenced by the XDG_CONFIG_HOME shell variable."));
  fprintf( stderr, "\n");
  fprintf( stderr, "%s\n",                 _("Usage"));
  fprintf( stderr, "  %s\n",               _("List available ICC profiles:"));
  fprintf( stderr, "      %s -l [-f] [-e] [-acdknoi] [-u|-s|-y|-m] [-24] [--duplicates|--no-repair] \n",        argv[0]);
  fprintf( stderr, "      -f  %s\n",       _("full path and file name"));
  fprintf( stderr, "      -e  %s\n",       _("internal name"));
  fprintf( stderr, "      -a  %s\n",       _("abstract class"));
  fprintf( stderr, "      -c  %s\n",       _("color space class"));
  fprintf( stderr, "      -d  %s\n",       _("display class"));
  fprintf( stderr, "      -k  %s\n",       _("(device) link class"));
  fprintf( stderr, "      -n  %s\n",       _("named color class"));
  fprintf( stderr, "      -o  %s\n",       _("output class"));
  fprintf( stderr, "      -i  %s\n",       _("input class"));
  fprintf( stderr, "      -2  %s\n",       _("select ICC v2 profiles"));
  fprintf( stderr, "      -4  %s\n",       _("select ICC v4 profiles"));
  fprintf( stderr, "      --path=STRING  %s\n",       _("filter for string part in path"));
  fprintf( stderr, "      --meta=key;value  %s\n",       _("filter for meta tag key/value pair"));
  fprintf( stderr, "      --duplicates  %s\n",_("show profiles with duplicate profile ID"));
  fprintf( stderr, "      --no-repair   %s\n",_("skip repair of profile ID"));
  fprintf( stderr, "\n");
  fprintf( stderr, "  %s\n",               _("List search paths:"));
  fprintf( stderr, "      %s -p [-u|-s|-y|-m]\n",        argv[0]);
  fprintf( stderr, "\n");
  fprintf( stderr, "  %s\n",               _("Install ICC profile:"));
  fprintf( stderr, "      %s [--gui] --install -u|-s|-y|-m [-d] %s\n", argv[0], _("ICC_FILE_NAME"));
  fprintf( stderr, "      %s --taxi=ID [--gui] [-d] --install -u|-s|-y|-m\n", argv[0]);
  fprintf( stderr, "      -d  %s\n",       _("use device sub path"));
  fprintf( stderr, "      --gui %s\n",     _("show hints and question GUI"));
  fprintf( stderr, "      --taxi=ID %s\n", _("download ID from Taxi data base"));
  fprintf( stderr, "\n");
  fprintf( stderr, "  %s\n",               _("Print a help text:"));
  fprintf( stderr, "      %s -h\n",        argv[0]);
  fprintf( stderr, "\n");
  fprintf( stderr, "  %s\n",               _("General options:"));
  fprintf( stderr, "      -v  %s\n",       _("verbose"));
  fprintf( stderr, "      -u  %s\n",       _("user path"));
  fprintf( stderr, "      -s  %s\n",       _("linux system path"));
  fprintf( stderr, "      -y  %s\n",       _("oyranos install path"));
  fprintf( stderr, "      -m  %s\n",       _("machine specific path"));
  fprintf( stderr, "\n");
  fprintf( stderr, "  %s:\n",               _("Example"));
  fprintf( stderr, "      SAVEIFS=$IFS ; IFS=$'\\n\\b'; profiles=(`oyranos-profiles -ldf`); IFS=$SAVEIFS; for file in \"${profiles[@]}\"; do ls \"$file\"; done");
  fprintf( stderr, "\n");
  fprintf( stderr, "\n");

  if(version) oyDeAllocateFunc_(version);
  if(id) oyDeAllocateFunc_(id);
  if(cfg_date) oyDeAllocateFunc_(cfg_date);
  if(devel_time) oyDeAllocateFunc_(devel_time);
}


int main( int argc , char** argv )
{
  int error = 0;
  int list_profiles = 0, 
      list_profile_full_names = 0, list_profile_internal_names = 0,
      list_paths = 0, user_path = 0, oyranos_path = 0, system_path = 0, machine_path = 0,
      flags = 0;
  int color_space = 0,
      display = 0,
      input = 0,
      output = 0,
      abstract = 0,
      named_color = 0,
      device_link = 0;
  char ** install = 0;
  int is_device_profile = 0;
  int install_n = 0,
      show_gui = 0;
  const char * taxi_id = NULL;
  char * path = NULL,
       * meta = NULL;

#ifdef USE_GETTEXT
  setlocale(LC_ALL,"");
#endif
  oyExportStart_(EXPORT_CHECK_NO);

  if(argc >= 2)
  {
    int pos = 1;
    unsigned i;
    char *wrong_arg = 0;
    DBG_PROG1_S("argc: %d\n", argc);
    while(pos < argc)
    {
      switch(argv[pos][0])
      {
        case '-':
            for(i = 1; pos < argc && i < strlen(argv[pos]); ++i)
            switch (argv[pos][i])
            {
              case '2': flags |= OY_ICC_VERSION_2; break;
              case '4': flags |= OY_ICC_VERSION_4; break;
              case 'l': list_profiles = 1; break;
              case 'f': list_profile_full_names = 1; break;
              case 'e': list_profile_internal_names = 1; break;
              case 'c': color_space = 1; break;
              case 'd': is_device_profile = display = 1; break;
              case 'i': input = 1; break;
              case 'o': output = 1; break;
              case 'a': abstract = 1; break;
              case 'k': device_link = 1; break;
              case 'n': named_color = 1; break;
              case 'p': list_paths = 1; break;
              case 'u': user_path = 1; break;
              case 'y': oyranos_path = 1; break;
              case 's': system_path = 1; break;
              case 'm': machine_path = 1; break;
              case 'v': oy_debug += 1; break;
              case 'h':
              case '-':
                       if(i == 1)
                       {
                             if(OY_IS_ARG("install"))
                        {
                          if(pos+1 < argc)
                          {
                            if(strcmp(argv[pos+1],"-u") == 0)
                            {  user_path = 1; ++pos; }
                            else if(strcmp(argv[pos+1],"-s") == 0)
                            {  system_path = 1; ++pos; }
                            else if(strcmp(argv[pos+1],"-y") == 0)
                            {  oyranos_path = 1; ++pos; }
                            else if(strcmp(argv[pos+1],"-m") == 0)
                            {  machine_path = 1; ++pos; }
                          }
                          if(user_path + oyranos_path + system_path +
                             machine_path != 1)
                          {
                            fprintf(stderr, "%s: [-u,-s,-y,-m]\n", _("A single path option is required"));
                            exit (0);
                          }
                          ++pos;
                          /* use all following arguments as profiles */
                          while(pos < argc)
                          {
                            oyStringListAddStaticString(&install, &install_n,
                                argv[pos], oyAllocateFunc_, oyDeAllocateFunc_ );
                            ++pos;
                          }
                          if(!install_n && !taxi_id)
                          {
                            fprintf(stderr, "%s: --install [-u|-s|-y|-m] ICC_file_name(s)\n", _("File name is missed"));
                            exit (0);
                          }
                          i=100; break;
                        }
                        else if(OY_IS_ARG("gui"))
                        {
                          show_gui = 1;
                          i=100; break;
                        }
                        else if(OY_IS_ARG("taxi"))
                        { OY_PARSE_STRING_ARG2(taxi_id, "taxi"); break; }
                        else if(OY_IS_ARG("no-repair"))
                        { flags |= OY_NO_REPAIR;
                          i=100; break;
                        }
                        else if(OY_IS_ARG("meta"))
                        { OY_PARSE_STRING_ARG2(meta, "meta"); break; }
                        else if(OY_IS_ARG("path"))
                        { OY_PARSE_STRING_ARG2(path, "path"); break; }
                        else if(OY_IS_ARG("paths"))
                        { list_paths = 1; i=100; break; }
                        else if(OY_IS_ARG("duplicates"))
                        { flags |= OY_ALLOW_DUPLICATES; i=100; break; }

                       } OY_FALLTHROUGH
              default:
                        printfHelp(argc, argv);
                        exit (0);
                        break;
            }
            break;
        default:
                        printfHelp(argc, argv);
                        exit (0);
                        break;
      }
      if( wrong_arg )
      {
       fprintf(stderr, "%s %s\n", _("wrong argument to option:"), wrong_arg);
       printfHelp(argc, argv);
       exit(1);
      }
      ++pos;
    }
  } else
  {
                        printfHelp(argc, argv);
                        exit (0);
  }

  if(oy_debug)
    fprintf( stderr, "  Oyranos v%s\n",
                  oyNoEmptyName_m_(oyVersionString(1,0)));
  /* check the default paths */
  /*oyPathAdd( OY_PROFILE_PATH_USER_DEFAULT );*/


  if(list_profiles || list_paths || install_n || taxi_id)
  {
    oyProfile_s * p = 0;
    oyProfiles_s * ps = 0;
    char ** names = NULL;
    uint32_t count = 0, i;
    int accept;
    const char * t = 0;

    if(user_path || oyranos_path || system_path || machine_path)
    {
        if(user_path)
          path = oyGetInstallPath( oyPATH_ICC, oySCOPE_USER, oyAllocateFunc_ );
        if(system_path)
          path = oyGetInstallPath( oyPATH_ICC, oySCOPE_SYSTEM, oyAllocateFunc_ );
        if(oyranos_path)
          path = oyGetInstallPath( oyPATH_ICC, oySCOPE_OYRANOS, oyAllocateFunc_ );
        if(machine_path)
          path = oyGetInstallPath( oyPATH_ICC, oySCOPE_MACHINE, oyAllocateFunc_ );

      if(oy_debug)
        fprintf( stderr, "%s: %s\n", _("Search path"), path );
    }

    if(list_profiles)
    {
      fprintf(stderr, "%s:\n", _("ICC profiles"));

      if(!(list_profile_full_names || list_profile_internal_names ||
           color_space || input || display || output || abstract ||
           named_color || device_link || flags & OY_ICC_VERSION_2 ||
           flags & OY_ICC_VERSION_4 || meta != NULL))
      {
      names = oyProfileListGet_ ( NULL, flags, &count );
      for(i = 0; i < count; ++i)
        {
          const char * sfn = names[i];
          if(path && strstr(sfn, path) == NULL)
            continue;
          if(strrchr(sfn, OY_SLASH_C))
            sfn = strrchr(sfn, OY_SLASH_C) + 1;
          fprintf(stdout, "%s\n", sfn);
        }
      } else
      {
        oyProfiles_s * patterns = NULL;
        if(meta)
        {
          oyProfile_s * pattern;
          char * t = NULL;
          oyStringAddPrintf( &t, oyAllocateFunc_,oyDeAllocateFunc_, "meta:%s", meta );
          pattern = oyProfile_FromFile( t, OY_NO_LOAD, NULL );
          if(!pattern)
          {
            fprintf(stderr, "%s %s\n", _("wrong argument to option:"), meta);
            printfHelp(argc, argv);
            exit(1);
          }
          patterns = oyProfiles_New(0);
          oyProfiles_MoveIn( patterns, &pattern, -1 );
        }

        ps = oyProfiles_Create( patterns, flags, 0 );
        oyProfiles_Release( &patterns );
        count = oyProfiles_Count(ps);
        for(i = 0; i < count; ++i)
        {
          icSignature sig_class = 0;

          accept = 1;
          p = 0;
          t = 0;

          if(list_profile_full_names || list_profile_internal_names ||
             flags & OY_ICC_VERSION_2 || flags & OY_ICC_VERSION_4 || meta)
          {
            p = oyProfiles_Get( ps, i );
          }

          if( color_space || input || display || output || abstract ||
              named_color || device_link)
          {
            accept = 0;
            if(!p)
              p = oyProfiles_Get( ps, i );
            sig_class = oyProfile_GetSignature( p, oySIGNATURE_CLASS );
          }

          if(!accept)
          {
            if(color_space && sig_class == icSigColorSpaceClass)
              accept = 1;
            else if(input && sig_class == icSigInputClass)
              accept = 1;
            else if(display && sig_class == icSigDisplayClass)
              accept = 1;
            else if(output && sig_class == icSigOutputClass)
              accept = 1;
            else if(abstract && sig_class == icSigAbstractClass)
              accept = 1;
            else if(device_link && sig_class == icSigLinkClass)
              accept = 1;
            else if(named_color && sig_class == icSigNamedColorClass)
              accept = 1;
          }

          if( accept && (flags & OY_ICC_VERSION_2 || flags & OY_ICC_VERSION_4) )
          {
            icSignature vs = oyValueUInt32( oyProfile_GetSignature(p,oySIGNATURE_VERSION) );      
            char * v = (char*)&vs;
            if(!((flags & OY_ICC_VERSION_2 && (int)v[0] == 2) ||
                 (flags & OY_ICC_VERSION_4 && (int)v[0] == 4)))
              accept = 0;
          }

          if(path)
          {
            const char * sfn = oyProfile_GetFileName(p, -1);
            if(strstr(sfn, path) == NULL)
            accept = 0;
          }

          if(!list_profile_full_names && !list_profile_internal_names &&
             accept)
          {
            const char * sfn = oyProfile_GetFileName(p, -1);
            if(strrchr(sfn, OY_SLASH_C))
              sfn = strrchr(sfn, OY_SLASH_C) + 1;
            fprintf(stdout, "%s", sfn);
          }

          if(list_profile_internal_names && accept)
          {
            t = oyProfile_GetText(p, oyNAME_DESCRIPTION);
            if(t)
              fprintf(stdout, "%s", t);
          }

          if(list_profile_full_names && accept)
          {
            if(list_profile_internal_names)
              fprintf(stdout, " (");
            t = oyProfile_GetFileName(p, -1);
            if(t)
              fprintf(stdout, "%s", t);
            if(list_profile_internal_names)
              fprintf(stdout, ")");
          }

          if(accept)
            fprintf(stdout, "\n");

          oyProfile_Release( &p );
        }
        oyProfiles_Release( &ps );
      }
    }

    if(list_paths)
    {
      int n = 0;
      char ** path_names =  oyProfilePathsGet_( &n, oyAllocateFunc_ );
      fprintf(stderr, "%s:\n", _("ICC profile search paths"));
      if(path)
      {
        char * pn = oyResolveDirFileName_(path);
        if(pn)
        {
          fprintf(stdout, "%s\n", pn );
          free(pn);
        }
      } else
        for(i = 0; (int)i < n; ++i)
          fprintf(stdout, "%s\n", path_names[i]);

      oyStringListRelease_(&path_names, n, oyDeAllocateFunc_);
    }

    if(taxi_id)
    {
      oyProfile_s * ip;
      oyOptions_s * options = NULL;
      char * show_text = 0;
      const char * file_name = NULL;

      error = oyOptions_SetFromString( &options,
                                 "//" OY_TYPE_STD "/argv/TAXI_id",
                                 taxi_id,
                                 OY_CREATE_NEW );

      ip = oyProfile_FromTaxiDB( options, NULL );
      if(!ip)
      {
        STRING_ADD( show_text, _("Could not open: ") );
        STRING_ADD( show_text, file_name );
      } else
      {
        is_device_profile = 1;
        installProfile( ip, path, is_device_profile, show_text, show_gui);

        oyProfile_Release( &ip );
      }
    }

    if(install_n)
      for(i = 0; (int)i < install_n; ++i)
      {
        const char * file_name = install[i];
        oyProfile_s * ip = 0;
        char * show_text = 0;

        if(file_name && strlen(file_name) > 10 && memcmp(file_name,"http://", 7) == 0)
        {
          size_t size = 0;
          char * mem = NULL;

          mem = oyReadUrlToMemf_( &size, "r", oyAllocateFunc_,
                                  file_name );

          if(mem && size)
          {
            ip = oyProfile_FromMem( size, mem, OY_NO_CACHE_WRITE, NULL );
            oyFree_m_( mem ); size = 0;
          }

        } else
          ip = oyProfile_FromFile( file_name, OY_NO_CACHE_WRITE | flags, 0 );

        if(!ip)
        {
          STRING_ADD( show_text, _("Could not open or invalid data: ") );
          STRING_ADD( show_text, file_name );
        }

        installProfile( ip, path, is_device_profile, show_text, show_gui);

        oyProfile_Release( &ip );
      }
    if(names)
      oyStringListRelease_(&names, count, oyDeAllocateFunc_);
  }


  oyFinish_( FINISH_IGNORE_I18N | FINISH_IGNORE_CACHES );

  return error;
}


int    installProfile                ( oyProfile_s       * ip,
                                       const char        * path,
                                       int                 is_device_profile,
                                       char              * show_text,
                                       int                 show_gui )
{
  int error = 0,
      severity = 0;
      {
        const char * in = oyProfile_GetText( ip, oyNAME_DESCRIPTION );
        oyOptions_s * opts = 0;

        oyOptions_SetFromString( &opts, "////path", path, OY_CREATE_NEW );
        if(is_device_profile)
          oyOptions_SetFromString( &opts, "////device", "1", OY_CREATE_NEW );
        error = oyProfile_Install( ip, oySCOPE_USER, opts );

        if(error == oyERROR_DATA_AMBIGUITY)
        {
          if(!show_text)
          {
            STRING_ADD( show_text, _("Profile already installed") );
            STRING_ADD( show_text, ":" );
          }
          STRING_ADD( show_text, " \'" );
          if(in)
            STRING_ADD( show_text, in );
          STRING_ADD( show_text, "\'" );
          severity = oyMSG_ERROR;
        } else if(error == oyERROR_DATA_WRITE)
        {
          if(!show_text)
          {
            STRING_ADD( show_text, _("Path can not be written") );
            STRING_ADD( show_text, ":" );
          }
          STRING_ADD( show_text, " \'" );
          if(path)
            STRING_ADD( show_text, path );
          STRING_ADD( show_text, "\'" );
          severity = oyMSG_ERROR;
        } else if(error == oyCORRUPTED)
        {
          if(!show_text)
          {
            STRING_ADD( show_text, _("Profile not useable") );
            STRING_ADD( show_text, ":" );
          }
          STRING_ADD( show_text, " \'" );
          if(in)
            STRING_ADD( show_text, in );
          STRING_ADD( show_text, "\'" );
          severity = oyMSG_ERROR;
        } else if(error > 0 && !show_text)
          oyStringAddPrintf_( &show_text, oyAllocateFunc_, oyDeAllocateFunc_,
                              "%s - %d",_("Internal Error"), error );

        oyShowMessage( severity, show_text, show_gui ); 
      }

  return error;
}
