#include "oyOptions_s.h"
#include <stdio.h>
void testOyjl( void );
int main (int argc OY_UNUSED, char ** argv OY_UNUSED)
{
  int error = 0;
  oyOptions_s * result = 0;
  const char * t = NULL,
             * json;
  FILE * zout = stdout;

  json = "{\"org\":{\"free\":[{\"s1key_a\":\"val_a\",\"s1key_b\":\"val_b\"},{\"s2key_c\":\"val_c\",\"s2key_d\":\"val_d\"}],\"key_e\":\"val_e\"}}";
  oyOptions_s * options = NULL;
  error = oyOptions_FromJSON( json, options, &result, "org" );
  t = oyOptions_GetText( result, oyNAME_NICK );
  fprintf( zout, "%s\n", t?t:0 );

  json = "{\"org\":{\"free\":[{\"s1key_a\":\"val_a\",\"s1key_b\":\"val_b\"},{\"s2key_c\":\"val_c\",\"s2key_d\":\"val_d\"}],\"key_e\":\"val_e_xxx\"}}";
  error = oyOptions_FromJSON( json, options, &result, "org" );
  t = oyOptions_GetText( result, oyNAME_NICK );
  fprintf( zout, "%s\n", t?t:0 );

  json = "{\"org\":{\"free\":[{\"s1key_a\":\"val_a\",\"s1key_b\":\"val_b\"},{\"s2key_c\":\"val_c\",\"s2key_d\":\"val_d\"}],\"key_e\":\"val_e_yyy\",\"key_f\":\"val_f\"}}";
  error = oyOptions_FromJSON( json, options, &result, "org" );
  t = oyOptions_GetText( result, oyNAME_NICK );
  fprintf( zout, "%s\n", t?t:0 );

  oyOptions_SetFromString( &options, OY_STD "/key_path", 
                                   "net/host/path", OY_CREATE_NEW);
  json = "{\"org\":{\"free\":[{\"s1key_a\":\"val_a\",\"s1key_b\":\"val_b\"},{\"s2key_c\":\"val_c\",\"s2key_d\":\"val_d\"}],\"key_e\":\"val_e_yyy\",\"key_f\":\"val_f\"}}";
  error = oyOptions_FromJSON( json, options, &result, "org/free/[%d]", 1 );
  t = oyOptions_GetText( result, oyNAME_NICK );
  fprintf( zout, "%s\n", t?t:0 );

  if(error)
    fprintf( zout, "oyOptions_FromJSON() returned %d\n", error );

  oyOptions_Release( &options );

  oyOptions_Release( &result );
  
  testOyjl( );

  return 0;
}

#include "oyranos_json.h"
void testOyjl(void)
{
  /* JSON string */
  const char * text = "{\"org\":{\"test\":[{\"s1key_a\":\"val_a\",\"s1key_b\":\"val_b\"},{\"s2key_c\":\"val_c\",\"s2key_d\":\"val_d\"}],\"key_e\":\"val_e_yyy\",\"key_f\":\"val_f\"}}";

  oyjl_val value = 0;
  int level = 0;
  char * json = 0;
  char error_buffer[128];

  /* read JSON into C data struct */
  oyjl_val root = oyjlTreeParse( text, error_buffer, 128 );
  
  /* convert back to JSON */
  oyjlTreeToJson( root, &level, &json );

    fprintf( stderr, "%s\n", json );
    free(json); json = NULL;

  /* use a xpath to obtain a node */
  value = oyjlTreeGetValuef( root, 0, "org/test/[%d]", 1 );

    oyjlTreeToJson( value, &level, &json );
    fprintf( stderr, "%s\n", json );
    free(json); json = NULL;

  /* use a xpath to remove a node */
  oyjlTreeClearValue( root, "org/test/[0]" );

    oyjlTreeToJson( root, &level, &json );
    fprintf( stderr, "%s\n", json );
    free(json); json = NULL;

  /* use a xpath to get a new node in a existing tree */
  value = oyjlTreeGetValue( root, OYJL_CREATE_NEW, "org/add/opt" );

    oyjlTreeToJson( root, &level, &json );
    fprintf( stderr, "%s\n", json );
    free(json); json = NULL;

  /* set the new node to some string value  */
  oyjlValueSetString( value, "value" );

    oyjlTreeToJson( root, &level, &json );
    fprintf( stderr, "%s\n", json );
    free(json); json = NULL;

  /* release memory */
  oyjlTreeFree ( root );

  /* use a xpath to create new tree */
  root = oyjlTreeNew( "new/tree/key" );

    oyjlTreeToJson( root, &level, &json );
    fprintf( stderr, "%s\n", json );
    free(json); json = NULL;

  /* release memory */
  oyjlTreeFree( root );
}
