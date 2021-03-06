/*  @file openicc_args.c
 *
 *  libOpenIcc - OpenICC Colour Management Configuration
 *
 *  @par Copyright:
 *            2018 (C) Kai-Uwe Behrmann
 *
 *  @brief    OpenICC argument handling
 *  @author   Kai-Uwe Behrmann <ku.b@gmx.de>
 *  @par License:
 *            MIT <http://www.opensource.org/licenses/mit-license.php>
 *  @since    2018/06/00
 */

#include "openicc_core.h"
#include "openicc_config_internal.h"

#include <oyjl.h>

#include <stddef.h>
#include <ctype.h> /* toupper() */
#ifdef HAVE_LANGINFO_H
#include <langinfo.h>
#endif

/** \addtogroup args Options Handling
 *  @brief   Structured Options and Arguments for more than the command line
 *
 *  Argument handling uses a compact, table like creation syntax.
 *  openiccUi_Create() is a high level API for tools to feed only the
 *  declarations and obtain the results in one call. On a lower level parsing
 *  with openiccOptions_Parse() detects conflicts during programming and
 *  on run time. The arguments can be printed as a typical command line tool
 *  help text with openiccOptions_PrintHelp(). The OpenICC JSON
 *  output from openiccUi_ToJson() is useable for automatical generated
 *  (G)UI's and further processing. openiccUi_ToMan() creates unix manual pages.
 *  openiccUi_ToMarkdown() lets you produce a man page for your web page.
 *  Generation of other formats is simple.
 *  Translations are supported.
 *
 *  Command line options support single letter and long options without
 *  and with empty space and equal sign style single argument.
 *  @verbatim
    > myProgramm -o --option -w=1 --with-argument 1
    @endverbatim
 *
 *  **Basic Tutorial** from @ref openicc_config_read.c :
    @dontinclude openicc_config_read.c
    @skip handle options
    @until done with options handling
 *
 *  @{ */

/** @brief    Release dynamic structure
 *  @memberof openiccOptionChoice_s
 *
 *  @version OpenICC: 0.1.1
 *  @date    2018/08/14
 *  @since   2018/08/14 (OpenICC: 0.1.1)
 */
void openiccOptionChoice_Release     ( openiccOptionChoice_s**choices )
{
  int n = 0,i;
  openiccOptionChoice_s * ca;
  if(!choices || !*choices) return;
  ca = *choices;
  while(ca[n].nick[0] != '\000') ++n;
  for(i = 0; i < n; ++i)
  {
    openiccOptionChoice_s * c = &ca[i];
    if(c->nick) free(c->nick);
    if(c->name) free(c->name);
    if(c->description) free(c->description);
    if(c->help) free(c->help);
  }
  *choices = NULL;
  free(*choices);
}

/** @brief    Return number of "oiwi" array elements
 *  @memberof openiccOptions_s
 *
 *  @version OpenICC: 0.1.1
 *  @date    2018/08/14
 *  @since   2018/08/14 (OpenICC: 0.1.1)
 */
int openiccOptions_Count             ( openiccOptions_s  * opts )
{
  int n = 0;
  while(openiccObjectToType( &opts->array[n] ) /*"oiwi"*/ == openiccOBJECT_OPTION) ++n;
  return n;
}

/** @brief    Return number of "oiwi" groups elements
 *  @memberof openiccOptions_s
 *
 *  @version OpenICC: 0.1.1
 *  @date    2018/08/14
 *  @since   2018/08/14 (OpenICC: 0.1.1)
 */
int openiccOptions_CountGroups       ( openiccOptions_s  * opts )
{
  int n = 0;
  while(openiccObjectToType( &opts->groups[n] ) /*"oiwg"*/ == openiccOBJECT_OPTION_GROUP) ++n;
  return n;
}


enum {
  openiccOPTIONSTYLE_ONELETTER = 0x01,
  openiccOPTIONSTYLE_STRING = 0x02,
  openiccOPTIONSTYLE_OPTIONAL_START = 0x04,
  openiccOPTIONSTYLE_OPTIONAL_END = 0x08,
  openiccOPTIONSTYLE_OPTIONAL_INSIDE_GROUP = 0x10,
  openiccOPTIONSTYLE_MAN = 0x20,
  openiccOPTIONSTYLE_MARKDOWN = 0x40
};
#define openiccOPTIONSTYLE_OPTIONAL (openiccOPTIONSTYLE_OPTIONAL_START | openiccOPTIONSTYLE_OPTIONAL_END)
static
const char * openiccOption_PrintArg  ( openiccOption_s   * o,
                                       int                 style )
{
  static char * text = NULL;
  if(text) text[0] = '\000';
  if(!o) return "";
  if(style & openiccOPTIONSTYLE_OPTIONAL_START)
    oyjlStringAdd( &text, malloc, free, "[" );
  if(style & openiccOPTIONSTYLE_ONELETTER)
  {
    if(style & openiccOPTIONSTYLE_MAN)
      oyjlStringAdd( &text, malloc, free, "\\-%c", o->o );
    else
      oyjlStringAdd( &text, malloc, free, "-%c", o->o );
  }
  if(style & openiccOPTIONSTYLE_ONELETTER && style & openiccOPTIONSTYLE_STRING)
    oyjlStringAdd( &text, malloc, free, "|" );
  if(style & openiccOPTIONSTYLE_STRING)
  {
    if(style & openiccOPTIONSTYLE_MAN)
      oyjlStringAdd( &text, malloc, free, "\\-\\-%s", o->option );
    else
      oyjlStringAdd( &text, malloc, free, "--%s", o->option );
  }
  if(o->value_name)
  {
    if(style & openiccOPTIONSTYLE_MAN)
      oyjlStringAdd( &text, malloc, free, " \\fI%s\\fR", o->value_name );
    if(style & openiccOPTIONSTYLE_MARKDOWN)
      oyjlStringAdd( &text, malloc, free, " *%s*", o->value_name );
    else if(style & openiccOPTIONSTYLE_OPTIONAL_INSIDE_GROUP)
      oyjlStringAdd( &text, malloc, free, "%s", o->value_name );
    else
      oyjlStringAdd( &text, malloc, free, " %s", o->value_name );
  }
  if(style & openiccOPTIONSTYLE_OPTIONAL_END)
    oyjlStringAdd( &text, malloc, free, "]" );
  return text;
}

openiccOptionChoice_s openicc_X_choices[] = {{"json", "", "", ""},
                                    {"json+command", "", "", ""},
                                    {"man", "", "", ""},
                                    {"markdown", "", "", ""},
                                    {"","","",""}};
void openiccOptions_EnrichInbuild( openiccOption_s * o )
{
  int oc = o->o;

  if(oc == 'X')
  {
    if(o->value_type == openiccOPTIONTYPE_CHOICE && o->values.choices.list == NULL)
    {
      openicc_X_choices[0].name = _("Json");
      openicc_X_choices[0].description = _("GUI");
      openicc_X_choices[0].help = _("Get a OpenICC Json UI declaration");
      openicc_X_choices[1].name = _("Json + Command");
      openicc_X_choices[1].description = _("GUI + Command");
      openicc_X_choices[1].help = _("Get OpenICC Json UI declaration incuding command");
      openicc_X_choices[2].name = _("Man");
      openicc_X_choices[2].description = _("Unix Man page");
      openicc_X_choices[2].help = _("Get a unix man page");
      openicc_X_choices[3].name = _("Markdown");
      openicc_X_choices[3].description = _("Formated text");
      openicc_X_choices[3].help = _("Get formated text");
      o->values.choices.list = openicc_X_choices;
      if(o->value_name == NULL)
      {
        o->value_name = "json|json+command|man|markdown";
        if(o->name == NULL)
        {
          o->name = _("export");
          if(o->description == NULL)
          {
            o->description = _("Export formated text");
            if(o->help == NULL)
              o->help = _("Get UI converted into text formats");
          }
        }
      }
    }
  }
}

/** @brief    Obtain the specified option from option char
 *  @memberof openiccOptions_s
 *
 *  @version OpenICC: 0.1.1
 *  @date    2018/08/14
 *  @since   2018/08/14 (OpenICC: 0.1.1)
 */
openiccOption_s * openiccOptions_GetOption (
                                       openiccOptions_s  * opts,
                                       char                oc )
{
  int i;
  int nopts = openiccOptions_Count( opts );
  openiccOption_s * o = NULL;

  for(i = 0; i < nopts; ++i)
  {
    o = &opts->array[i];
    if(o->o && o->o == oc)
    {
      if(oc == 'X' && o->value_type == openiccOPTIONTYPE_CHOICE && o->values.choices.list == NULL)
        openiccOptions_EnrichInbuild(o);

      return o;
    }
    else
      o = NULL;
  }
  return o;
}

/** @brief    Obtain the specified option from option string
 *  @memberof openiccOptions_s
 *
 *  @version OpenICC: 0.1.1
 *  @date    2018/08/14
 *  @since   2018/08/14 (OpenICC: 0.1.1)
 */
openiccOption_s * openiccOptions_GetOptionL (
                                       openiccOptions_s  * opts,
                                       const char        * ostring )
{
  int i;
  int nopts = openiccOptions_Count( opts );
  openiccOption_s * o = NULL;
  char * str = oyjlStringCopy(ostring, malloc);
  char * t = strchr(str, '=');

  if(t)
    t[0] = '\000';

  for(i = 0; i < nopts; ++i)
  {
    o = &opts->array[i];
    if(o->option && strcmp(o->option, str) == 0)
      return o;
    else
      o = NULL;
  }
  fprintf( stderr, "Option not found: %s\n", str );

  return o;
}
static
openiccOPTIONSTATE_e openiccOptions_Check (
                                       openiccOptions_s  * opts )
{
  int i,j;
  int nopts = openiccOptions_Count( opts );
  openiccOption_s * o = NULL, * b = NULL;

  for(i = 0; i < nopts; ++i)
  {
    o = &opts->array[i];
    for(j = i+1; j < nopts; ++j)
    {
      b = &opts->array[j];
      if(o->o == b->o)
      {
        fprintf( stderr, "%s %s \'%c\'\n", _("Usage Error:"), _("Double occuring option"), b->o );
        return openiccOPTION_DOUBLE_OCCURENCE;
      }
    }
  }
  return openiccOPTION_NONE;
}

/** @brief    Parse the options into a private data structure
 *  @memberof openiccOptions_s
 *
 *  The returned status can be used to detect usage errors and hint them on
 *  the command line.
 *  In the usual case where the variable fields are set, the results
 *  will be set too.
 *
 *  @version OpenICC: 0.1.1
 *  @date    2018/08/19
 *  @since   2018/08/14 (OpenICC: 0.1.1)
 */
openiccOPTIONSTATE_e openiccOptions_Parse (
                                       openiccOptions_s  * opts )
{
  openiccOPTIONSTATE_e state = openiccOPTION_NONE;
  openiccOption_s * o;
  char ** result;

  /* parse the command line arguments */
  if(!opts->private_data)
  {
    int i, pos = 0;
    result = (char**) calloc( 2, sizeof(char*) );
    result[0] = (char*) calloc( 65536, sizeof(char) );
    if((state = openiccOptions_Check(opts)) != openiccOPTION_NONE)
      goto clean_parse;
    for(i = 1; i < opts->argc; ++i)
    {
      char * str = opts->argv[i];
      int l = strlen(str);
      char arg = ' ';
      char * long_arg = NULL;
      char * value = NULL;

      /* parse -a | -a value | -a=value | -ba | -ba value | -ba=value */
           if(l > 1 && str[0] == '-' && str[1] != '-')
      {
        int require_value, j;
        for(j = 1; j < l; ++j)
        {
          arg = str[j];
          o = openiccOptions_GetOption( opts, arg );
          if(!o)
          {
            fprintf( stderr, "%s %s \'%c\'\n", _("Usage Error:"), _("Option not supported"), arg );
            state = openiccOPTION_NOT_SUPPORTED;
            break;
          }
          require_value = o->value_type != openiccOPTIONTYPE_NONE;
          if( require_value )
          {
            value = NULL;
            if( j == l-1 && opts->argc > i+1 && (opts->argv[i+1][0] != '-' || strlen(opts->argv[i+1]) <= 1) )
            {
              value = opts->argv[i+1];
              ++i;
            }
            else if( str[j+1] == '=' )
            {
              ++j;
              value = &str[j+1];
              j = l;
            }
            else
            {
              fprintf( stderr, "%s %s \'%c\'\n", _("Usage Error:"), _("Option needs a argument"), arg );
              state = openiccOPTION_MISSING_VALUE;
            }
            if(value)
            {
              int llen = 0;
              while(result[llen]) ++llen;
              result[0][pos] = arg;
              ++pos;
              oyjlStringListAddStaticString( &result, &llen, value, malloc, free );
            }
          }
          else if(!require_value && !(j < l-1 && str[j+1] == '='))
          {
            int llen = 0;
            while(result[llen]) ++llen;
            result[0][pos] = arg;
            ++pos;
            oyjlStringListAddStaticString( &result, &llen, "1", malloc, free );
          }
          else
          {
            int i;
            for(i = 0; i < opts->argc; ++i)
              fprintf( stderr, "\'%s\' ", opts->argv[i]);
            fprintf( stderr, "\n");
            fprintf( stderr, "%s %s \'%c\'\n", _("Usage Error:"), _("Option has a unexpected argument"), arg );
            state = openiccOPTION_UNEXPECTED_VALUE;
            j = l;
          }
        }
      }
      /* parse --arg | --arg value | --arg=value */
      else if(l > 2 && str[0] == '-' && str[1] == '-')
      {
        int require_value;
        long_arg = &str[2];
        o = openiccOptions_GetOptionL( opts, long_arg );
        if(!o)
        {
          fprintf( stderr, "%s %s \'%s\'\n", _("Usage Error:"), _("Option not supported"), long_arg );
          state = openiccOPTION_NOT_SUPPORTED;
          goto clean_parse;
        }
        require_value = o->value_type != openiccOPTIONTYPE_NONE;
        if( require_value )
        {
          value = NULL;

          if( strchr(str, '=') != NULL )
            value = strchr(str, '=') + 1;
          else if( opts->argc > i+1 && opts->argv[i+1][0] != '-' )
          {
            value = opts->argv[i+1];
            ++i;
          }
          else
          {
            fprintf( stderr, "%s %s \'%s\'\n", _("Usage Error:"), _("Option needs a argument"), long_arg );
            state = openiccOPTION_MISSING_VALUE;
          }

          if(value)
          {
            int llen = 0;
            while(result[llen]) ++llen;
            result[0][pos] = o->o;
            ++pos;
            oyjlStringListAddStaticString( &result, &llen, value, malloc, free );
          }
        } else
        {
          if(!( strchr(str, '=') != NULL || (opts->argc > i+1 && opts->argv[i+1][0] != '-') ))
          {
            int llen = 0;
            while(result[llen]) ++llen;
            result[0][pos] = o->o;
            ++pos;
            oyjlStringListAddStaticString( &result, &llen, "1", malloc, free );
          } else
          {
            fprintf( stderr, "%s %s \'%s\'\n", _("Usage Error:"), _("Option has a unexpected argument"), opts->argv[i+1] );
            state = openiccOPTION_UNEXPECTED_VALUE;
          }
        }
      }
      /* parse anonymous value, if requested */
      else
      {
        result[0][pos] = '-';
        ++pos;
      }
    }
    opts->private_data = result;

    pos = 0;
    while(result[0][pos])
    {
      openiccOption_s * o = openiccOptions_GetOption( opts, result[0][pos] );
      if(o)
      switch(o->variable_type)
      {
        case openiccNONE:   break;
        case openiccSTRING: openiccOptions_GetResult( opts, o->o, o->variable.s, 0, 0 ); break;
        case openiccDOUBLE: openiccOptions_GetResult( opts, o->o, 0, o->variable.d, 0 ); break;
        case openiccINT:    openiccOptions_GetResult( opts, o->o, 0, 0, o->variable.i ); break;
      }
      ++pos;
    }
  }

  return state;

clean_parse:
  free(result[0]);
  free(result);
  return state;
}

/** @brief    Obtain the parsed result
 *  @memberof openiccOptions_s
 *
 *  This function is only useful, if the results shall be obtained
 *  independently from openiccOptions_Parse().
 *
 *  If the option was not specified the state openiccOPTION_NONE will be
 *  returned and otherwise openiccOPTION_USER_CHANGED. With result_int and
 *  a option type of openiccOPTIONTYPE_NONE, the number of occurences is
 *  obtained, e.g. -vvv will give result_int = 3. A option type
 *  openiccOPTIONTYPE_DOUBLE can ask for the floating point result with a
 *  result_dbl argument.
 *
 *  @version OpenICC: 0.1.1
 *  @date    2018/08/14
 *  @since   2018/08/14 (OpenICC: 0.1.1)
 */
openiccOPTIONSTATE_e openiccOptions_GetResult (
                                       openiccOptions_s  * opts,
                                       char                oc,
                                       const char       ** result_string,
                                       double            * result_dbl,
                                       int               * result_int )
{
  openiccOPTIONSTATE_e state = openiccOPTION_NONE;
  ptrdiff_t pos = -1;
  const char * t;
  const char ** results;
  const char * list;

  /* parse the command line arguments */
  if(!opts->private_data)
    state = openiccOptions_Parse( opts );
  if(state != openiccOPTION_NONE)
    return state;

  results = opts->private_data;
  list = results[0];
  t = strrchr( list, oc );
  if(t)
  {
    pos = t - list;
    state = openiccOPTION_USER_CHANGED;
  }
  else if(oc == ' ' && strlen(list))
  {
    if(result_int)
      *result_int = strlen(list);
    return openiccOPTION_USER_CHANGED;
  }
  else
    return openiccOPTION_NONE;

  t = results[1 + pos];

  if(result_string)
    *result_string = t;

  if(result_dbl)
  {
    openiccOption_s * o = openiccOptions_GetOption( opts, oc );
    oyjlStringToDouble( t, result_dbl );
    if( o->value_type == openiccOPTIONTYPE_DOUBLE &&
        ( o->values.dbl.start > *result_dbl ||
          o->values.dbl.end < *result_dbl) )
    {
      fprintf( stderr, "%s %s \'%c\' %s %g->%g !: %g\n", _("Usage Error:"), _("Option has a different value range"), oc, o->name, o->values.dbl.start, o->values.dbl.end, *result_dbl  );
    }
  }
  if(result_int)
  {
    int l = strlen( list ), i,n = 0;
    openiccOption_s * o = openiccOptions_GetOption( opts, oc );
    if(o->value_type == openiccOPTIONTYPE_NONE)
    {
      for(i = 0; i < l; ++i)
        if(list[i] == oc)
          ++n;
      *result_int = n;
    } else
    {
      long lo = 0;
      if(oyjlStringToLong( t, &lo ) == 0)
        *result_int = lo;
    }
  }

  return state;
}

/** @brief    Convert the parsed content to JSON
 *  @memberof openiccOptions_s
 *
 *  @version OpenICC: 0.1.1
 *  @date    2018/08/14
 *  @since   2018/08/14 (OpenICC: 0.1.1)
 */
char * openiccOptions_ResultsToJson  ( openiccOptions_s  * opts )
{
  char * args = NULL,
       * rjson = NULL;
  char ** results = opts->private_data;
  oyjl_val root, value;
  int i,n;

  if(!results)
  {
    if(openiccOptions_Parse( opts ))
      return NULL;

    results = opts->private_data;
    if(!results)
      return NULL;
  }

  args = results[0];
  n = strlen( args );
  root = oyjlTreeNew( "" );
  for(i = 0; i < n; ++i)
  {
    char a[4] = {0,0,0,0};
    a[0] = args[i];
    value = oyjlTreeGetValue( root, OYJL_CREATE_NEW, a );
    oyjlValueSetString( value, results[i+1] );
  }

  i = 0;
  oyjlTreeToJson( root, &i, &rjson );
  oyjlTreeFree( root );

  return rjson;
}

/** @brief    Convert the parsed content to simple text
 *  @memberof openiccOptions_s
 *
 *  @version OpenICC: 0.1.1
 *  @date    2018/08/14
 *  @since   2018/08/14 (OpenICC: 0.1.1)
 */
char * openiccOptions_ResultsToText  ( openiccOptions_s  * opts )
{
  char * args = NULL,
       * text = NULL;
  char ** results = opts->private_data;
  int i,n;

  if(!results)
  {
    if(openiccOptions_Parse( opts ))
      return NULL;

    results = opts->private_data;
    if(!results)
      return NULL;
  }

  args = results[0];
  n = strlen( args );
  for(i = 0; i < n; ++i)
  {
    char a[4] = {args[i],0,0,0};
    char * value = results[i+1];
    oyjlStringAdd( &text, malloc, free, "%s:%s\n", a, value );
  }

  return text;
}

/** @internal
 *  @brief    Print synopsis of a option group to stderr
 *  @memberof openiccOptions_s
 *
 *  @version OpenICC: 0.1.1
 *  @date    2018/08/14
 *  @since   2018/08/14 (OpenICC: 0.1.1)
 */
static
const char * openiccOptions_PrintHelpSynopsis (
                                       openiccOptions_s  * opts,
                                       openiccOptionGroup_s * g,
                                       int                 style )
{
  int i;
  int m = g->mandatory ? strlen(g->mandatory) : 0;
  int on = g->optional ? strlen(g->optional) : 0;
  static char * text = NULL;
  int opt_group = 0;
  int gstyle = style;

  if(text) text[0] = 0;

  if( m || on )
  {
    if(style & openiccOPTIONSTYLE_MAN)
      oyjlStringAdd( &text, malloc, free, "\\fB%s\\fR", opts->argv[0] );
    else if(style & openiccOPTIONSTYLE_MARKDOWN)
      oyjlStringAdd( &text, malloc, free, "**%s**", opts->argv[0] );
    else
      oyjlStringAdd( &text, malloc, free, "%s", opts->argv[0] );
  }
  else
    return text;

  for(i = 0; i < m; ++i)
  {
    char oc = g->mandatory[i];
    openiccOption_s * o = openiccOptions_GetOption( opts, oc );
    if(oc == '|')
      oyjlStringAdd( &text, malloc, free, "|" );
    else if(!o)
    {
      fprintf(stderr, "\n%s: option not declared: %c\n", g->name, oc);
      exit(1);
    }
    oyjlStringAdd( &text, malloc, free, " %s", openiccOption_PrintArg(o, style) );
  }
  for(i = 0; i < on; ++i)
  {
    char oc = g->optional[i];
    openiccOption_s * o = openiccOptions_GetOption( opts, oc );
    gstyle = style | openiccOPTIONSTYLE_OPTIONAL;
    if(i < on - 1 && g->optional[i+1] == '|')
    {
      if(opt_group == 0)
        gstyle = style | openiccOPTIONSTYLE_OPTIONAL_START | openiccOPTIONSTYLE_OPTIONAL_INSIDE_GROUP;
      else
        gstyle = style | openiccOPTIONSTYLE_OPTIONAL_INSIDE_GROUP;
      opt_group = 1;
    }
    else if(oc == '|')
    {
      oyjlStringAdd( &text, malloc, free, "|" );
      continue;
    }
    else if(opt_group)
    {
      gstyle = style | openiccOPTIONSTYLE_OPTIONAL_END;
      opt_group = 0;
    }
    else if(!o)
    {
      fprintf(stderr, "\n%s: option not declared: %c\n", g->name, oc);
      exit(1);
    }

    oyjlStringAdd( &text, malloc, free, "%s%s", gstyle & openiccOPTIONSTYLE_OPTIONAL_START ? " ":"", openiccOption_PrintArg(o, gstyle) );
  }
  return text;
}

static openiccOptionChoice_s ** openicc_get_choices_list_ = NULL;
static int openicc_get_choices_list_selected_[256];
static
openiccOptionChoice_s * openiccOption_GetChoices_ (
                                       openiccOption_s      * o,
                                       int               * selected,
                                       openiccOptions_s     * opts )
{
  if(!o) return NULL;

  if(!openicc_get_choices_list_)
  {
    int i;
    for(i = 0; i < 256; ++i) openicc_get_choices_list_selected_[i] = -1;
    openicc_get_choices_list_ = calloc( sizeof(openiccOptionChoice_s*), 256 ); /* number of possible chars */
  }

  if( !openicc_get_choices_list_[(int)o->o] ||
      (selected && openicc_get_choices_list_selected_[(int)o->o] == -1) )
    openicc_get_choices_list_[(int)o->o] = o->values.getChoices(o, selected ? &openicc_get_choices_list_selected_[(int)o->o] : selected, opts );

  if(selected)
    *selected = openicc_get_choices_list_selected_[(int)o->o];
  return openicc_get_choices_list_[(int)o->o];
}

#include <stdarg.h> /* va_list */
/** @brief    Print help text to stderr
 *  @memberof openiccOptions_s
 *
 *  @param   opts                      options to print
 *  @param   ui                        more info for e.g. from the documentation section for the description block; optional
 *  @param   verbose                   gives debug output
 *  @param   motto_format              prints a customised intoduction line
 *
 *  @version OpenICC: 0.1.1
 *  @date    2018/08/14
 *  @since   2018/08/14 (OpenICC: 0.1.1)
 */
void  openiccOptions_PrintHelp       ( openiccOptions_s  * opts,
                                       openiccUi_s       * ui,
                                       int                 verbose,
                                       const char        * motto_format,
                                                           ... )
{
  int i,ng;
  va_list list;
  int indent = 2;
  openiccUiHeaderSection_s * section = NULL;
  fprintf( stderr, "\n");
  if(verbose)
  {
    for(i = 0; i < opts->argc; ++i)
      fprintf( stderr, "\'%s\' ", opts->argv[i]);
    fprintf( stderr, "\n");
  }

  if(!motto_format)
  {
    openiccUiHeaderSection_s * version = openiccUi_GetHeaderSection( ui,
                                                               "version" );
    fprintf( stderr, "%s v%s - %s", opts->argv[0],
                              version && version->name ? version->name : "",
                              ui->description ? ui->description : "" );
  }
  else
  {
    va_start( list, motto_format );
    vfprintf( stderr, motto_format, list );
    va_end  ( list );
  }
  fprintf( stderr, "\n");

  ng = openiccOptions_CountGroups(opts);
  if(!ng) return;

  if( ui && (section = openiccUi_GetHeaderSection(ui, "documentation")) != NULL &&
      section->description )
    fprintf( stderr, "\n%s:\n  %s\n", _("Description"), section->description );

  fprintf( stderr, "\n%s:\n", _("Synopsis") );
  for(i = 0; i < ng; ++i)
  {
    openiccOptionGroup_s * g = &opts->groups[i];
    fprintf( stderr, "  %s\n", openiccOptions_PrintHelpSynopsis( opts, g, openiccOPTIONSTYLE_ONELETTER ) );
  }

  fprintf( stderr, "\n%s:\n", _("Usage")  );
  for(i = 0; i < ng; ++i)
  {
    openiccOptionGroup_s * g = &opts->groups[i];
    int d = g->detail ? strlen(g->detail) : 0,
        j,k;
    fprintf( stderr, "  %s\n", g->description  );
    if(g->mandatory && g->mandatory[0])
    {
      fprintf( stderr, "\t%s\n", openiccOptions_PrintHelpSynopsis( opts, g, openiccOPTIONSTYLE_ONELETTER ) );
    }
    for(j = 0; j < d; ++j)
    {
      char oc = g->detail[j];
      openiccOption_s * o = openiccOptions_GetOption( opts, oc );
      if(!o)
      {
        fprintf(stderr, "\n%s: option not declared: %c\n", g->name, oc);
        exit(1);
      }
      for(k = 0; k < indent; ++k) fprintf( stderr, " " );
      switch(o->value_type)
      {
        case openiccOPTIONTYPE_CHOICE:
          {
            int n = 0,l;
            if(o->value_name)
            {
              fprintf( stderr, "\t" );
              fprintf( stderr, "%s", openiccOption_PrintArg(o, openiccOPTIONSTYLE_ONELETTER | openiccOPTIONSTYLE_STRING) );
              fprintf( stderr, "\t%s%s%s\n", o->description ? o->description:"", o->help?": ":"", o->help?o->help :"" );
            }
            while(o->values.choices.list[n].nick[0] != '\000')
              ++n;
            for(l = 0; l < n; ++l)
              fprintf( stderr, "\t  -%c %s\t\t# %s%s%s\n",
                  o->o,
                  o->values.choices.list[l].nick,
                  o->values.choices.list[l].name && o->values.choices.list[l].nick[0] ? o->values.choices.list[l].name : o->values.choices.list[l].description,
                  o->values.choices.list[l].help?" - ":"",
                  o->values.choices.list[l].help?o->values.choices.list[l].help:"" );
          }
          break;
        case openiccOPTIONTYPE_FUNCTION:
          {
            int n = 0,l;
            openiccOptionChoice_s * list = openiccOption_GetChoices_(o, NULL, opts );
            if(list)
              while(list[n].nick[0] != '\000')
                ++n;
            for(l = 0; l < n; ++l)
              fprintf( stderr, "\t  -%c %s\t\t# %s\n", o->o, list[l].nick, list[l].name && list[l].nick[0] ? list[l].name : list[l].description );
            /* not possible, as the result of openiccOption_GetChoices_() is cached - openiccOptionChoice_Release( &list ); */
          }
          break;
        case openiccOPTIONTYPE_DOUBLE:
          fprintf( stderr, "\t" );
          fprintf( stderr, "%s", openiccOption_PrintArg(o, openiccOPTIONSTYLE_ONELETTER | openiccOPTIONSTYLE_STRING) );
          fprintf( stderr, "\t%s%s%s\n", o->description ? o->description:"", o->help?": ":"", o->help?o->help :"" );
          break;
        case openiccOPTIONTYPE_STRING:
          fprintf( stderr, "\t" );
          fprintf( stderr, "%s", openiccOption_PrintArg(o, openiccOPTIONSTYLE_ONELETTER | openiccOPTIONSTYLE_STRING) );
          fprintf( stderr, "\t%s%s%s\n", o->description ? o->description:"", o->help?": ":"", o->help?o->help :"" );
          break;
        case openiccOPTIONTYPE_NONE:
          fprintf( stderr, "\t" );
          fprintf( stderr, "%s", openiccOption_PrintArg(o, openiccOPTIONSTYLE_ONELETTER | openiccOPTIONSTYLE_STRING) );
          fprintf( stderr, "\t%s%s%s\n", o->description ? o->description:"", o->help?": ":"", o->help?o->help :"" );
        break;
        case openiccOPTIONTYPE_START: break;
        case openiccOPTIONTYPE_END: break;
      }
    }
    if(d) fprintf( stderr, "\n" );
  }
  fprintf( stderr, "\n" );
}

/** @brief    Allocate a new options structure
 *  @memberof openiccOptions_s
 *
 *  @version OpenICC: 0.1.1
 *  @date    2018/08/14
 *  @since   2018/08/14 (OpenICC: 0.1.1)
 */
openiccOptions_s * openiccOptions_New( int                 argc,
                                       char             ** argv )
{
  openiccOptions_s * opts = calloc( sizeof(openiccOptions_s), 1 );
  memcpy( opts->type, "oiws", 4 );

  opts->argc = argc;
  opts->argv = argv;
  return opts;
}

/** @brief    Allocate a new ui structure
 *  @memberof openiccUi_s
 *
 *  The openiccUi_s contains already options in the opts member.
 *
 *  @version OpenICC: 0.1.1
 *  @date    2018/08/14
 *  @since   2018/08/14 (OpenICC: 0.1.1)
 */
openiccUi_s* openiccUi_New           ( int                 argc,
                                       char             ** argv )
{
  openiccUi_s * ui = calloc( sizeof(openiccUi_s), 1 );
  memcpy( ui->type, "oiui", 4 );
  ui->opts = openiccOptions_New( argc, argv );
  return ui;
}

/** @brief    Create a new UI structure
 *  @memberof openiccUi_s
 *
 *  This is a high level convinience function.
 *  The returned openiccUi_s is a comlete description of the UI and can be
 *  used instantly. The options are parsed, errors are printed, help text
 *  is printed for the boolean -h/--help option. Boolean -v/--verbose
 *  is handled too. The results are set to the declared variables. 
 *  The app_type defaults to "tool", but it can be replaced if needed.
 *
 *  @code
  openiccUi_s * ui = openiccUi_Create( argc, argv,
                                       "myCl",
                                       _("My Command"),
                                       _("My Command line tool from Me"),
                                       "my_logo",
                                       info, options, groups, NULL )
    @endcode
 *
 *  @param[in]     argc                number of command line arguments
 *  @param[in]     argv                command line args from C/C++ main()
 *  @param[in]     nick                four byte string; e.g. "myCl"
 *  @param[in]     name                short name of the tool; i18n;
 *                 e.g. _("My Command")
 *  @param[in]     description         compact sentence starting with full name; i18n;
 *                 e.g. _("My Command line tool from Me")
 *  @param[in]     logo                icon name; This variable must contain
 *                 the file name only, without ending. The icon needs
 *                 to be installed in typical icon search path and will be
 *                 detected there. e.g. "my_logo" points to "my_logo.{png|svg}"
 *  @param[in]     info                general information for rich UI's and
 *                                     for help text
 *  @param[in,out] options             the main option declaration, with
 *                 syntax declaration and variable passing for setting results
 *  @param[in]     groups              the option grouping declares
 *                 dependencies of options and provides a UI layout
 *  @param[out]    state               inform about processing
 *                                     - openiccUI_STATE_HELP : help was detected, printed and openiccUi_s was released
 *                                     - openiccUI_STATE_EXPORT : export of json, man or markdown was detected, printed and openiccUi_s was released
 *                                     - openiccUI_STATE_VERBOSE : verbose was detected
 *                                     - openiccUI_STATE_OPTION+ : error occured in option parser, message printed, openiccOPTIONSTATE_e is placed in >> openiccUI_STATE_OPTION and openiccUi_s was released
 *  @return                            UI object for later use
 *
 *  @version OpenICC: 0.1.1
 *  @date    2018/11/09
 *  @since   2018/08/20 (OpenICC: 0.1.1)
 */
openiccUi_s *  openiccUi_Create      ( int                 argc,
                                       char             ** argv,
                                       const char        * nick,
                                       const char        * name,
                                       const char        * description,
                                       const char        * logo,
                                       openiccUiHeaderSection_s * info,
                                       openiccOption_s   * options,
                                       openiccOptionGroup_s * groups,
                                       int               * status )
{
  int help = 0, verbose = 0;
  const char * export = NULL;
  openiccOption_s * h, * v, * X;
  openiccOPTIONSTATE_e opt_state = openiccOPTION_NONE;
  openiccInit();

  /* allocate options structure */
  openiccUi_s * ui = openiccUi_New( argc, argv ); /* argc+argv are required for parsing the command line options */
  /* tell about the tool */
  ui->app_type = "tool";
  ui->nick = nick;
  ui->name = name;
  ui->description = description;
  ui->logo = logo;

  /* Select from *version*, *manufacturer*, *copyright*, *license*, *url*,
   * *support*, *download*, *sources*, *openicc_modules_author* and
   * *documentation* what you see fit. Add new ones as needed. */
  ui->sections = info;
  ui->opts->array = options;
  ui->opts->groups = groups;

  /* get results and check syntax ... */
  opt_state = openiccOptions_Parse( ui->opts );
  /* ... and report detected errors */
  if(opt_state != openiccOPTION_NONE)
  {
    fputs( _("... try with --help|-h option for usage text. give up"), stderr );
    fputs( "\n", stderr );
    openiccUi_Release( &ui);
    if(status)
      *status = opt_state << openiccUI_STATE_OPTION;
    return NULL;
  }

  X = openiccOptions_GetOption( ui->opts, 'X' );
  if(X && X->variable_type == openiccSTRING && X->variable.s)
    export = *X->variable.s;
  h = openiccOptions_GetOption( ui->opts, 'h' );
  if(h && h->variable_type == openiccINT && h->variable.i)
    help = *h->variable.i;
  v = openiccOptions_GetOption( ui->opts, 'v' );
  if(v && v->variable_type == openiccINT && v->variable.i)
  {
    verbose = *v->variable.i;
    if(status && verbose)
      *status |= openiccUI_STATE_VERBOSE;
  }
  if(help)
  {
    openiccUiHeaderSection_s * version = openiccUi_GetHeaderSection( ui,
                                                               "version" );
    openiccOptions_PrintHelp( ui->opts, ui, verbose, "%s v%s - %s", argv[0],
                              version && version->name ? version->name : "",
                              ui->description ? ui->description : "" );
    openiccUi_Release( &ui);
    if(status)
      *status |= openiccUI_STATE_HELP;
    return NULL;
  }
  if(export)
  {
    if(status)
      *status |= openiccUI_STATE_EXPORT;
    if(strcmp(export, "json") == 0)
    {
      puts( openiccUi_ToJson( ui, 0 ) );
      openiccUi_Release( &ui);
      return NULL;
    }
    if(strcmp(export, "man") == 0)
    {
      puts( openiccUi_ToMan( ui, 0 ) );
      openiccUi_Release( &ui);
      return NULL;
    }
    if(strcmp(export, "markdown") == 0)
    {
      puts( openiccUi_ToMarkdown( ui, 0 ) );
      openiccUi_Release( &ui);
      return NULL;
    }
  }
  /* done with options handling */

  return ui;
}

/** @brief    Release "oiui"
 *  @memberof openiccUi_s
 *
 *  Release openiccUi_s::opts, openiccUi_s::private_data and openiccUi_s.
 *
 *  @version OpenICC: 0.1.1
 *  @date    2018/08/14
 *  @since   2018/08/14 (OpenICC: 0.1.1)
 */
void           openiccUi_Release     ( openiccUi_s      ** ui )
{
  char ** list;
  int pos = 0;
  if(!ui || !*ui) return;
  if(openiccObjectToType( *ui ) != openiccOBJECT_UI)
  {
    char * a = (char*)*ui;
    char type[5] = {a[0],a[1],a[2],a[3],0};
    fprintf(stderr, "Unexpected object: \"%s\"(expected: \"%s\")\n", type, 
            openiccObjectTypeToString( openiccOBJECT_UI ) );
    return;
  }
  list = (*ui)->opts->private_data;
  while( list && list[pos] )
    free(list[pos++]);
  if((*ui)->opts->private_data) free((*ui)->opts->private_data);
  if((*ui)->opts) free((*ui)->opts);
  free((*ui));
  *ui = NULL;
}

/** @brief    Return the number of sections of type "oihs"
 *  @memberof openiccUi_s
 *
 *  The openiccUi_s contains already options in the opts member.
 *
 *  @version OpenICC: 0.1.1
 *  @date    2018/08/14
 *  @since   2018/08/14 (OpenICC: 0.1.1)
 */
int     openiccUi_CountHeaderSections( openiccUi_s       * ui )
{
  int n = 0;
  while(openiccObjectToType( &ui->sections[n] ) /*"oihs"*/ == openiccOBJECT_UI_HEADER_SECTION) ++n;
  return n;
}

/** @brief    Return the section which was specified by its nick name
 *  @memberof openiccUi_s
 *
 *  @version OpenICC: 0.1.1
 *  @date    2018/08/14
 *  @since   2018/08/14 (OpenICC: 0.1.1)
 */
openiccUiHeaderSection_s * openiccUi_GetHeaderSection (
                                       openiccUi_s       * ui,
                                       const char        * nick )
{
  openiccUiHeaderSection_s * section = NULL;
  int i, count = openiccUi_CountHeaderSections(ui);
  for(i = 0; i < count; ++i)
    if( strcmp(ui->sections[i].nick, nick) == 0 )
      section = &ui->sections[i];
  return section;
}

/** @brief    Return a JSON representation from options
 *  @memberof openiccUi_s
 *
 *  The JSON data shall be useable with openicc-json-qml options renderer.
 *
 *  @version OpenICC: 0.1.1
 *  @date    2018/08/14
 *  @since   2018/08/14 (OpenICC: 0.1.1)
 */
char *       openiccUi_ToJson        ( openiccUi_s       * ui,
                                       int                 flags OYJL_UNUSED )
{
  char * t = NULL, num[64];
  oyjl_val root, key;
  int i,n,ng;

  root = oyjlTreeNew( "" );
  oyjlTreeSetValueString( root, "org/freedesktop/openicc/modules/[0]/openicc_module_api_version", "1" );
  if(ui->app_type && ui->app_type[0])
  {
    oyjlTreeSetValueString( root, "org/freedesktop/openicc/modules/[0]/type", ui->app_type );
    if(strcmp( ui->app_type, "tool" ) == 0)
      oyjlTreeSetValueString( root, "org/freedesktop/openicc/modules/[0]/label", _("Tool") );
    else if(strcmp( ui->app_type, "module" ) == 0)
      oyjlTreeSetValueString( root, "org/freedesktop/openicc/modules/[0]/label", _("Module") );
  }
  oyjlTreeSetValueString( root, "org/freedesktop/openicc/modules/[0]/nick", ui->nick );
  oyjlTreeSetValueString( root, "org/freedesktop/openicc/modules/[0]/name", ui->name );
  oyjlTreeSetValueString( root, "org/freedesktop/openicc/modules/[0]/description", ui->description );
  if(ui->logo)
    oyjlTreeSetValueString( root, "org/freedesktop/openicc/modules/[0]/logo", ui->logo );

  n = openiccUi_CountHeaderSections( ui );
  for(i = 0; i < n; ++i)
  {
    openiccUiHeaderSection_s * s = &ui->sections[i];
    key = oyjlTreeGetValuef( root, OYJL_CREATE_NEW, "org/freedesktop/openicc/modules/[0]/information/[%d]/%s", i, "type" );
    oyjlValueSetString( key, s->nick );
    key = oyjlTreeGetValuef( root, OYJL_CREATE_NEW, "org/freedesktop/openicc/modules/[0]/information/[%d]/%s", i, "label" );
    if(s->label)
      oyjlValueSetString( key, s->label );
    else
    {
      if(strcmp(s->nick, "manufacturer") == 0) oyjlValueSetString( key, _("Manufacturer") );
      else if(strcmp(s->nick, "copyright") == 0) oyjlValueSetString( key, _("Copyright") );
      else if(strcmp(s->nick, "license") == 0) oyjlValueSetString( key, _("License") );
      else if(strcmp(s->nick, "url") == 0) oyjlValueSetString( key, _("Url") );
      else if(strcmp(s->nick, "support") == 0) oyjlValueSetString( key, _("Support") );
      else if(strcmp(s->nick, "download") == 0) oyjlValueSetString( key, _("Download") );
      else if(strcmp(s->nick, "sources") == 0) oyjlValueSetString( key, _("Sources") );
      else if(strcmp(s->nick, "development") == 0) oyjlValueSetString( key, _("Development") );
      else if(strcmp(s->nick, "openicc_module_author") == 0) oyjlValueSetString( key, _("OpenICC Module Author") );
      else if(strcmp(s->nick, "documentation") == 0) oyjlValueSetString( key, _("Documentation") );
      else if(strcmp(s->nick, "date") == 0) oyjlValueSetString( key, _("Date") );
      else if(strcmp(s->nick, "version") == 0) oyjlValueSetString( key, _("Version") );
      else oyjlValueSetString( key, _(s->nick) );
    }
    key = oyjlTreeGetValuef( root, OYJL_CREATE_NEW, "org/freedesktop/openicc/modules/[0]/information/[%d]/%s", i, "name" );
    oyjlValueSetString( key, s->name );
    if(s->description)
    {
      key = oyjlTreeGetValuef( root, OYJL_CREATE_NEW, "org/freedesktop/openicc/modules/[0]/information/[%d]/%s", i, "description" );
      oyjlValueSetString( key, s->description );
    }
  }

  ng = openiccOptions_CountGroups( ui->opts );
  for(i = 0; i < ng; ++i)
  {
    openiccOptionGroup_s * g = &ui->opts->groups[i];
    openiccOptions_s * opts = ui->opts;

    if(!(g->mandatory && g->mandatory[0]))
      continue;

    key = oyjlTreeGetValuef( root, OYJL_CREATE_NEW, "org/freedesktop/openicc/modules/[0]/groups/[%d]/%s", i, "name" );
    oyjlValueSetString( key, g->name );
    key = oyjlTreeGetValuef( root, OYJL_CREATE_NEW, "org/freedesktop/openicc/modules/[0]/groups/[%d]/%s", i, "description" );
    oyjlValueSetString( key, g->description );
    if(g->help)
    {
      key = oyjlTreeGetValuef( root, OYJL_CREATE_NEW, "org/freedesktop/openicc/modules/[0]/groups/[%d]/%s", i, "help" );
      oyjlValueSetString( key, g->help );
        fprintf(stderr, "found help: %s\n", g->help);
    }

    key = oyjlTreeGetValuef( root, OYJL_CREATE_NEW, "org/freedesktop/openicc/modules/[0]/groups/[%d]/%s", i, "mandatory" );
    oyjlValueSetString( key, g->mandatory );
    int d = g->detail ? strlen(g->detail) : 0,
        j;
    for(j = 0; j < d; ++j)
    {
      char oc = g->detail[j];
      openiccOption_s * o = openiccOptions_GetOption( opts, oc );
      key = oyjlTreeGetValuef( root, OYJL_CREATE_NEW, "org/freedesktop/openicc/modules/[0]/groups/[%d]/options/[%d]/%s", i,j, "key" );
      if(!o->key)
        sprintf(num, "%c", o->o);
      oyjlValueSetString( key, o->key?o->key:num );
      key = oyjlTreeGetValuef( root, OYJL_CREATE_NEW, "org/freedesktop/openicc/modules/[0]/groups/[%d]/options/[%d]/%s", i,j, "name" );
      oyjlValueSetString( key, o->name );
      key = oyjlTreeGetValuef( root, OYJL_CREATE_NEW, "org/freedesktop/openicc/modules/[0]/groups/[%d]/options/[%d]/%s", i,j, "description" );
      oyjlValueSetString( key, o->description );
      if(o->help)
      {
        key = oyjlTreeGetValuef( root, OYJL_CREATE_NEW, "org/freedesktop/openicc/modules/[0]/groups/[%d]/options/[%d]/%s", i,j, "help" );
        oyjlValueSetString( key, o->help );
      }

      switch(o->value_type)
      {
        case openiccOPTIONTYPE_CHOICE:
          {
            int n = 0,l;
            key = oyjlTreeGetValuef( root, OYJL_CREATE_NEW, "org/freedesktop/openicc/modules/[0]/groups/[%d]/options/[%d]/%s", i,j, "default" );
            sprintf( num, "%d", o->values.choices.selected );
            oyjlValueSetString( key, num );
            while(o->values.choices.list[n].nick[0] != '\000')
              ++n;
            for(l = 0; l < n; ++l)
            {
              key = oyjlTreeGetValuef( root, OYJL_CREATE_NEW, "org/freedesktop/openicc/modules/[0]/groups/[%d]/options/[%d]/choices/[%d]/%s", i,j,l, "nick" );
              oyjlValueSetString( key, o->values.choices.list[l].nick );
              key = oyjlTreeGetValuef( root, OYJL_CREATE_NEW, "org/freedesktop/openicc/modules/[0]/groups/[%d]/options/[%d]/choices/[%d]/%s", i,j,l, "name" );
              oyjlValueSetString( key, o->values.choices.list[l].name );
            }
          }
          key = oyjlTreeGetValuef( root, OYJL_CREATE_NEW, "org/freedesktop/openicc/modules/[0]/groups/[%d]/options/[%d]/%s", i,j, "type" );
          oyjlValueSetString( key, "choice" );
          break;
        case openiccOPTIONTYPE_FUNCTION:
          {
            int n = 0,l, selected;
            openiccOptionChoice_s * list = openiccOption_GetChoices_(o, &selected, opts );
            if(list)
              while(list[n].nick[0] != '\000')
                ++n;
            if(0 <= selected && selected < n && strlen(list[selected].nick))
            {
              key = oyjlTreeGetValuef( root, OYJL_CREATE_NEW, "org/freedesktop/openicc/modules/[0]/groups/[%d]/options/[%d]/%s", i,j, "default" );
              oyjlValueSetString( key, list[selected].nick );
            }
            for(l = 0; l < n; ++l)
            {
              key = oyjlTreeGetValuef( root, OYJL_CREATE_NEW, "org/freedesktop/openicc/modules/[0]/groups/[%d]/options/[%d]/choices/[%d]/%s", i,j,l, "nick" );
              oyjlValueSetString( key, list[l].nick );
              key = oyjlTreeGetValuef( root, OYJL_CREATE_NEW, "org/freedesktop/openicc/modules/[0]/groups/[%d]/options/[%d]/choices/[%d]/%s", i,j,l, "name" );
              oyjlValueSetString( key, list[l].name );
            }
            /* not possible, as the result of openiccOption_GetChoices_() is cached - openiccOptionChoice_Release( &list ); */
          }
          key = oyjlTreeGetValuef( root, OYJL_CREATE_NEW, "org/freedesktop/openicc/modules/[0]/groups/[%d]/options/[%d]/%s", i,j, "type" );
          oyjlValueSetString( key, "choice" );
          break;
        case openiccOPTIONTYPE_DOUBLE:
          key = oyjlTreeGetValuef( root, OYJL_CREATE_NEW, "org/freedesktop/openicc/modules/[0]/groups/[%d]/options/[%d]/%s", i,j, "default" );
          sprintf( num, "%g", o->values.dbl.d ); oyjlValueSetString( key, num );
          key = oyjlTreeGetValuef( root, OYJL_CREATE_NEW, "org/freedesktop/openicc/modules/[0]/groups/[%d]/options/[%d]/%s", i,j, "start" );
          sprintf( num, "%g", o->values.dbl.start ); oyjlValueSetString( key, num );
          key = oyjlTreeGetValuef( root, OYJL_CREATE_NEW, "org/freedesktop/openicc/modules/[0]/groups/[%d]/options/[%d]/%s", i,j, "end" );
          sprintf( num, "%g", o->values.dbl.end ); oyjlValueSetString( key, num );
          key = oyjlTreeGetValuef( root, OYJL_CREATE_NEW, "org/freedesktop/openicc/modules/[0]/groups/[%d]/options/[%d]/%s", i,j, "tick" );
          sprintf( num, "%g", o->values.dbl.tick ); oyjlValueSetString( key, num );
          key = oyjlTreeGetValuef( root, OYJL_CREATE_NEW, "org/freedesktop/openicc/modules/[0]/groups/[%d]/options/[%d]/%s", i,j, "type" );
          oyjlValueSetString( key, "double" );
          break;
        case openiccOPTIONTYPE_NONE:
          key = oyjlTreeGetValuef( root, OYJL_CREATE_NEW, "org/freedesktop/openicc/modules/[0]/groups/[%d]/options/[%d]/%s", i,j, "default" );
          oyjlValueSetString( key, "0" );
          key = oyjlTreeGetValuef( root, OYJL_CREATE_NEW, "org/freedesktop/openicc/modules/[0]/groups/[%d]/options/[%d]/%s", i,j, "type" );
          oyjlValueSetString( key, "bool" );
          {
            int l; char t[12];
            for(l = 0; l < 2; ++l)
            {
              sprintf(t, "%d", l);
              key = oyjlTreeGetValuef( root, OYJL_CREATE_NEW, "org/freedesktop/openicc/modules/[0]/groups/[%d]/options/[%d]/choices/[%d]/%s", i,j,l, "nick" );
              oyjlValueSetString( key, t );
              key = oyjlTreeGetValuef( root, OYJL_CREATE_NEW, "org/freedesktop/openicc/modules/[0]/groups/[%d]/options/[%d]/choices/[%d]/%s", i,j,l, "name" );
              oyjlValueSetString( key, l?_("Yes"):_("No") );
            }
          }
          break;
        case openiccOPTIONTYPE_STRING:
          if(o->values.suggest)
          {
            key = oyjlTreeGetValuef( root, OYJL_CREATE_NEW, "org/freedesktop/openicc/modules/[0]/groups/[%d]/options/[%d]/%s", i,j, "suggest" );
            oyjlValueSetString( key, o->values.suggest );
          }
          key = oyjlTreeGetValuef( root, OYJL_CREATE_NEW, "org/freedesktop/openicc/modules/[0]/groups/[%d]/options/[%d]/%s", i,j, "type" );
          oyjlValueSetString( key, "string" );
          break;
        case openiccOPTIONTYPE_START: break;
        case openiccOPTIONTYPE_END: break;
      }
    }
  }
  i = 0;
  oyjlTreeToJson( root, &i, &t );

  return t;
}

char *       openiccStringToUpper    ( const char        * t )
{
  char * text = oyjlStringCopy(t, malloc);
  int slen = strlen(t), i;
  for(i = 0; i < slen; ++i)
    text[i] = toupper(t[i]);
  return text;
}

static
char *       openiccExtraManSection  ( openiccOptions_s  * opts,
                                       const char        * opt_name,
                                       int                 flags )
{
  openiccOption_s * o = openiccOptions_GetOptionL( opts, opt_name );
  char * text = NULL;
  if(o)
  {
    int n = 0,l;
    if(o->value_type == openiccOPTIONTYPE_CHOICE)
    {
      openiccOptionChoice_s * list = o->values.choices.list;
      while(o->values.choices.list[n].nick[0] != '\000') ++n;
      if(n)
      {
        char * up = openiccStringToUpper( &opt_name[4] );
        char * sect = oyjlStringReplace( up, "_", " ", malloc, free );
        const char * section = sect;
        if(strcmp(section,"EXAMPLES") == 0)
          section = _("EXAMPLES");
        else if(strcmp(section,"EXIT-STATE") == 0)
          section = _("EXIT-STATE");
        else if(strcmp(section,"ENVIRONMENT VARIABLES") == 0)
          section = _("ENVIRONMENT VARIABLES");
        else if(strcmp(section,"HISTORY") == 0)
          section = _("HISTORY");
        else if(strcmp(section,"FILES") == 0)
          section = _("FILES");
        else if(strcmp(section,"SEE AS WELL") == 0)
          section = _("SEE AS WELL");
        if(flags & openiccOPTIONSTYLE_MARKDOWN)
          oyjlStringAdd( &text, malloc, free, "# %s\n", _(section) );
        else
          oyjlStringAdd( &text, malloc, free, ".SH %s\n", _(section) );
        for(l = 0; l < n; ++l)
          if(flags & openiccOPTIONSTYLE_MARKDOWN)
            oyjlStringAdd( &text, malloc, free, "## %s\n%s %s %s\n", list[l].nick, list[l].name, list[l].description, list[l].help );
          else
            oyjlStringAdd( &text, malloc, free, ".TP\n%s\n.br\n%s %s %s\n", list[l].nick, list[l].name, list[l].description, list[l].help );
        free(sect);
        free(up);
      }
    }
  }
  return text;
}

static
char *       openiccExtraManSections ( openiccOptions_s  * opts, int flags )
{
  char * text = NULL;
  int nopts = openiccOptions_Count( opts );
  int l;
  for(l = 0; l < nopts; ++l)
  {
    openiccOption_s * o = &opts->array[l];
    const char * option = o->option;
    int olen = option ? strlen(option) : 0;
    if(olen > 7 && option[0] == 'm' && option[1] == 'a' && option[2] == 'n' && option[3] == '-')
    {
      char * tmp = openiccExtraManSection(opts, option, flags);
      if(tmp)
      {
        oyjlStringAdd( &text, malloc, free, tmp );
        free(tmp);
      }
    }
  }
  return text;
}

/** @brief    Return a man page from options
 *  @memberof openiccUi_s
 *
 *  Some man pages might contain some additional sections.
 *  They are supported as options. To generate a custom man page section,
 *  add a blind option to your options list and set the openiccOption_s::o
 *  char to something non interupting like, dot '.' or similar.
 *  The openiccOption_s::option string
 *  contains "man-section_head", with "section-head" being adapted to your
 *  needs. The "man-" part will be cut off and 
 *  "section_head" will become uppercase and underline '_' become empty
 *  space: "SECTION HEAD".
 *  Use openiccOption_s::value_type=openiccOPTIONTYPE_CHOICE
 *  and place your string list into openiccOptionChoice_s::nick by filling all
 *  members. Translated section heads are "EXAMPLES, "SEE AS WELL", "HISTORY",
 *  "ENVIRONMENT VARIABLES", "EXIT-STATE" and "FILES".
 *
 *  @version OpenICC: 0.1.1
 *  @date    2018/10/15
 *  @since   2018/10/10 (OpenICC: 0.1.1)
 */
char *       openiccUi_ToMan         ( openiccUi_s       * ui,
                                       int                 flags OYJL_UNUSED )
{
  char * text = NULL, * tmp;
  const char * date = NULL,
             * desc = NULL,
             * mnft = NULL, * mnft_url = NULL,
             * copy = NULL, * lice = NULL,
             * bugs = NULL, * bugs_url = NULL,
             * vers = NULL;
  int i,n,ng;
  openiccOptions_s * opts = ui->opts;

  n = openiccUi_CountHeaderSections( ui );
  for(i = 0; i < n; ++i)
  {
    openiccUiHeaderSection_s * s = &ui->sections[i];
    if(strcmp(s->nick, "manufacturer") == 0) { mnft = s->name; mnft_url = s->description; }
    else if(strcmp(s->nick, "copyright") == 0) copy = s->name;
    else if(strcmp(s->nick, "license") == 0) lice = s->name;
    else if(strcmp(s->nick, "url") == 0) continue;
    else if(strcmp(s->nick, "support") == 0) { bugs = s->name; bugs_url = s->description; }
    else if(strcmp(s->nick, "download") == 0) continue;
    else if(strcmp(s->nick, "sources") == 0) continue;
    else if(strcmp(s->nick, "development") == 0) continue;
    else if(strcmp(s->nick, "openicc_module_author") == 0) continue;
    else if(strcmp(s->nick, "documentation") == 0) desc = s->description ? s->description : s->name;
    else if(strcmp(s->nick, "version") == 0) vers = s->name;
    else if(strcmp(s->nick, "date") == 0) date = s->description ? s->description : s->name;
  }

  ng = openiccOptions_CountGroups(opts);
  if(!ng) return NULL;

  if(ui->app_type && ui->app_type[0])
  {
    int tool = strcmp( ui->app_type, "tool" ) == 0;
    oyjlStringAdd( &text, malloc, free, ".TH \"%s\" %d \"%s\" \"%s\"\n", ui->nick,
                   tool?1:7, date?date:"", tool?"User Commands":"Misc" );
  }

  if( ui )
    oyjlStringAdd( &text, malloc, free, ".SH NAME\n%s %s%s \\- %s\n", ui->nick, vers?"v":"", vers?vers:"", ui->name );

  oyjlStringAdd( &text, malloc, free, ".SH %s\n", _("SYNOPSIS") );
  for(i = 0; i < ng; ++i)
  {
    openiccOptionGroup_s * g = &opts->groups[i];
    const char * syn = openiccOptions_PrintHelpSynopsis( opts, g,
                         openiccOPTIONSTYLE_ONELETTER | openiccOPTIONSTYLE_MAN );
    if(syn[0])
      oyjlStringAdd( &text, malloc, free, "%s\n%s", syn, (i < (ng-1)) ? ".br\n" : "" );
  }

  if(desc)
    oyjlStringAdd( &text, malloc, free, ".SH %s\n%s\n", _("DESCRIPTION"), desc );

  oyjlStringAdd( &text, malloc, free, ".SH %s\n", _("OPTIONS") );
  for(i = 0; i < ng; ++i)
  {
    openiccOptionGroup_s * g = &opts->groups[i];
    int d = g->detail ? strlen(g->detail) : 0,
        j;
    oyjlStringAdd( &text, malloc, free, ".TP\n%s\n", g->description  );
    if(g->mandatory && g->mandatory[0])
    {
      oyjlStringAdd( &text, malloc, free, "%s\n", openiccOptions_PrintHelpSynopsis( opts, g, openiccOPTIONSTYLE_ONELETTER | openiccOPTIONSTYLE_MAN ) );
    }
    oyjlStringAdd( &text, malloc, free, ".br\n"  );
    if(g->help)
    {
      oyjlStringAdd( &text, malloc, free, "%s\n.br\n.sp\n.br\n", g->help );
    }
    for(j = 0; j < d; ++j)
    {
      char oc = g->detail[j];
      openiccOption_s * o = openiccOptions_GetOption( opts, oc );
      if(!o)
      {
        fprintf(stderr, "\n%s: option not declared: %c\n", g->name, oc);
        exit(1);
      }
      switch(o->value_type)
      {
        case openiccOPTIONTYPE_CHOICE:
          {
            int n = 0,l;
            oyjlStringAdd( &text, malloc, free, "%s", openiccOption_PrintArg(o, openiccOPTIONSTYLE_ONELETTER | openiccOPTIONSTYLE_STRING | openiccOPTIONSTYLE_MAN) );
            oyjlStringAdd( &text, malloc, free, "\t%s%s%s\n.br\n", o->description ? o->description:"", o->help?": ":"", o->help?o->help :"" );
            while(o->values.choices.list[n].nick[0] != '\000')
              ++n;
            for(l = 0; l < n; ++l)
              oyjlStringAdd( &text, malloc, free, "\t\\-%c %s\t\t# %s %s %s\n.br\n",
                  o->o,
                  o->values.choices.list[l].nick,
                  o->values.choices.list[l].name && o->values.choices.list[l].nick[0] ? o->values.choices.list[l].name : o->values.choices.list[l].description,
                  o->values.choices.list[l].help?" - ":"",
                  o->values.choices.list[l].help?o->values.choices.list[l].help:"" );
          }
          break;
        case openiccOPTIONTYPE_FUNCTION:
          {
            int n = 0,l;
            openiccOptionChoice_s * list = openiccOption_GetChoices_(o, NULL, opts );
            oyjlStringAdd( &text, malloc, free, "%s", openiccOption_PrintArg(o, openiccOPTIONSTYLE_ONELETTER | openiccOPTIONSTYLE_STRING | openiccOPTIONSTYLE_MAN) );
            oyjlStringAdd( &text, malloc, free, "\t%s%s%s\n.br\n", o->description ? o->description:"", o->help?": ":"", o->help?o->help :"" );
            if(list)
              while(list[n].nick[0] != '\000')
                ++n;
            for(l = 0; l < n; ++l)
              oyjlStringAdd( &text, malloc, free, "\t\\-%c %s\t\t# %s\n.br\n", o->o, list[l].nick, list[l].name && list[l].nick[0] ? list[l].name : list[l].description );
            /* not possible, as the result of openiccOption_GetChoices_() is cached - openiccOptionChoice_Release( &list ); */
          }
          break;
        case openiccOPTIONTYPE_DOUBLE:
          oyjlStringAdd( &text, malloc, free, "%s", openiccOption_PrintArg(o, openiccOPTIONSTYLE_ONELETTER | openiccOPTIONSTYLE_STRING | openiccOPTIONSTYLE_MAN) );
          oyjlStringAdd( &text, malloc, free, "\t%s%s%s\n.br\n", o->description ? o->description:"", o->help?": ":"", o->help?o->help :"" );
          break;
        case openiccOPTIONTYPE_STRING:
          oyjlStringAdd( &text, malloc, free, "%s", openiccOption_PrintArg(o, openiccOPTIONSTYLE_ONELETTER | openiccOPTIONSTYLE_STRING | openiccOPTIONSTYLE_MAN) );
          oyjlStringAdd( &text, malloc, free, "\t%s%s%s\n.br\n", o->description ? o->description:"", o->help?": ":"", o->help?o->help :"" );
          break;
        case openiccOPTIONTYPE_NONE:
          oyjlStringAdd( &text, malloc, free, "%s", openiccOption_PrintArg(o, openiccOPTIONSTYLE_ONELETTER | openiccOPTIONSTYLE_STRING | openiccOPTIONSTYLE_MAN) );
          oyjlStringAdd( &text, malloc, free, "\t%s%s%s\n.br\n", o->description ? o->description:"", o->help?": ":"", o->help?o->help :"" );
        break;
        case openiccOPTIONTYPE_START: break;
        case openiccOPTIONTYPE_END: break;
      }
    }
  }

  tmp = openiccExtraManSections( opts, openiccOPTIONSTYLE_MAN );
  if(tmp)
  {
    oyjlStringAdd( &text, malloc, free, "%s", tmp );
    free(tmp);
  }

  if(mnft)
    oyjlStringAdd( &text, malloc, free, ".SH %s\n%s %s\n", _("AUTHOR"), mnft, mnft_url?mnft_url:"" );

  if(lice || copy)
  {
    oyjlStringAdd( &text, malloc, free, ".SH %s\n%s\n", _("COPYRIGHT"), copy?copy:"" );
    if(lice)
      oyjlStringAdd( &text, malloc, free, ".br\n%s: %s\n", _("License"), lice?lice:"" );
  }

  if(bugs)
    oyjlStringAdd( &text, malloc, free, ".SH %s\n%s %s\n", _("BUGS"), bugs, bugs_url?bugs_url:"" );

  return text;
}

/** @brief    Return markdown formated text from options
 *  @memberof openiccUi_s
 *
 *  @see openiccUi_ToMan()
 *
 *  @version OpenICC: 0.1.1
 *  @date    2018/11/07
 *  @since   2018/11/07 (OpenICC: 0.1.1)
 */
char *       openiccUi_ToMarkdown    ( openiccUi_s       * ui,
                                       int                 flags OYJL_UNUSED )
{
  char * text = NULL, * tmp, * doxy_link = NULL;
  const char * date = NULL,
             * desc = NULL,
             * mnft = NULL, * mnft_url = NULL,
             * copy = NULL, * lice = NULL,
             * bugs = NULL, * bugs_url = NULL,
             * vers = NULL,
             * country = NULL;
  int i,n,ng;
  openiccOptions_s * opts = ui->opts;

  n = openiccUi_CountHeaderSections( ui );
  for(i = 0; i < n; ++i)
  {
    openiccUiHeaderSection_s * s = &ui->sections[i];
    if(strcmp(s->nick, "manufacturer") == 0) { mnft = s->name; mnft_url = s->description; }
    else if(strcmp(s->nick, "copyright") == 0) copy = s->name;
    else if(strcmp(s->nick, "license") == 0) lice = s->name;
    else if(strcmp(s->nick, "url") == 0) continue;
    else if(strcmp(s->nick, "support") == 0) { bugs = s->name; bugs_url = s->description; }
    else if(strcmp(s->nick, "download") == 0) continue;
    else if(strcmp(s->nick, "sources") == 0) continue;
    else if(strcmp(s->nick, "development") == 0) continue;
    else if(strcmp(s->nick, "openicc_module_author") == 0) continue;
    else if(strcmp(s->nick, "documentation") == 0) desc = s->description ? s->description : s->name;
    else if(strcmp(s->nick, "version") == 0) vers = s->name;
    else if(strcmp(s->nick, "date") == 0) date = s->description ? s->description : s->name;
  }

  ng = openiccOptions_CountGroups(opts);
  if(!ng) return NULL;

#ifdef HAVE_LANGINFO_H
  country = nl_langinfo( _NL_ADDRESS_LANG_AB );
#endif

  oyjlStringAdd( &doxy_link, malloc, free, "{#%s%s}", ui->nick, country?country:"" );
  tmp = oyjlStringReplace( doxy_link, "-", "", malloc, free );
  free(doxy_link); doxy_link = tmp; tmp = NULL;

  oyjlStringAdd( &text, malloc, free, "# %s %s%s %s\n", ui->nick, vers?"v":"", vers?vers:"", doxy_link );

  if(ui->app_type && ui->app_type[0])
  {
    int tool = strcmp( ui->app_type, "tool" ) == 0;
    oyjlStringAdd( &text, malloc, free, "*\"%s\"* *%d* *\"%s\"* \"%s\"\n", ui->nick,
                   tool?1:7, date?date:"", tool?"User Commands":"Misc" );
  }

  if( ui )
    oyjlStringAdd( &text, malloc, free, "## NAME\n%s %s%s - %s\n", ui->nick, vers?"v":"", vers?vers:"", ui->name );

  oyjlStringAdd( &text, malloc, free, "## %s\n", _("SYNOPSIS") );
  for(i = 0; i < ng; ++i)
  {
    openiccOptionGroup_s * g = &opts->groups[i];
    const char * syn = openiccOptions_PrintHelpSynopsis( opts, g,
                         openiccOPTIONSTYLE_ONELETTER | openiccOPTIONSTYLE_MARKDOWN );
    if(syn[0])
      oyjlStringAdd( &text, malloc, free, "%s\n%s", syn, (i < (ng-1)) ? "<br />\n" : "" );
  }

  if(desc)
    oyjlStringAdd( &text, malloc, free, "## %s\n%s\n", _("DESCRIPTION"), desc );

  oyjlStringAdd( &text, malloc, free, "## %s\n", _("OPTIONS") );
  for(i = 0; i < ng; ++i)
  {
    openiccOptionGroup_s * g = &opts->groups[i];
    int d = g->detail ? strlen(g->detail) : 0,
        j;
    oyjlStringAdd( &text, malloc, free, "### %s\n", g->description  );
    if(g->mandatory && g->mandatory[0])
    {
      oyjlStringAdd( &text, malloc, free, "%s\n", openiccOptions_PrintHelpSynopsis( opts, g, openiccOPTIONSTYLE_ONELETTER | openiccOPTIONSTYLE_MARKDOWN ) );
    }
    oyjlStringAdd( &text, malloc, free, "\n"  );
    if(g->help)
    {
      oyjlStringAdd( &text, malloc, free, "%s\n\n", g->help );
    }
    for(j = 0; j < d; ++j)
    {
      char oc = g->detail[j];
      openiccOption_s * o = openiccOptions_GetOption( opts, oc );
      if(!o)
      {
        fprintf(stderr, "\n%s: option not declared: %c\n", g->name, oc);
        exit(1);
      }
      switch(o->value_type)
      {
        case openiccOPTIONTYPE_CHOICE:
          {
            int n = 0,l;
            oyjlStringAdd( &text, malloc, free, "* %s", openiccOption_PrintArg(o, openiccOPTIONSTYLE_ONELETTER | openiccOPTIONSTYLE_STRING | openiccOPTIONSTYLE_MARKDOWN) );
            oyjlStringAdd( &text, malloc, free, "\t%s%s%s\n", o->description ? o->description:"", o->help?": ":"", o->help?o->help :"" );
            while(o->values.choices.list[n].nick[0] != '\000')
              ++n;
            for(l = 0; l < n; ++l)
              oyjlStringAdd( &text, malloc, free, "   * -%c %s\t\t# %s\n", o->o, o->values.choices.list[l].nick, o->values.choices.list[l].name && o->values.choices.list[l].nick[0] ? o->values.choices.list[l].name : o->values.choices.list[l].description );
          }
          break;
        case openiccOPTIONTYPE_FUNCTION:
          {
            int n = 0,l;
            openiccOptionChoice_s * list = openiccOption_GetChoices_(o, NULL, opts );
            oyjlStringAdd( &text, malloc, free, "* %s", openiccOption_PrintArg(o, openiccOPTIONSTYLE_ONELETTER | openiccOPTIONSTYLE_STRING | openiccOPTIONSTYLE_MARKDOWN) );
            oyjlStringAdd( &text, malloc, free, "\t%s%s%s\n", o->description ? o->description:"", o->help?": ":"", o->help?o->help :"" );
            if(list)
              while(list[n].nick[0] != '\000')
                ++n;
            for(l = 0; l < n; ++l)
              oyjlStringAdd( &text, malloc, free, "   * -%c %s\t\t# %s\n", o->o, list[l].nick, list[l].name && list[l].nick[0] ? list[l].name : list[l].description );
            /* not possible, as the result of openiccOption_GetChoices_() is cached - openiccOptionChoice_Release( &list ); */
          }
          break;
        case openiccOPTIONTYPE_DOUBLE:
          oyjlStringAdd( &text, malloc, free, "* %s", openiccOption_PrintArg(o, openiccOPTIONSTYLE_ONELETTER | openiccOPTIONSTYLE_STRING | openiccOPTIONSTYLE_MARKDOWN) );
          oyjlStringAdd( &text, malloc, free, "\t%s%s%s\n", o->description ? o->description:"", o->help?": ":"", o->help?o->help :"" );
          break;
        case openiccOPTIONTYPE_STRING:
          oyjlStringAdd( &text, malloc, free, "* %s", openiccOption_PrintArg(o, openiccOPTIONSTYLE_ONELETTER | openiccOPTIONSTYLE_STRING | openiccOPTIONSTYLE_MARKDOWN) );
          oyjlStringAdd( &text, malloc, free, "\t%s%s%s\n", o->description ? o->description:"", o->help?": ":"", o->help?o->help :"" );
          break;
        case openiccOPTIONTYPE_NONE:
          oyjlStringAdd( &text, malloc, free, "* %s", openiccOption_PrintArg(o, openiccOPTIONSTYLE_ONELETTER | openiccOPTIONSTYLE_STRING | openiccOPTIONSTYLE_MARKDOWN) );
          oyjlStringAdd( &text, malloc, free, "\t%s%s%s\n", o->description ? o->description:"", o->help?": ":"", o->help?o->help :"" );
        break;
        case openiccOPTIONTYPE_START: break;
        case openiccOPTIONTYPE_END: break;
      }
    }
    oyjlStringAdd( &text, malloc, free, "\n"  );
  }

  tmp = openiccExtraManSections( opts, openiccOPTIONSTYLE_MARKDOWN );
  if(tmp)
  {
    oyjlStringAdd( &text, malloc, free, "%s", tmp );
    free(tmp);
  }

  if(mnft)
    oyjlStringAdd( &text, malloc, free, "## %s\n%s %s\n", _("AUTHOR"), mnft, mnft_url?mnft_url:"" );

  if(lice || copy)
  {
    oyjlStringAdd( &text, malloc, free, "## %s\n*%s*\n", _("COPYRIGHT"), copy?copy:"" );
    if(lice)
      oyjlStringAdd( &text, malloc, free, "\n\n### %s\n%s\n", _("License"), lice?lice:"" );
  }

  if(bugs && bugs_url)
    oyjlStringAdd( &text, malloc, free, "## %s\n[%s](%s)\n", _("BUGS"), bugs, bugs_url?bugs_url:"" );
  else if(bugs)
    oyjlStringAdd( &text, malloc, free, "## %s\n[%s](%s)\n", _("BUGS"), bugs, bugs );

  {
    char * tmp = oyjlStringReplace( text, "`", "\\`", malloc, free );
    text = oyjlStringReplace( tmp, "-", "\\-", malloc, free );
    tmp = oyjlStringReplace( text, "_", "\\_", malloc, free );
    free(text);
    text = tmp;
  }

  return text;
}
// TODO: explicite allow for non option bound arguments, for syntax checking - use '-' as special option inside openiccOptionGroup_s::mandatory
// TODO: implement mandatory checks and support group::flags 0x01 - call group options independently
// TODO: make the qml renderer aware of mandatory options as part of sending a call to the tool; add action button to all manatory options except bool options; render mandatory switch as a button
// TODO: the renderer keeps as simple as possible like the command line
// TODO: MAN page synopsis logic ...
// TODO: man page generator: /usr/share/man/man1/ftp.1.gz + http://man7.org/linux/man-pages/man7/groff_mdoc.7.html
// TODO: synopsis syntax ideas: https://unix.stackexchange.com/questions/17833/understand-synopsis-in-manpage


/** 
 *  @} *//* args
 */

/* private stuff */

openiccUiHeaderSection_s * oiUiInfo  ( const char          * documentation )
{
  openiccUiHeaderSection_s s[] = {
    /* type,  nick,      label,name,                 description */
    { "oihs", "version", NULL, OPENICC_VERSION_NAME, NULL },
    { "oihs", "manufacturer", NULL, "Kai-Uwe Behrmann", "http://www.openicc.org" },
    { "oihs", "copyright", NULL, "Copyright 2018 Kai-Uwe Behrmann", NULL },
    { "oihs", "license", NULL, "newBSD", "http://www.openicc.org" },
    { "oihs", "url", NULL, "http://www.openicc.org", NULL },
    { "oihs", "support", NULL, "https://www.github.com/OpenICC/config/issues", NULL },
    { "oihs", "download", NULL, "https://github.com/OpenICC/config/releases", NULL },
    { "oihs", "sources", NULL, "https://github.com/OpenICC/config", NULL },
    { "oihs", "development", NULL, "https://github.com/OpenICC/config", NULL },
    { "oihs", "openicc_module_author", NULL, "Kai-Uwe Behrmann", "http://www.behrmann.name" },
    { "oihs", "documentation", NULL, "http://www.openicc.info", documentation },
    { "oihs", "date", NULL, "1970-01-01T12:00:00", "" },
    { "", NULL, NULL, NULL, NULL }
  };
  return openiccMemDup( s, sizeof(s) );
}

