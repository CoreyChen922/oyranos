/** @file oyranos-monitor-white-point.c
 *
 *  Oyranos is an open source Color Management System 
 *
 *  Copyright (C) 2017-2018  Kai-Uwe Behrmann
 *
 */

/**
 *  @brief    night vision admin tool
 *  @internal
 *  @author   Kai-Uwe Behrmann <ku.b@gmx.de> and others
 *  @par License:
 *            BSD-3-Clause <http://www.opensource.org/licenses/BSD-3-Clause>
 *  @since    2017/09/19
 *
 *  A tool to set the white point dependent on local state of the sun.
 *  
 */

# include <stddef.h>

#define __USE_POSIX2 1
#include <stdio.h>                /* popen() */
#include <math.h>
#include <string.h>
#include <time.h>
#include <unistd.h> /* usleep() */

#include "bb_100K.h"
#include "oyranos_color.h"
#include "oyranos_debug.h"
#include "oyranos_helper.h"
#include "oyranos_helper_macros.h"
#include "oyranos_internal.h"
#include "oyranos_json.h"
#include "oyranos_config.h"
#include "oyranos_version.h"
#include "oyranos_sentinel.h"
#include "oyranos_string.h"
#include "oyranos_texts.h"
#include "oyranos_threads.h"

int __sunriset__( int year, int month, int day, double lon, double lat,
                  double altit, int upper_limb, double *trise, double *tset );
int findLocation(oySCOPE_e scope, int dry);
int getLocation( double * lon, double * lat);
double getSunHeight( double year, double month, double day, double gmt_hours, double lat, double lon );
int getSunriseSunset( double * rise, double * set, int dry );
int runDaemon(int dmode);
int setWtptMode( oySCOPE_e scope, int wtpt_mode, int dry );
void pingNativeDisplay();
int checkWtptState();
void updateVCGT();
double estimateTemperature( double cie_a, double cie_b );

void  printfHelp (int argc, char** argv)
{
  int i;
  fprintf( stderr, "\n");
  for(i = 0; i < argc; ++i)
    fprintf( stderr, "%s ", argv[i]);
  fprintf( stderr, "\n");
  fprintf( stderr, "%s v%d.%d.%d %s\n", argv[0],
                        OYRANOS_VERSION_A,OYRANOS_VERSION_B,OYRANOS_VERSION_C,
                                _("is a monitor white point handler"));
  fprintf( stderr, "%s:\n",               _("Usage"));
  fprintf( stderr, "  %s\n",              _("Autostart daemon:"));
  fprintf( stderr, "      %s --daemon|-d=1 [-v]\n", argv[0] );
  fprintf( stderr, "\n");
  fprintf( stderr, "  %s\n",              _("Run sunset daemon:"));
  fprintf( stderr, "      %s -d %s [-v]\n", argv[0], _("MODE") );
  fprintf( stderr,  "      -d 0 - %s\n",   _("Deactivate"));
  fprintf( stderr,  "      -d 1 - %s\n",   _("Autostart"));
  fprintf( stderr,  "      -d 2 - %s\n",   _("Activate"));
  fprintf( stderr, "\n");
  fprintf( stderr, "  %s\n",              _("Show modes:"));
  fprintf( stderr, "      %s -m [-v]\n", argv[0] );
  fprintf( stderr, "\n");
  fprintf( stderr,  "  %s\n",             _("Set actual mode:"));
  fprintf( stderr,  "     %s -w 0|1|2|3|4|5|6|7|8 [--system-wide] [-v]\n", argv[0]);
  fprintf( stderr,  "      -w 0 - %s\n",   _("no white point adjustments (old behaviour)"));
  fprintf( stderr,  "      -w 1 - %s\n",   _("automatic"));
  fprintf( stderr,  "      -w 2 - %s\n",   _("D50"));
  fprintf( stderr,  "      -w 3 - %s\n",   _("D55"));
  fprintf( stderr,  "      -w 4 - %s\n",   _("D65"));
  fprintf( stderr,  "      -w 5 - %s\n",   _("D75"));
  fprintf( stderr,  "      -w 6 - %s\n",   _("D93"));
  fprintf( stderr,  "      -w 7 - %s\n",   _("first monitors actual media white point as target for all other monitors"));
  fprintf( stderr,  "      -w 8 - %s\n",   _("second monitors actual media white point as target for all other monitors"));
  fprintf( stderr, "\n");
  fprintf( stderr,  "  %s\n",             _("Set preferred nightly white point:"));
  fprintf( stderr,  "     %s -n 1|2|3 [--system-wide] [-v]\n", argv[0]);
  fprintf( stderr,  "      -n 1 - %s\n",   _("automatic"));
  fprintf( stderr,  "      -n 2 - %s\n",   _("D50"));
  fprintf( stderr,  "      -n 3 - %s\n",   _("D55"));
  fprintf( stderr, "\n");
  fprintf( stderr,  "  %s\n",             _("Set preferred sun white point:"));
  fprintf( stderr,  "     %s -s 0|1|2|3|4|5|6|7|8 [--system-wide] [-v]\n", argv[0]);
  fprintf( stderr,  "      -s x - %s\n",   _("see the -w option"));
  fprintf( stderr, "\n");
  fprintf( stderr,  "  %s\n",             _("Set automatic white point:"));
  fprintf( stderr,  "     %s -a KELVIN [--system-wide] [-v]\n", argv[0]);
  fprintf( stderr,  "      -a KELVIN - %s\n",   _("A value from 2700 till 8000 Kelvin is expected to show no artefacts"));
  fprintf( stderr, "\n");
  fprintf( stderr,  "  %s\n",             _("Set preferred nightly effect:"));
  fprintf( stderr,  "     %s --night-effect ICC_PROFILE [--system-wide] [-v]\n", argv[0]);
  fprintf( stderr,  "      --night-effect ICC_PROFILE - %s\n", _("a ICC profile of class abstract. Ideally the effect profile works on 1D RGB curves only and is marked meta:EFFECT_linear=yes ."));
  fprintf( stderr, "\n");
  fprintf( stderr,  "  %s\n",             _("Set preferred daylight effect:"));
  fprintf( stderr,  "     %s --sunlight-effect ICC_PROFILE [--system-wide] [-v]\n", argv[0]);
  fprintf( stderr,  "      --sunlight-effect ICC_PROFILE - %s\n", _("a ICC profile of class abstract. Ideally the effect profile works on 1D RGB curves only and is marked meta:EFFECT_linear=yes ."));
  fprintf( stderr, "\n");
  fprintf( stderr, "  %s\n",              _("Get Location:"));
  fprintf( stderr, "      %s -l [-v]\n", argv[0] );
  fprintf( stderr, "\n");
  fprintf( stderr, "  %s\n",              _("Set Longitude:"));
  fprintf( stderr, "      %s --longitude %s [-v]\n", argv[0], _("ANGLE_IN_DEGREE") );
  fprintf( stderr, "\n");
  fprintf( stderr, "  %s\n",              _("Set Latitude:"));
  fprintf( stderr, "      %s --latitude %s [-v]\n", argv[0], _("ANGLE_IN_DEGREE") );
  fprintf( stderr, "\n");
  fprintf( stderr, "  %s\n",              _("Show sun rise:"));
  fprintf( stderr, "      %s -r [-v]\n", argv[0] );
  fprintf( stderr, "\n");
  fprintf( stderr, "  %s\n",              _("Set twilight angle:"));
  fprintf( stderr, "      %s -t %s [-v]\n", argv[0], _("ANGLE_IN_DEGREE") );
  fprintf( stderr,  "      -t 0 - %s\n",   _("rise/set"));
  fprintf( stderr,  "      -t -6 - %s\n",   _("civil"));
  fprintf( stderr,  "      -t -12 - %s\n",  _("nautical"));
  fprintf( stderr,  "      -t -18 - %s\n",  _("astronomical"));
  fprintf( stderr, "\n");
  fprintf( stderr, "  %s\n",              _("General options:"));
  fprintf( stderr, "      -v \t%s\n",     _("verbose"));
  fprintf( stderr, "\n");
  fprintf( stderr, _("For more informations read the man page:"));
  fprintf( stderr, "\n");
  fprintf( stderr, "      man oyranos-monitor-white-point\n");
}



oySCOPE_e scope = oySCOPE_USER;
double hour_ = -1.0; /* ignore this default value */

int main( int argc , char** argv )
{
  unsigned i;
  int error = 0;
  /* the functional switches */
  int wtpt_mode = -1;
  int wtpt_mode_night = -1;
  int wtpt_mode_sunlight = -1;
  char * sunlight_effect = NULL;
  char * night_effect = NULL;
  double temperature = 0.0;
  int show = 0;
  int dry = 0;
  int location = 0;
  double longitude = 360;
  double latitude = 360;
  int sunrise = 0;
  double twilight = -1000;
  char * value = NULL;
  double rise = 0.0,
         set = 0.0;
  int daemon = -1;
  int check = 0;

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


  if(argc != 1)
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
              case 'a': i=0; OY_PARSE_FLOAT_ARG2( temperature, "a", 1100, 10100, 5000 ); break;
              case 'd': OY_PARSE_INT_ARG( daemon ); break;
              case 'r': sunrise = 1; break;
              case 'l': location = 1; break;
              case 'n': OY_PARSE_INT_ARG( wtpt_mode_night ); break;
              case 'j': show = 2; break;
              case 'm': show = 1; break;
              case 's': OY_PARSE_INT_ARG( wtpt_mode_sunlight ); break;
              case 't': i=0; OY_PARSE_FLOAT_ARG2( twilight, "t", -90.0, 90.0, 0.0 ); break;
              case 'w': OY_PARSE_INT_ARG( wtpt_mode ); break;
              case 'v': oy_debug += 1; break;
              case 'h':
              case '-':
                        if(i == 1)
                        {
                             if(OY_IS_ARG("verbose"))
                        { oy_debug += 1; i=100; break;}
                        else if(OY_IS_ARG("system-wide"))
                        { scope = oySCOPE_SYSTEM; i=100; break; }
                        else if(OY_IS_ARG("check"))
                        { check = 1; i=100; break; }
                        else if(OY_IS_ARG("daemon"))
                        { daemon = 1; i=100; break; }
                        else if(OY_IS_ARG("latitude"))
                        { OY_PARSE_FLOAT_ARG2( latitude,  "latitude",  - 90.0,  90.0, 0.0 ); i=100; break; }
                        else if(OY_IS_ARG("longitude"))
                        { OY_PARSE_FLOAT_ARG2( longitude, "longitude", -180.0, 180.0, 0.0 ); i=100; break; }
                        else if(OY_IS_ARG("sunlight-effect"))
                        { OY_PARSE_STRING_ARG2(sunlight_effect, "sunlight-effect"); break; }
                        else if(OY_IS_ARG("night-effect"))
                        { OY_PARSE_STRING_ARG2(night_effect, "night-effect"); break; }
                        else if(OY_IS_ARG("hour"))
                        { OY_PARSE_FLOAT_ARG2( hour_, "hour", 0.0, 24.0, 0.0 ); i=100; break; }
                        else if(OY_IS_ARG("dry-run"))
                        { dry = 1; i=100; break; }
                        } OY_FALLTHROUGH
              default:
                        printfHelp(argc, argv);
                        exit (0);
                        break;
            }
            break;
      }
      if( wrong_arg )
      {
        fprintf( stderr, "%s %s\n", _("wrong argument to option:"), wrong_arg);
        printfHelp(argc, argv);
        exit(1);
      }
      ++pos;
    }
    if(oy_debug) fprintf( stderr,  "%s\n", argv[1] );
  }
  else
  {
                        printfHelp(argc, argv);
                        exit (0);
  }

  if(oy_debug)
    fprintf( stderr, "  Oyranos v%s\n",
                  oyNoEmptyName_m_(oyVersionString(1,0)));


  if(temperature != 0.0)
  {
    int i = (temperature - 1000) / 100;
    double xyz[3] = { bb_100K[i][0], bb_100K[i][1], bb_100K[i][2] };
    double XYZ[3] = { xyz[0]/xyz[1], 1.0, xyz[2]/xyz[1] }, oldXYZ[3];
    double Lab[3], oldLab[3];
    double cie_a = 0.0, cie_b = 0.0;
    char * comment = NULL;
    double old_temperature = 0;
    oyXYZ2Lab( XYZ, Lab);

    oyGetDisplayWhitePoint( 1 /* automatic */, oldXYZ );
    oyXYZ2Lab( oldXYZ, oldLab);
    cie_a = oldLab[1]/256.0 + 0.5;
    cie_b = oldLab[2]/256.0 + 0.5;

    old_temperature = estimateTemperature( cie_a, cie_b );
    if(old_temperature)
      oyStringAddPrintf( &comment, 0,0, "Old Temperature:: %g ", old_temperature );
    oyStringAddPrintf( &comment, 0,0, "Old cie_a: %f cie_b: %f ", cie_a, cie_b );
    cie_a = Lab[1]/256.0 + 0.5;
    cie_b = Lab[2]/256.0 + 0.5;
    oyStringAddPrintf( &comment, 0,0, "New Temperature: %.00f cie_a: %f cie_b: %f", temperature, cie_a, cie_b );
    fprintf (stderr, "%s %s\n", _("Automatic white point"), comment );
    if(dry == 0)
    {
      oySetDisplayWhitePoint( XYZ, scope, comment );
      pingNativeDisplay();
    }
    oyFree_m_( comment );
  }

  if(wtpt_mode >= 0)
  {
    error = setWtptMode( scope, wtpt_mode, dry );
    return error;
  }


  if(show == 1)
  {
    uint32_t flags = 0;
    int choices = 0, current = -1;
    const char ** choices_string_list = NULL;
    error = oyOptionChoicesGet2( oyWIDGET_DISPLAY_WHITE_POINT, flags,
                                 oyNAME_NAME, &choices,
                                 &choices_string_list, &current );
    if(!error)
    {
      fprintf(stderr, "%s\n", _("Choices:"));
      for(i = 0; (int)i < choices; ++i)
      {
        const char * t = "";
        char * tmp = NULL;
        if(i == 1) /* automatic */
        {
          double cie_a = 0.0, cie_b = 0.0, XYZ[3], Lab[3];
          oyGetDisplayWhitePoint( 1 /* automatic */, XYZ );
          oyXYZ2Lab(XYZ,Lab); cie_a = Lab[1]/256.0+0.5; cie_b = Lab[2]/256.0+0.5;
          double temperature = estimateTemperature( cie_a, cie_b );
          if(temperature)
          {
            oyStringAddPrintf( &tmp, 0,0, "%g %s ", temperature, _("Kelvin") );
            t = tmp;
          }
        }
        printf("-w %u # %s %s%s\n", i, choices_string_list[i], t, (int)i == current ? "*":" ");
        if(tmp) oyFree_m_(tmp);
      }
    }
  }


  if(location)
    error = findLocation(scope, dry);

  if(twilight != -1000)
  {
    oyStringAddPrintfC(&value, 0,0, "%g", twilight);
    if(dry == 0)
      oySetPersistentString( OY_DISPLAY_STD OY_SLASH "/twilight", scope, value, NULL );
    oyFree_m_(value);
  }

  if(longitude != 360)
  {
    oyStringAddPrintfC(&value, 0,0, "%g", longitude);
    if(dry == 0)
      oySetPersistentString( OY_DISPLAY_STD "/longitude", scope, value, NULL );
    oyFree_m_(value);
  }

  if(latitude != 360)
  {
    oyStringAddPrintfC(&value, 0,0, "%g", latitude);
    if(dry == 0)
      oySetPersistentString( OY_DISPLAY_STD "/latitude", scope, value, NULL );
    oyFree_m_(value);
  }

  if(wtpt_mode_night != -1)
  {
    oyStringAddPrintf(&value, 0,0, "%d", wtpt_mode_night);
    if(dry == 0)
      oySetPersistentString( OY_DISPLAY_STD "/display_white_point_mode_night", scope, value, NULL );
    oyFree_m_(value);
  }

  if(wtpt_mode_sunlight != -1)
  {
    oyStringAddPrintf(&value, 0,0, "%d", wtpt_mode_sunlight);
    if(dry == 0)
      oySetPersistentString( OY_DISPLAY_STD "/display_white_point_mode_sunlight", scope, value, NULL );
    oyFree_m_(value);
  }

  if(night_effect != NULL && dry == 0)
    oySetPersistentString( OY_DISPLAY_STD "/night_effect", scope, night_effect[0]?night_effect:NULL, NULL );

  if(sunlight_effect != NULL && dry == 0)
    oySetPersistentString( OY_DISPLAY_STD "/sunlight_effect", scope, sunlight_effect[0]?sunlight_effect:NULL, NULL );

  if(sunrise)
  {
    error = getSunriseSunset( &rise, &set, dry );

    printf( "%g %g\n", rise, set);
  }

  if(daemon != -1)
    error = runDaemon(daemon);

  if(show == 2)
  {
  }

  if(check)
    checkWtptState( dry );

  oyFinish_( FINISH_IGNORE_I18N | FINISH_IGNORE_CACHES );

  return error;
}

void pingNativeDisplay()
{
  /* ping X11 observers about option change
   * ... by setting a known property again to its old value
   */
  oyOptions_s * opts = oyOptions_New(NULL), * results = 0;
  oyOptions_Handle( "//" OY_TYPE_STD "/send_native_update_event",
                      opts,"send_native_update_event",
                      &results );
  oyOptions_Release( &opts );
}

int setWtptMode( oySCOPE_e scope, int wtpt_mode, int dry )
{
  int choices = 0;
  const char ** choices_string_list = NULL;

  int error = 0;
  
  if(dry == 0)
  {
    oySetBehaviour( oyBEHAVIOUR_DISPLAY_WHITE_POINT, scope, wtpt_mode );
    updateVCGT();
  }
  /* ping X11 observers about option change
   * ... by setting a known property again to its old value
   */
  if(!error)
    pingNativeDisplay();
  else
    fprintf(stderr, "error %d in oySetBehaviour(oyBEHAVIOUR_DISPLAY_WHITE_POINT,%d,%d)\n", error, scope, wtpt_mode);

  {
    int current = -1;
    uint32_t flags = 0;
    error = oyOptionChoicesGet2( oyWIDGET_DISPLAY_WHITE_POINT, flags,
                                 oyNAME_NAME, &choices,
                                 &choices_string_list, &current );
    if(current == wtpt_mode)
      fprintf (stderr, "%s => %s\n", _("New white point mode"),
               (wtpt_mode<choices && choices_string_list) ? choices_string_list[wtpt_mode] : "----");
    else
      fprintf (stderr, "%s %s => %d != %d\n", _("!!! ERROR"), _("New white point mode"),
               wtpt_mode, current);
    oyOptionChoicesFree( oyWIDGET_DISPLAY_WHITE_POINT, &choices_string_list, choices );
  }

  return error;
}

void updateVCGT()
{
  size_t size = 0;
  char * result = oyReadCmdToMem_( "oyranos-monitor -l | wc -l", &size, "r", malloc );
  char * tmpname = oyGetTempFileName_( NULL, "vcgt.icc", 0, oyAllocateFunc_ );
  if(!result) return;
  int count = atoi(result), i;
  char * cmd = NULL;
  fprintf(stderr, "monitor count: %d\n", count);
  free(result);

  for(i = 0; i < count; ++i)
  {
    int r OY_UNUSED;
    oyStringAddPrintf(&cmd, 0,0, "oyranos-monitor -c -f vcgt -d %d -o %s", i, tmpname);
    fputs(cmd, stderr); fputs("\n", stderr );
    r = system( cmd );
    oyFree_m_(cmd);
    oyStringAddPrintf(&cmd, 0,0, "oyranos-monitor -g -d %d %s", i, tmpname);
    fputs(cmd, stderr); fputs("\n", stderr );
    r = system( cmd );
    oyFree_m_(cmd);
    //remove( tmpname );
  }
}

double estimateTemperature( double cie_a_, double cie_b_ )
{
  double temperature = 0;

  if(cie_a_ != 0.0 && cie_b_ != 0.0)
  {
    int i;
    for(i = 1; i <= bb_100K[0][2]; ++i)
    {
      double xyz[3] = { bb_100K[i][0], bb_100K[i][1], bb_100K[i][2] };
      double XYZ[3] = { xyz[0]/xyz[1], 1.0, xyz[2]/xyz[1] };
      double Lab[3];
      double cie_a = 0.0, cie_b = 0.0;
      oyXYZ2Lab( XYZ, Lab);

      cie_a = Lab[1]/256.0 + 0.5;
      cie_b = Lab[2]/256.0 + 0.5;
      if(fabs(cie_a - cie_a_) < 0.0001 &&
         fabs(cie_b - cie_b_) < 0.0001)
      {
        temperature = bb_100K[0][0] + i * bb_100K[0][1];
        break;
      }
    }
  }

  return temperature;
}

int findLocation(oySCOPE_e scope, int dry)
{
  int error = 0;

  {
    size_t size = 0;
    char * geo_json = oyReadUrlToMemf_( &size, "r", oyAllocateFunc_,
                                        "http://freegeoip.net/json/", NULL ),
	 * value = NULL;
    oyjl_val root = 0;
    oyjl_val v = 0;
    double lon = 0,
	   lat = 0;

    if(geo_json)
    {
      char * t = oyAllocateFunc_(256);
      root = oyjlTreeParse( geo_json, t, 256 );
      if(t[0])
        WARNc2_S( "%s: %s\n", _("found issues parsing JSON"), t );
      oyFree_m_(t);
      if(oy_debug)
        fprintf(stderr, "%s\n", geo_json);
    } else
      error = 1;

    if(root)
    {
      char * json = NULL;
      int level = 0;
      v = oyjlTreeGetValuef( root, 0, "latitude", NULL );
      value = oyjlValueText( v, oyAllocateFunc_ );
      if(oyStringToDouble( value, &lat ))
        error = 1;
      v = oyjlTreeGetValuef( root, 0, "longitude", NULL );
      value = oyjlValueText( v, oyAllocateFunc_ );
      if(oyStringToDouble( value, &lon ))
        error = 1;
      oyFree_m_(value);
      oyjlTreeToJson( root, &level, &json );

      if(lat != 0.0 && lon != 0.0)
      {
        #define PRINT_VAL( key, name )\
        v = oyjlTreeGetValuef( root, 0, key, NULL ); \
        value = oyjlValueText( v, oyAllocateFunc_ ); \
	if(value) \
        { if(value[0]) \
            fprintf( stderr, "%s: %s\n", name, value ); \
          oyFree_m_(value); \
        }
        PRINT_VAL( "time_zone", _("Time Zone") )
        PRINT_VAL( "country_name", _("Country") )
        PRINT_VAL( "city", _("City") )
        #undef PRINT_VAL

#ifdef HAVE_LOCALE_H
        char * save_locale = oyjlStringCopy( setlocale(LC_NUMERIC, 0 ), malloc );
        setlocale(LC_NUMERIC, "C");
#endif
        printf( "%g %g\n", lat,lon);

        oyStringAddPrintfC(&value, 0,0, "%g", lat);
        if(dry == 0)
          oySetPersistentString( OY_DISPLAY_STD "/latitude", scope, value, NULL );
        oyFree_m_(value);
        oyStringAddPrintfC(&value, 0,0, "%g", lon);
        if(dry == 0)
          oySetPersistentString( OY_DISPLAY_STD "/longitude", scope, value, NULL );
        oyFree_m_(value);

#ifdef HAVE_LOCALE_H
        setlocale(LC_NUMERIC, save_locale);
        if(save_locale)
          free( save_locale );
#endif
      } else
        error = 1;

      oyjlTreeFree(root);
    } else
      error = 1;
  }

  return error;
}

int getLocation( double * lon, double * lat)
{
  int need_location = 0;
  char * value = NULL;
 
  value = oyGetPersistentString( OY_DISPLAY_STD "/latitude", 0, oySCOPE_USER_SYS, oyAllocateFunc_ );
  if(value && oyStringToDouble( value, lat ))
    fprintf(stderr, "lat = %g / %s\n", *lat, value);
  if(value)
  {
    oyFree_m_(value);
  } else
    need_location = 1;
  value = oyGetPersistentString( OY_DISPLAY_STD "/longitude", 0, oySCOPE_USER_SYS, oyAllocateFunc_ );
  if(value && oyStringToDouble( value, lon ))
    fprintf(stderr, "lon = %g / %s\n", *lon, value);
  if(value)
  {
    oyFree_m_(value);
  } else
    need_location = 1;

  return need_location;
}

#define oyGetCurrentGMTHour_(arg) ((hour_ != -1.0) ? hour_ + oyGetCurrentGMTHour(arg)*0.0 : oyGetCurrentGMTHour(arg))
double oyNormaliseHour(double hour)
{
  if(hour > 24.0)
    return hour - 24.0;
  if(hour < 0.0)
    return hour + 24.0;
  else
    return hour;
}

int getSunriseSunset( double * rise, double * set, int dry )
{
  double lat = 0.0,
         lon = 0.0;
  double twilight = 0;
  int year,month,day;
  int r;
  char * value = NULL;
 
  if(getLocation(&lon, &lat))
  {
    findLocation( scope, dry );
    getLocation(&lon, &lat);
  }

  {
    time_t  cutime;         /* Time since epoch */
    struct tm       *gmt;
    char time_str[24];

    cutime = time(NULL); /* time right NOW */
    gmt = localtime(&cutime);
    strftime(time_str, 24, "%Y", gmt);
    year  = strtol(time_str,NULL,10);
    strftime(time_str, 24, "%m", gmt);
    month = strtol(time_str,NULL,10);
    strftime(time_str, 24, "%d", gmt);
    day   = strtol(time_str,NULL,10);
  }

  value =
      oyGetPersistentString(OY_DISPLAY_STD "/twilight", 0, oySCOPE_USER_SYS, oyAllocateFunc_);
  if(value && oyStringToDouble( value, &twilight ))
    fprintf(stderr, "twilight = %g / %s\n", twilight, value);
  if(value)
  { oyFree_m_(value);
  }

  r = __sunriset__( year,month,day, lon,lat,
                    (twilight==0)?-35.0/60.0:twilight, 0, rise, set );
  if(r > 0)
    fprintf(stderr, "sun will not get below twilight today\n");
  if(r < 0)
    fprintf(stderr, "sun will not get above twilight today\n");
  {
    int hour, minute, second, gmt_diff_second;
    double elevation;

    oyGetCurrentGMTHour_( &gmt_diff_second );
    oySplitHour( oyGetCurrentLocalHour( oyGetCurrentGMTHour_(0), gmt_diff_second ), &hour, &minute, &second );
    elevation = getSunHeight( year, month, day, oyGetCurrentGMTHour_(0), lat, lon );
    fprintf( stderr, "%d-%d-%d %d:%.2d:%.2d",
             year, month, day, hour, minute, second );
    oySplitHour( oyGetCurrentLocalHour( *rise, gmt_diff_second ), &hour, &minute, &second );
    fprintf( stderr, " %s: %g° %g° %s: %g° (%s: %g°) %s: %d:%.2d:%.2d",
             _("Geographical Position"), lat, lon, _("Twilight"), twilight, _("Sun Elevation"), elevation, _("Sunrise"), hour, minute, second );
    oySplitHour( oyGetCurrentLocalHour( *set,  gmt_diff_second ), &hour, &minute, &second );
    fprintf( stderr, " %s: %d:%.2d:%.2d\n",
             _("Sunset"), hour, minute, second );
  }

  return r;
}


/* check the sunrise / sunset state */
int checkWtptState(int dry)
{
  int error = 0;
  int cmode;
  char * effect = NULL;

  int    diff;
  double dtime, rise, set;

  int choices = 0;
  const char ** choices_string_list = NULL;
  char * value;
  int active = 1;

  /* settings changed on disk, need to reread */
  oyGetPersistentStrings( NULL );

  value = oyGetPersistentString(OY_DEFAULT_DISPLAY_WHITE_POINT_DAEMON, 0,
                                oySCOPE_USER_SYS, oyAllocateFunc_);
  if(!value || strcmp(value,"oyranos-monitor-white-point") != 0)
    active = 0;
  if(value)
  { oyFree_m_(value);
  }
  if(!active)
    return -1;


  dtime = oyGetCurrentGMTHour_(&diff);

  {
    int current = -1;
    uint32_t flags = 0;
    error = oyOptionChoicesGet2( oyWIDGET_DISPLAY_WHITE_POINT, flags,
                                 oyNAME_NAME, &choices,
                                 &choices_string_list, &current );
  }

  if( choices_string_list && getSunriseSunset( &rise, &set, dry ) == 0 )
  {
    int new_mode = -1;
    char * new_effect = NULL;
    int use_effect = 0;

    cmode = oyGetBehaviour( oyBEHAVIOUR_DISPLAY_WHITE_POINT );
    effect = oyGetPersistentString( OY_DEFAULT_EFFECT_PROFILE, 0, oySCOPE_USER_SYS, oyAllocateFunc_ );
    fprintf (stderr, "%s: %s %s: %s\n", _("Actual white point mode"), cmode<choices?choices_string_list[cmode]:"----",
            _("Effect"), oyNoEmptyString_m_(effect));

    if(rise < dtime && dtime <= set)
    /* day time */
    {
      char * value = oyGetPersistentString( OY_DISPLAY_STD "/display_white_point_mode_sunlight", 0, oySCOPE_USER_SYS, oyAllocateFunc_ );
      if(value)
      {
        new_mode = atoi(value);
        oyFree_m_(value);
      } else /* defaut to D65 for daylight */
        new_mode = 4;

      new_effect = oyGetPersistentString( OY_DISPLAY_STD "/sunlight_effect", 0, oySCOPE_USER_SYS, oyAllocateFunc_ );
      if(new_effect)
        ++use_effect;
      else
      {
        value = oyGetPersistentString( OY_DISPLAY_STD "/night_effect", 0, oySCOPE_USER_SYS, oyAllocateFunc_ );
        if(value)
        {
          ++use_effect;
          oyFree_m_(value);
        }
      }
      if(dry == 0)
        if(!oyExistPersistentString( OY_DISPLAY_STD "/night", "0", 0, oySCOPE_USER_SYS ))
          oySetPersistentString( OY_DISPLAY_STD "/night", scope, "0", NULL );

    } else
    /* night time */
    {
      char * value = oyGetPersistentString( OY_DISPLAY_STD "/display_white_point_mode_night", 0, oySCOPE_USER_SYS, oyAllocateFunc_ );
      if(value)
      {
        new_mode = atoi(value);
        oyFree_m_(value);
      } else /* defaut to D50 for night light */
        new_mode = 2;

      new_effect = oyGetPersistentString( OY_DISPLAY_STD "/night_effect", 0, oySCOPE_USER_SYS, oyAllocateFunc_ );
      if(new_effect)
        ++use_effect;
      else
      {
        value = oyGetPersistentString( OY_DISPLAY_STD "/sunlight_effect", 0, oySCOPE_USER_SYS, oyAllocateFunc_ );
        if(value)
        {
          ++use_effect;
          oyFree_m_(value);
        }
      }
      if(dry == 0)
        if(!oyExistPersistentString( OY_DISPLAY_STD "/night", "1", 0, oySCOPE_USER_SYS ))
          oySetPersistentString( OY_DISPLAY_STD "/night", scope, "1", NULL );
    }

    if( (new_mode != cmode) ||
        ((effect?1:0) != (new_effect?1:0) ||
         (effect && new_effect && strcmp(effect, new_effect) != 0)))
    {
      fprintf(  stderr, "%s: %s %s: %s\n", _("New white point mode"), new_mode<choices?choices_string_list[new_mode]:"----",
                _("Effect"), oyNoEmptyString_m_(new_effect) );

      if(dry == 0 && use_effect)
        oySetPersistentString( OY_DEFAULT_EFFECT_PROFILE, scope, new_effect, NULL );

      if(cmode != new_mode)
        error = setWtptMode( scope, new_mode, dry );
    }

    if(effect) oyFree_m_(effect);
    if(new_effect) oyFree_m_(new_effect);
  }

  oyOptionChoicesFree( oyWIDGET_DISPLAY_WHITE_POINT, &choices_string_list, choices );

  return error;
}

#ifdef HAVE_DBUS
#include "oyranos_dbus_macros.h"
oyDBusFilter_m
oyWatchDBus_m( oyDBusFilter )
oyFinishDBus_m
int oy_dbus_config_changed = 0;
static void oyMonitorCallbackDBus    ( double              progress_zero_till_one OY_UNUSED,
                                       char              * status_text,
                                       int                 thread_id_ OY_UNUSED,
                                       int                 job_id OY_UNUSED,
                                       oyStruct_s        * cb_progress_context )
{
  const char * key;
  int verbose = oyOption_GetValueInt( (oyOption_s*)cb_progress_context, 0 );
  if(!status_text) return;

  oyGetPersistentStrings(NULL);

  key = strchr( status_text, '/' );
  if(key)
    ++key;
  else
    return;
  if(!verbose) return;

  if( strstr(key, OY_STD "/ping") == NULL && /* let us ping */
      strstr(key, OY_DISPLAY_STD) == NULL && /* all display variables */
      strstr(key, OY_DEFAULT_DISPLAY_WHITE_POINT) == NULL && /* oySetDisplayWhitePoint() */
      strstr(key, OY_DEFAULT_EFFECT) == NULL && /* effect switch changes */
      strstr(key, OY_DEFAULT_EFFECT_PROFILE) == NULL /* new effect profile */
    )
    return;
  if( /* skip XY(Z) and listen only on Z to reduce flicker */
      strstr(key, OY_DEFAULT_DISPLAY_WHITE_POINT_X) != NULL ||
      strstr(key, OY_DEFAULT_DISPLAY_WHITE_POINT_Y) != NULL || 
      /* skip XY(Z) with elektra style array indixes */
      strstr(key, OY_DISPLAY_STD OY_SLASH "display_white_point_XYZ/#0") != NULL ||
      strstr(key, OY_DISPLAY_STD OY_SLASH "display_white_point_XYZ/#1") != NULL
    )
    return;

  char * v = oyGetPersistentString( key, 0, scope, oyAllocateFunc_ );
  fprintf(stderr, "%s:%s\n", key,v);
  oyFree_m_(v);

  checkWtptState( 0 );
  updateVCGT();

  /* Clear the changed state, before a new check. */
  oy_dbus_config_changed = 1;
}
#endif /* HAVE_DBUS */


int runDaemon(int dmode)
{
  int error = 0, id, active = 1;
  double hour_old = 0.0;

  if(dmode == 0) /* stop service */
  {
    /* erase the key */
    oySetPersistentString( OY_DEFAULT_DISPLAY_WHITE_POINT_DAEMON, scope, NULL, NULL );
    active = 0; 
    return error;
  }
  else
  if(dmode == 1) /* check if service is desired */
  {
    char * value = 
      oyGetPersistentString(OY_DEFAULT_DISPLAY_WHITE_POINT_DAEMON, 0,
		            oySCOPE_USER_SYS, oyAllocateFunc_);
    if(!value || strcmp(value,"oyranos-monitor-white-point") != 0)
      active = 0;

    oyFree_m_(value);
  }
  else /* dmode >= 2 => start service */
    oySetPersistentString( OY_DEFAULT_DISPLAY_WHITE_POINT_DAEMON, scope, "oyranos-monitor-white-point", "CLI Daemon" );

  /* ensure all keys are setup properly
   * before we lock the DBus connection by listening */
  if(active)
    checkWtptState( 0 );

#ifdef HAVE_DBUS
  oyStartDBusObserver( oyWatchDBus, oyFinishDBus, oyMonitorCallbackDBus, OY_DISPLAY_STD, NULL )
  if(id)
    fprintf(stderr, "oyStartDBusObserver ID: %d\n", id);

  while(1)
  {
    double hour = oyGetCurrentGMTHour_( 0 );
    double repeat_check = 1.0/60.0; /* every minute */

    oyLoopDBusObserver( hour, repeat_check, oy_dbus_config_changed, checkWtptState(0) )

    /* delay next polling */
    oySleep( 0.25 );
  }

  return error;
#endif /* HAVE_DBUS */
}


/*  ---------------- 8< ------------------ */
/*#include "sunriset.h"*/
extern const char* timezone_name;
extern long int timezone_offset;

#define TMOD(x) ((x)<0?(x)+24:((x)>=24?(x)-24:(x)))
#define DAYSOFF(x) ((x)<0?"(-1) ":((x)>=24?"(+1) ":""))

#define HOURS(h) ((int)(floor(h)))
#define MINUTES(h) ((int)(60*(h-floor(h))))

#define ABSSS(x) ((x)<0?-(x):(x))

/* A macro to compute the number of days elapsed since 2000 Jan 0.0 */
/* (which is equal to 1999 Dec 31, 0h UT)                           */
/* Dan R sez: This is some pretty fucking high magic. */
#define days_since_2000_Jan_0(y,m,d) \
    (367L*(y)-((7*((y)+(((m)+9)/12)))/4)+((275*(m))/9)+(d)-730530L)

/* Some conversion factors between radians and degrees */

#ifndef PI
 #define PI        3.1415926535897932384
#endif

#define RADEG     ( 180.0 / PI )
#define DEGRAD    ( PI / 180.0 )

/* The trigonometric functions in degrees */

#define sind(x)  sin((x)*DEGRAD)
#define cosd(x)  cos((x)*DEGRAD)
#define tand(x)  tan((x)*DEGRAD)

#define atand(x)    (RADEG*atan(x))
#define asind(x)    (RADEG*asin(x))
#define acosd(x)    (RADEG*acos(x))
#define atan2d(y,x) (RADEG*atan2(y,x))

/* Following are some macros around the "workhorse" function __daylen__ */
/* They mainly fill in the desired values for the reference altitude    */
/* below the horizon, and also selects whether this altitude should     */
/* refer to the Sun's center or its upper limb.                         */


/* This macro computes the length of the day, from sunrise to sunset. */
/* Sunrise/set is considered to occur when the Sun's upper limb is    */
/* 50 arc minutes below the horizon (this accounts for the refraction */
/* of the Earth's atmosphere).                                        */
/* The original version of the program used the value of 35 arc mins, */
/* which is the accepted value in Sweden.                             */
#define day_length(year,month,day,lon,lat)  \
        __daylen__( year, month, day, lon, lat, -50.0/60.0, 1 )

/* This macro computes the length of the day, including civil twilight. */
/* Civil twilight starts/ends when the Sun's center is 6 degrees below  */
/* the horizon.                                                         */
#define day_civil_twilight_length(year,month,day,lon,lat)  \
        __daylen__( year, month, day, lon, lat, -6.0, 0 )

/* This macro computes the length of the day, incl. nautical twilight.  */
/* Nautical twilight starts/ends when the Sun's center is 12 degrees    */
/* below the horizon.                                                   */
#define day_nautical_twilight_length(year,month,day,lon,lat)  \
        __daylen__( year, month, day, lon, lat, -12.0, 0 )

/* This macro computes the length of the day, incl. astronomical twilight. */
/* Astronomical twilight starts/ends when the Sun's center is 18 degrees   */
/* below the horizon.                                                      */
#define day_astronomical_twilight_length(year,month,day,lon,lat)  \
        __daylen__( year, month, day, lon, lat, -18.0, 0 )


/* This macro computes times for sunrise/sunset.                      */
/* Sunrise/set is considered to occur when the Sun's upper limb is    */
/* 35 arc minutes below the horizon (this accounts for the refraction */
/* of the Earth's atmosphere).                                        */
#define sun_rise_set(year,month,day,lon,lat,rise,set)  \
        __sunriset__( year, month, day, lon, lat, -35.0/60.0, 1, rise, set )

/* This macro computes the start and end times of civil twilight.       */
/* Civil twilight starts/ends when the Sun's center is 6 degrees below  */
/* the horizon.                                                         */
#define civil_twilight(year,month,day,lon,lat,start,end)  \
        __sunriset__( year, month, day, lon, lat, -6.0, 0, start, end )

/* This macro computes the start and end times of nautical twilight.    */
/* Nautical twilight starts/ends when the Sun's center is 12 degrees    */
/* below the horizon.                                                   */
#define nautical_twilight(year,month,day,lon,lat,start,end)  \
        __sunriset__( year, month, day, lon, lat, -12.0, 0, start, end )

/* This macro computes the start and end times of astronomical twilight.   */
/* Astronomical twilight starts/ends when the Sun's center is 18 degrees   */
/* below the horizon.                                                      */
#define astronomical_twilight(year,month,day,lon,lat,start,end)  \
        __sunriset__( year, month, day, lon, lat, -18.0, 0, start, end )


/* Function prototypes */

double __daylen__( int year, int month, int day, double lon, double lat,
                   double altit, int upper_limb );

int __sunriset__( int year, int month, int day, double lon, double lat,
                  double altit, int upper_limb, double *rise, double *set );

void sunpos( double d, double *lon, double *r );

void sun_RA_dec( double d, double *RA, double *dec, double *r );

double revolution( double x );

double rev180( double x );

double GMST0( double d );

/*
SUNRISET.C - computes Sun rise/set times, start/end of twilight, and
             the length of the day at any date and latitude
Written as DAYLEN.C, 1989-08-16
Modified to SUNRISET.C, 1992-12-01
(c) Paul Schlyter, 1989, 1992
Released to the public domain by Paul Schlyter, December 1992
*/


#include <stdio.h>
#include <math.h>
#include <time.h>

/*#include "sunriset.h"*/

/* The "workhorse" function for sun rise/set times */

int __sunriset__( int year, int month, int day, double lon, double lat,
                  double altit, int upper_limb, double *trise, double *tset )
/***************************************************************************/
/* Note: year,month,date = calendar date, 1801-2099 only.             */
/*       Eastern longitude positive, Western longitude negative       */
/*       Northern latitude positive, Southern latitude negative       */
/*       The longitude value IS critical in this function!            */
/*       altit = the altitude which the Sun should cross              */
/*               Set to -35/60 degrees for rise/set, -6 degrees       */
/*               for civil, -12 degrees for nautical and -18          */
/*               degrees for astronomical twilight.                   */
/*         upper_limb: non-zero -> upper limb, zero -> center         */
/*               Set to non-zero (e.g. 1) when computing rise/set     */
/*               times, and to zero when computing start/end of       */
/*               twilight.                                            */
/*        *rise = where to store the rise time                        */
/*        *set  = where to store the set  time                        */
/*                Both times are relative to the specified altitude,  */
/*                and thus this function can be used to comupte       */
/*                various twilight times, as well as rise/set times   */
/* Return value:  0 = sun rises/sets this day, times stored at        */
/*                    *trise and *tset.                               */
/*               +1 = sun above the specified "horizon" 24 hours.     */
/*                    *trise set to time when the sun is at south,    */
/*                    minus 12 hours while *tset is set to the south  */
/*                    time plus 12 hours. "Day" length = 24 hours     */
/*               -1 = sun is below the specified "horizon" 24 hours   */
/*                    "Day" length = 0 hours, *trise and *tset are    */
/*                    both set to the time when the sun is at south.  */
/*                                                                    */
/**********************************************************************/
{
      double  d,  /* Days since 2000 Jan 0.0 (negative before) */
      sr,         /* Solar distance, astronomical units */
      sRA,        /* Sun's Right Ascension */
      sdec,       /* Sun's declination */
      sradius,    /* Sun's apparent radius */
      t,          /* Diurnal arc */
      tsouth,     /* Time when Sun is at south */
      sidtime;    /* Local sidereal time */

      int rc = 0; /* Return cde from function - usually 0 */

      /* Compute d of 12h local mean solar time */
      d = days_since_2000_Jan_0(year,month,day) + 0.5 - lon/360.0;

      /* Compute local sideral time of this moment */
      sidtime = revolution( GMST0(d) + 180.0 + lon );

      /* Compute Sun's RA + Decl at this moment */
      sun_RA_dec( d, &sRA, &sdec, &sr );

      /* Compute time when Sun is at south - in hours UT */
      tsouth = 12.0 - rev180(sidtime - sRA)/15.0;

      /* Compute the Sun's apparent radius, degrees */
      sradius = 0.2666 / sr;

      /* Do correction to upper limb, if necessary */
      if ( upper_limb )
            altit -= sradius;

      /* Compute the diurnal arc that the Sun traverses to reach */
      /* the specified altitide altit: */
      {
            double cost;
            cost = ( sind(altit) - sind(lat) * sind(sdec) ) /
                  ( cosd(lat) * cosd(sdec) );
            if ( cost >= 1.0 )
                  rc = -1, t = 0.0;       /* Sun always below altit */
            else if ( cost <= -1.0 )
                  rc = +1, t = 12.0;      /* Sun always above altit */
            else
                  t = acosd(cost)/15.0;   /* The diurnal arc, hours */
      }

      /* Store rise and set times - in hours UT */
      *trise = tsouth - t;
      *tset  = tsouth + t;

      return rc;
}  /* __sunriset__ */



/* The "workhorse" function */


double __daylen__( int year, int month, int day, double lon, double lat,
                   double altit, int upper_limb )
/**********************************************************************/
/* Note: year,month,date = calendar date, 1801-2099 only.             */
/*       Eastern longitude positive, Western longitude negative       */
/*       Northern latitude positive, Southern latitude negative       */
/*       The longitude value is not critical. Set it to the correct   */
/*       longitude if you're picky, otherwise set to to, say, 0.0     */
/*       The latitude however IS critical - be sure to get it correct */
/*       altit = the altitude which the Sun should cross              */
/*               Set to -35/60 degrees for rise/set, -6 degrees       */
/*               for civil, -12 degrees for nautical and -18          */
/*               degrees for astronomical twilight.                   */
/*         upper_limb: non-zero -> upper limb, zero -> center         */
/*               Set to non-zero (e.g. 1) when computing day length   */
/*               and to zero when computing day+twilight length.      */
/**********************************************************************/
{
      double  d,  /* Days since 2000 Jan 0.0 (negative before) */
      obl_ecl,    /* Obliquity (inclination) of Earth's axis */
      sr,         /* Solar distance, astronomical units */
      slon,       /* True solar longitude */
      sin_sdecl,  /* Sine of Sun's declination */
      cos_sdecl,  /* Cosine of Sun's declination */
      sradius,    /* Sun's apparent radius */
      t;          /* Diurnal arc */

      /* Compute d of 12h local mean solar time */
      d = days_since_2000_Jan_0(year,month,day) + 0.5 - lon/360.0;

      /* Compute obliquity of ecliptic (inclination of Earth's axis) */
      obl_ecl = 23.4393 - 3.563E-7 * d;

      /* Compute Sun's position */
      sunpos( d, &slon, &sr );

      /* Compute sine and cosine of Sun's declination */
      sin_sdecl = sind(obl_ecl) * sind(slon);
      cos_sdecl = sqrt( 1.0 - sin_sdecl * sin_sdecl );

      /* Compute the Sun's apparent radius, degrees */
      sradius = 0.2666 / sr;

      /* Do correction to upper limb, if necessary */
      if ( upper_limb )
            altit -= sradius;

      /* Compute the diurnal arc that the Sun traverses to reach */
      /* the specified altitide altit: */
      {
            double cost;
            cost = ( sind(altit) - sind(lat) * sin_sdecl ) /
                  ( cosd(lat) * cos_sdecl );
            if ( cost >= 1.0 )
                  t = 0.0;                      /* Sun always below altit */
            else if ( cost <= -1.0 )
                  t = 24.0;                     /* Sun always above altit */
            else  t = (2.0/15.0) * acosd(cost); /* The diurnal arc, hours */
      }
      return t;
}  /* __daylen__ */


/* This function computes the Sun's position at any instant */

void sunpos( double d, double *lon, double *r )
/******************************************************/
/* Computes the Sun's ecliptic longitude and distance */
/* at an instant given in d, number of days since     */
/* 2000 Jan 0.0.  The Sun's ecliptic latitude is not  */
/* computed, since it's always very near 0.           */
/******************************************************/
{
      double M,         /* Mean anomaly of the Sun */
             w,         /* Mean longitude of perihelion */
                        /* Note: Sun's mean longitude = M + w */
             e,         /* Eccentricity of Earth's orbit */
             E,         /* Eccentric anomaly */
             x, y,      /* x, y coordinates in orbit */
             v;         /* True anomaly */

      /* Compute mean elements */
      M = revolution( 356.0470 + 0.9856002585 * d );
      w = 282.9404 + 4.70935E-5 * d;
      e = 0.016709 - 1.151E-9 * d;

      /* Compute true longitude and radius vector */
      E = M + e * RADEG * sind(M) * ( 1.0 + e * cosd(M) );
      x = cosd(E) - e;
      y = sqrt( 1.0 - e*e ) * sind(E);
      *r = sqrt( x*x + y*y );              /* Solar distance */
      v = atan2d( y, x );                  /* True anomaly */
      *lon = v + w;                        /* True solar longitude */
      if ( *lon >= 360.0 )
            *lon -= 360.0;                   /* Make it 0..360 degrees */
}

void sun_RA_dec( double d, double *RA, double *dec, double *r )
{
  double lon, obl_ecl;
  double xs, ys, zs OY_UNUSED;
  double xe, ye, ze;
  
  /* Compute Sun's ecliptical coordinates */
  sunpos( d, &lon, r );
  
  /* Compute ecliptic rectangular coordinates */
  xs = *r * cosd(lon);
  ys = *r * sind(lon);
  zs = 0; /* because the Sun is always in the ecliptic plane! */

  /* Compute obliquity of ecliptic (inclination of Earth's axis) */
  obl_ecl = 23.4393 - 3.563E-7 * d;
  
  /* Convert to equatorial rectangular coordinates - x is unchanged */
  xe = xs;
  ye = ys * cosd(obl_ecl);
  ze = ys * sind(obl_ecl);
  
  /* Convert to spherical coordinates */
  *RA = atan2d( ye, xe );
  *dec = atan2d( ze, sqrt(xe*xe + ye*ye) );
      
}  /* sun_RA_dec */


/******************************************************************/
/* This function reduces any angle to within the first revolution */
/* by subtracting or adding even multiples of 360.0 until the     */
/* result is >= 0.0 and < 360.0                                   */
/******************************************************************/

#define INV360    ( 1.0 / 360.0 )

double revolution( double x )
/*****************************************/
/* Reduce angle to within 0..360 degrees */
/*****************************************/
{
      return( x - 360.0 * floor( x * INV360 ) );
}  /* revolution */

double rev180( double x )
/*********************************************/
/* Reduce angle to within -180..+180 degrees */
/*********************************************/
{
      return( x - 360.0 * floor( x * INV360 + 0.5 ) );
}  /* revolution */


/*******************************************************************/
/* This function computes GMST0, the Greenwhich Mean Sidereal Time */
/* at 0h UT (i.e. the sidereal time at the Greenwhich meridian at  */
/* 0h UT).  GMST is then the sidereal time at Greenwich at any     */
/* time of the day.  I've generelized GMST0 as well, and define it */
/* as:  GMST0 = GMST - UT  --  this allows GMST0 to be computed at */
/* other times than 0h UT as well.  While this sounds somewhat     */
/* contradictory, it is very practical:  instead of computing      */
/* GMST like:                                                      */
/*                                                                 */
/*  GMST = (GMST0) + UT * (366.2422/365.2422)                      */
/*                                                                 */
/* where (GMST0) is the GMST last time UT was 0 hours, one simply  */
/* computes:                                                       */
/*                                                                 */
/*  GMST = GMST0 + UT                                              */
/*                                                                 */
/* where GMST0 is the GMST "at 0h UT" but at the current moment!   */
/* Defined in this way, GMST0 will increase with about 4 min a     */
/* day.  It also happens that GMST0 (in degrees, 1 hr = 15 degr)   */
/* is equal to the Sun's mean longitude plus/minus 180 degrees!    */
/* (if we neglect aberration, which amounts to 20 seconds of arc   */
/* or 1.33 seconds of time)                                        */
/*                                                                 */
/*******************************************************************/

double GMST0( double d )
{
      double sidtim0;
      /* Sidtime at 0h UT = L (Sun's mean longitude) + 180.0 degr  */
      /* L = M + w, as defined in sunpos().  Since I'm too lazy to */
      /* add these numbers, I'll let the C compiler do it for me.  */
      /* Any decent C compiler will add the constants at compile   */
      /* time, imposing no runtime or code overhead.               */
      sidtim0 = revolution( ( 180.0 + 356.0470 + 282.9404 ) +
                          ( 0.9856002585 + 4.70935E-5 ) * d );
      return sidtim0;
} /* GMST0 */
/*  ---------------- 8< ------------------ */

double getSunHeight( double year, double month, double day, double gmt_hours,
                     double lat, double lon )
{
  double  d = days_since_2000_Jan_0(year,month,day) + 0.5 - lon/360.0,
      sr,         /* Solar distance, astronomical units */
      sdec,       /* Sun's declination */
      sRA,        /* Sun's Right Ascension */
      sidtime,    /* Local sidereal time */
      t,          /* Sun's local hour angle */
      hs,         /* Sun's Height sinus */
      h,          /* Sun's Height */
      A;          /* Sun's Azimut */

  int hour,minute,second;
  oySplitHour( oyGetCurrentLocalHour( gmt_hours, 0 ), &hour, &minute, &second );
  if(oy_debug)
  fprintf( stderr, "GMT:\t%02d:%02d:%02d\n", hour,minute,second );
  if(oy_debug)
  fprintf( stderr, "JD (GMT 12:00):\t%fd\n", d + 2451545.0 + lon/360.0 );
  if(oy_debug)
  fprintf( stderr, "JD0 (GMT 12:00):\t%fd\n", d + lon/360.0 );
  if(oy_debug)
  fprintf( stderr, "LMST:\t%fd\n", d + gmt_hours/24.0*365.25/366.25 );

  /* Compute local sideral time of this moment */
  sidtime = revolution( GMST0(d) + 360.*gmt_hours/24.*365.25/366.25 + lon );
  oySplitHour( oyGetCurrentLocalHour( sidtime/15., 0 ), &hour, &minute, &second );
  if(oy_debug)
  fprintf( stderr, "Local Mean Sidereal Time:\t%02d:%02d:%02d\n", hour,minute,second );

  sun_RA_dec( d, &sRA, &sdec, &sr );
  if(oy_debug)
  fprintf( stderr, "Rectaszension:\t%g°\n", sRA);
  if(oy_debug)
  fprintf( stderr, "Declination:\t%g°\n", sdec);
  t = sidtime - sRA;
  if(oy_debug)
  fprintf( stderr, "Sun's Hourly Angle:\t%g° (%gh)\n", t, t/15.);
  A = atand( sind( t ) /
             ( cosd( t )*sind( lon ) - tand( sdec )*cosd( lon ) )
           );
  if(oy_debug)
  fprintf( stderr, "Sun's Azimut:\t%g°\n", revolution(A-180.0) );
  hs = cosd( sdec )*cosd( t )*cosd( lat ) + sind( sdec )*sind( lat );
  h = asind( hs );
  if(oy_debug)
  fprintf( stderr, "Sun's Height:\t%g° sin(%g)\n", h, hs );

  return h;
}
