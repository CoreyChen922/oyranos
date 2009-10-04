/** @file oyranos_cmm_oicc.c
 *
 *  Oyranos is an open source Colour Management System 
 *
 *  @par Copyright:
 *            2008-2009 (C) Kai-Uwe Behrmann
 *
 *  @brief    colour management policy module for Oyranos
 *  @internal
 *  @author   Kai-Uwe Behrmann <ku.b@gmx.de>
 *  @par License:
 *            new BSD <http://www.opensource.org/licenses/bsd-license.php>
 *  @since    2008/12/16
 */


#include "config.h"
#include "oyranos_alpha.h"
#include "oyranos_alpha_internal.h"
#include "oyranos_cmm.h"
#include "oyranos_helper.h"
#include "oyranos_icc.h"
#include "oyranos_i18n.h"
#include "oyranos_io.h"
#include "oyranos_definitions.h"
#include "oyranos_texts.h"

#include <iconv.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if !defined(WIN32)
#include <dlfcn.h>
#include <inttypes.h>
#endif

#define CMM_NICK "oicc"
oyMessage_f message = oyFilterMessageFunc;
int            oiccFilterMessageFuncSet( oyMessage_f       message_func );
int                oiccFilterInit      ( );
oyWIDGET_EVENT_e   oiccWidgetEvent   ( oyOptions_s       * options,
                                       oyWIDGET_EVENT_e    type,
                                       oyStruct_s        * event );

/** Function oiccFilterMessageFuncSet
 *  @brief
 *
 *  @version Oyranos: 0.1.10
 *  @date    2007/11/00
 *  @since   2007/11/00 (Oyranos: 0.1.8)
 */
int          oiccFilterMessageFuncSet( oyMessage_f         message_func )
{
  message = message_func;
  return 0;
}


/** Function oiccFilterInit
 *  @brief   API requirement
 *
 *  @version Oyranos: 0.1.10
 *  @date    2009/07/24
 *  @since   2009/07/24 (Oyranos: 0.1.10)
 */
int                oiccFilterInit      ( )
{
  int error = 0;
  return error;
}

/** Function oicc_defaultICCValidateOptions
 *  @brief   dummy
 *
 *  @version Oyranos: 0.1.9
 *  @since   2008/11/13 (Oyranos: 0.1.9)
 *  @date    2008/11/13
 */
oyOptions_s* oicc_defaultICCValidateOptions
                                     ( oyFilterCore_s    * filter,
                                       oyOptions_s       * validate,
                                       int                 statical,
                                       uint32_t          * result )
{
  uint32_t error = !filter;

  *result = error;

  return 0;
}


/*
 <xf:model> <xf:instance> - must be added in Oyranos to make the model complete
 */
char oicc_default_colour_icc_options[] = {
 "\n\
  <" OY_TOP_SHARED ">\n\
   <" OY_DOMAIN_STD ">\n\
    <" OY_TYPE_STD ">\n\
     <profile>\n\
      <editing_rgb.front>eciRGB_v2.icc</editing_rgb.front>\n\
      <editing_cmyk.front>coated_FOGRA39L_argl.icc</editing_cmyk.front>\n\
      <editing_gray.front>Gray.icc</editing_gray.front>\n\
      <editing_lab.front>Lab.icc</editing_lab.front>\n\
      <editing_xyz.front>XYZ.icc</editing_xyz.front>\n\
      <assumed_rgb.front>eciRGB_v2.icc</assumed_rgb.front>\n\
      <assumed_cmyk.front>coated_FOGRA39L_argl.icc</assumed_cmyk.front>\n\
      <assumed_gray.front>Gray.icc</assumed_gray.front>\n\
      <assumed_lab.front>Lab.icc</assumed_lab.front>\n\
      <assumed_xyz.front>XYZ.icc</assumed_xyz.front>\n\
     </profile>\n\
     <behaviour>\n\
      <action_untagged_assign.front>1</action_untagged_assign.front>\n\
      <action_missmatch_cmyk.front>1</action_missmatch_cmyk.front>\n\
      <action_missmatch_rgb.front>1</action_missmatch_rgb.front>\n\
      <mixed_colour_spaces_print_doc_convert.front>1</mixed_colour_spaces_print_doc_convert.front>\n\
      <mixed_colour_spaces_screen_doc_convert.front>2</mixed_colour_spaces_screen_doc_convert.front>\n\
      <proof_hard.advanced.front>0</proof_hard.advanced.front>\n\
      <proof_soft.advanced.front>0</proof_soft.advanced.front>\n\
      <rendering_intent>0</rendering_intent>\n\
      <rendering_bpc>1</rendering_bpc>\n\
      <rendering_intent_proof>0</rendering_intent_proof>\n\
      <rendering_gamut_warning.advanced>0</rendering_gamut_warning.advanced>\n\
      <rendering_high_precission.advanced>0</rendering_high_precission.advanced>\n\
     </behaviour>\n\
    </" OY_TYPE_STD ">\n\
   </" OY_DOMAIN_STD ">\n\
  </" OY_TOP_SHARED ">\n"
};

#define A(long_text) STRING_ADD( tmp, long_text)

char * oiccAddStdProfiles_routine    ( char               * tmp,
                                       oyPROFILE_e          profile_type )
{
  oyProfiles_s * iccs = 0;
  oyProfile_s * p = 0;
  int n,i;
  const char * profile_text,
             * file_name;
  const char * t = 0;

  iccs = oyProfiles_ForStd( profile_type, 0, 0 );
  n = oyProfiles_Count( iccs );
  for(i = 0; i < n; ++i)
  {
    p = oyProfiles_Get( iccs, i );
    file_name = oyProfile_GetFileName( p, 0 );
    profile_text = oyProfile_GetText( p, oyNAME_DESCRIPTION );

    if(oyStrrchr_(file_name,OY_SLASH_C))
      t = oyStrrchr_(file_name,OY_SLASH_C) + 1;
    else
      t = file_name;

    A("\n\
       <xf:item>\n");

    if(profile_text)
    {
      A("\
        <xf:label>");
      A(          profile_text);
      if(file_name)
      {
        A(                   " (");
        A(                      file_name);
        A(                              ")");
      }
      A(                     "</xf:label>\n");
    }

    if(t)
    {
    A("\
        <xf:value>");
    A(            t);
    A(            "</xf:value>\n");
    }
    A("\
       </xf:item>\n");

    oyProfile_Release( &p );
  }

  return tmp;
}

int oiccGetDefaultColourIccOptionsUI ( oyOptions_s        * options,
                                       char              ** ui_text,
                                       oyAlloc_f            allocateFunc )
{
  char * tmp = 0;
  oyOptions_s * os = options;
  const char * editing_rgb = oyOptions_FindString( os, "editing_rgb", 0 );
  const char * editing_cmyk = oyOptions_FindString( os, "editing_cmyk", 0 );
  const char * editing_gray = oyOptions_FindString( os, "editing_gray", 0 );
  const char * editing_lab = oyOptions_FindString( os, "editing_lab", 0 );
  const char * editing_xyz = oyOptions_FindString( os, "editing_xyz", 0 );
  const char * assumed_rgb = oyOptions_FindString( os, "assumed_rgb", 0 );
  const char * assumed_cmyk = oyOptions_FindString( os, "assumed_cmyk", 0 );
  const char * assumed_gray = oyOptions_FindString( os, "assumed_gray", 0 );
  const char * assumed_lab = oyOptions_FindString( os, "assumed_lab", 0 );
  const char * assumed_xyz = oyOptions_FindString( os, "assumed_xyz", 0 );
  const char * action_untagged_assign = oyOptions_FindString( os, "action_untagged_assign", 0 );

#if 0
  const char * action_missmatch_cmyk = oyOptions_FindString( os, "action_missmatch_cmyk", 0 );
  const char * action_missmatch_rgb = oyOptions_FindString( os, "action_missmatch_rgb", 0 );
  const char * mixed_colour_spaces_print_doc_convert = oyOptions_FindString( os, "mixed_colour_spaces_print_doc_convert", 0 );
  const char * mixed_colour_spaces_screen_doc_convert = oyOptions_FindString( os, "mixed_colour_spaces_screen_doc_convert", 0 );
  const char * proof_hard = oyOptions_FindString( os, "proof_hard", 0 );
  const char * proof_soft = oyOptions_FindString( os, "proof_soft", 0 );
  const char * rendering_intent = oyOptions_FindString( os, "rendering_intent", 0 );
  const char * rendering_bpc = oyOptions_FindString( os, "rendering_bpc", 0 );
  const char * rendering_intent_proof = oyOptions_FindString( os, "rendering_intent_proof", 0 );
  const char * rendering_gamut_warning = oyOptions_FindString( os, "rendering_gamut_warning", 0 );
  const char * rendering_high_precission = oyOptions_FindString( os, "rendering_high_precission", 0 );
#endif

  tmp = oyStringCopy_( "\
  <h3>Oyranos ", oyAllocateFunc_ );

  A(       _("Default Profiles"));
  A(                         ":</h3>\n");
  if(editing_rgb)
  {
    A("\
     <xf:select1 ref=\"/" OY_TOP_SHARED "/" OY_DOMAIN_STD "/" OY_TYPE_STD "/profile/editing_rgb\">\n\
      <xf:label>" );
    A(                   _("Editing Rgb"));
    A(                                "</xf:label>\n\
      <xf:choices>\n\
       <"CMM_NICK":profiles oyPROFILE_e=\"oyEDITING_RGB\"/>\n");
  tmp = oiccAddStdProfiles_routine( tmp, oyEDITING_RGB );
    A("\
      </xf:choices>\n\
     </xf:select1>\n\
     <br/>\n");
  }
  if(editing_cmyk)
  {
    A("\
     <xf:select1 ref=\"/" OY_TOP_SHARED "/" OY_DOMAIN_STD "/" OY_TYPE_STD "/profile/editing_cmyk\">\n\
      <xf:label>" );
    A(                   _("Editing Cmyk"));
    A(                                "</xf:label>\n\
      <xf:choices>\n\
       <"CMM_NICK":profiles oyPROFILE_e=\"oyEDITING_CMYK\"/>\n");
    tmp = oiccAddStdProfiles_routine( tmp, oyEDITING_CMYK );
    A("\
      </xf:choices>\n\
     </xf:select1>\n\
     <br/>\n");
  }
  if(editing_lab)
  {
    A("\
     <xf:select1 ref=\"/" OY_TOP_SHARED "/" OY_DOMAIN_STD "/" OY_TYPE_STD "/profile/editing_lab\">\n\
      <xf:label>" );
    A(                   _("Editing Lab"));
    A(                                "</xf:label>\n\
      <xf:choices>\n\
       <"CMM_NICK":profiles oyPROFILE_e=\"oyEDITING_LAB\"/>\n");
    tmp = oiccAddStdProfiles_routine( tmp, oyEDITING_LAB );
    A("\
      </xf:choices>\n\
     </xf:select1>\n\
     <br/>\n");
  }
  if(editing_xyz)
  {
    A("\
     <xf:select1 ref=\"/" OY_TOP_SHARED "/" OY_DOMAIN_STD "/" OY_TYPE_STD "/profile/editing_xyz\">\n\
      <xf:label>" );
    A(                   _("Editing XYZ"));
    A(                                "</xf:label>\n\
      <xf:choices>\n\
       <"CMM_NICK":profiles oyPROFILE_e=\"oyEDITING_XYZ\"/>\n");
  tmp = oiccAddStdProfiles_routine( tmp, oyEDITING_XYZ );
    A("\
      </xf:choices>\n\
     </xf:select1>\n\
     <br/>\n");
  }
  if(editing_gray)
  {
    A("\
     <xf:select1 ref=\"/" OY_TOP_SHARED "/" OY_DOMAIN_STD "/" OY_TYPE_STD "/profile/editing_gray\">\n\
      <xf:label>" );
    A(                   _("Editing Gray"));
    A(                                "</xf:label>\n\
      <xf:choices>\n\
       <"CMM_NICK":profiles oyPROFILE_e=\"oyEDITING_GRAY\"/>\n");
    tmp = oiccAddStdProfiles_routine( tmp, oyEDITING_GRAY );
    A("\
      </xf:choices>\n\
     </xf:select1>\n");
  }
  if(assumed_rgb)
  {
    A("\
     <xf:select1 ref=\"/" OY_TOP_SHARED "/" OY_DOMAIN_STD "/" OY_TYPE_STD "/profile/assumed_rgb\">\n\
      <xf:label>" );
    A(                   _("Assumed Rgb source"));
    A(                                "</xf:label>\n\
      <xf:choices>\n\
       <"CMM_NICK":profiles oyPROFILE_e=\"oyASSUMED_RGB\"/>\n");
  tmp = oiccAddStdProfiles_routine( tmp, oyASSUMED_RGB );
    A("\
      </xf:choices>\n\
     </xf:select1>\n\
     <br/>\n");
  }
  if(assumed_cmyk)
  {
    A("\
     <xf:select1 ref=\"/" OY_TOP_SHARED "/" OY_DOMAIN_STD "/" OY_TYPE_STD "/profile/assumed_cmyk\">\n\
      <xf:label>" );
    A(                   _("Assumed Cmyk source"));
    A(                                "</xf:label>\n\
      <xf:choices>\n\
       <"CMM_NICK":profiles oyPROFILE_e=\"oyASSUMED_CMYK\"/>\n");
    tmp = oiccAddStdProfiles_routine( tmp, oyASSUMED_CMYK );
    A("\
      </xf:choices>\n\
     </xf:select1>\n\
     <br/>\n");
  }
  if(assumed_lab)
  {
    A("\
     <xf:select1 ref=\"/" OY_TOP_SHARED "/" OY_DOMAIN_STD "/" OY_TYPE_STD "/profile/assumed_lab\">\n\
      <xf:label>" );
    A(                   _("Assumed Lab source"));
    A(                                "</xf:label>\n\
      <xf:choices>\n\
       <"CMM_NICK":profiles oyPROFILE_e=\"oyASSUMED_LAB\"/>\n");
    tmp = oiccAddStdProfiles_routine( tmp, oyASSUMED_LAB );
    A("\
      </xf:choices>\n\
     </xf:select1>\n\
     <br/>\n");
  }
  if(assumed_xyz)
  {
    A("\
     <xf:select1 ref=\"/" OY_TOP_SHARED "/" OY_DOMAIN_STD "/" OY_TYPE_STD "/profile/assumed_xyz\">\n\
      <xf:label>" );
    A(                   _("Assumed XYZ source"));
    A(                                "</xf:label>\n\
      <xf:choices>\n\
       <"CMM_NICK":profiles oyPROFILE_e=\"oyASSUMED_XYZ\"/>\n");
  tmp = oiccAddStdProfiles_routine( tmp, oyASSUMED_XYZ );
    A("\
      </xf:choices>\n\
     </xf:select1>\n\
     <br/>\n");
  }
  if(assumed_gray)
  {
    A("\
     <xf:select1 ref=\"/" OY_TOP_SHARED "/" OY_DOMAIN_STD "/" OY_TYPE_STD "/profile/assumed_gray\">\n\
      <xf:label>" );
    A(                   _("Assumed Gray source"));
    A(                                "</xf:label>\n\
      <xf:choices>\n\
       <"CMM_NICK":profiles oyPROFILE_e=\"oyASSUMED_GRAY\"/>\n");
    tmp = oiccAddStdProfiles_routine( tmp, oyASSUMED_GRAY );
    A("\
      </xf:choices>\n\
     </xf:select1>\n");
  }

  A("\
  <h3>Oyranos " );
  A(       _("Behaviour"));
  A(                         ":</h3>\n\
  <xf:group>\n");

  if(action_untagged_assign)
  {
    A("\
     <xf:select1 ref=\"/" OY_ACTION_UNTAGGED_ASSIGN "\">\n\
      <xf:help>" );
    A(      _("Image has no colour space embedded. What default action shall be performed?") );
    A(                "</xf:help>\n\
      <xf:label>" );
    A(       _("No Image profile"));
    A(                                "</xf:label>\n\
      <xf:choices>\n\
       <xf:item>\n\
        <xf:label>");
    A(         _("Assign No Profile"));
    A(                                "</xf:label>\n\
        <xf:value>0</xf:value>\n\
       </xf:item>\n\
       <xf:item>\n\
        <xf:label>");
    A(         _("Assign Assumed Profile"));
    A(                                "</xf:label>\n\
        <xf:value>1</xf:value>\n\
       </xf:item>\n\
       <xf:item>\n\
        <xf:label>");
    A(         _("Promt"));
    A(                                "</xf:label>\n\
        <xf:value>2</xf:value>\n\
       </xf:item>\n\
      </xf:choices>\n\
     </xf:select1>\n");
  }
  A("\
  </xf:group>" );


  if(allocateFunc && tmp)
  {
    char * t = oyStringCopy_( tmp, allocateFunc );
    oyFree_m_( tmp );
    tmp = t; t = 0;
  } else
    return 1;

  *ui_text = tmp;

  return 0;
}

oyWIDGET_EVENT_e   oiccWidgetEvent   ( oyOptions_s       * options,
                                       oyWIDGET_EVENT_e    type,
                                       oyStruct_s        * event )
{return 0;}


const char * oiccProfileGetText      ( oyStruct_s        * obj,
                                       oyNAME_e            type,
                                       int                 flags )
{
  const char * text = 0;

  if(!obj)
  {
    if(type == oyNAME_NAME)
      text = _("ICC profile");
    else if(type == oyNAME_DESCRIPTION)
      text = _("ICC colour profile for colour transformations");
    else
      text = "oyProfile_s";
  } else
  {
    if(obj &&
       obj->type_ == oyOBJECT_PROFILE_S)
      text = oyProfile_GetText( (oyProfile_s*)obj, type );
  }

  return text;
}

/** Function oiccObjectLoadFromMem
 *  @brief   load a ICC profile from a in memory data blob
 *
 *  @version Oyranos: 0.1.10
 *  @since   2008/11/23 (Oyranos: 0.1.9)
 *  @date    2009/09/14
 */
oyStruct_s * oiccProfileLoadFromMem   ( size_t              buf_size,
                                       const oyPointer     buf,
                                       uint32_t            flags,
                                       oyObject_s          object )
{
  return (oyStruct_s*) oyProfile_FromMem( buf_size, buf, flags, object );
}

/** Function oiccObjectScan
 *  @brief   load ICC profile informations from a in memory data blob
 *
 *  @version Oyranos: 0.1.10
 *  @since   2008/11/23 (Oyranos: 0.1.9)
 *  @date    2009/09/14
 */
int          oiccObjectScan       ( oyPointer           buf,
                                       size_t              buf_size,
                                       char             ** intern,
                                       char             ** filename,
                                       oyAlloc_f           allocateFunc )
{
  oyProfile_s * temp_prof = oyProfile_FromMem( buf_size, buf, 0, 0 );
  int error = !temp_prof;
  const char * internal = oyProfile_GetText( temp_prof, oyNAME_DESCRIPTION );
  const char * external = oyProfile_GetFileName( temp_prof, 0 );

  if(intern && internal)
    *intern = oyStringCopy_( internal, allocateFunc );

  if(filename && external)
    *filename = oyStringCopy_( external, allocateFunc );

  oyProfile_Release( &temp_prof );

  return error;
}

oyCMMobjectType_s icc_profile = {
  oyOBJECT_CMM_DATA_TYPES_S, /* oyStruct_s::type; */
  0,0,0, /* unused oyStruct_s fields in static data; keep to zero */
  oyOBJECT_PROFILE_S, /* id; */
  "color/icc", /* paths; sub paths */
  0, /* pathsGet */
  "icc:icm", /* exts; file name extensions */
  "profile", /* element_name; in XML documents */
  oiccProfileGetText, /* oyCMMobjectGetText; */
  oiccProfileLoadFromMem, /* oyCMMobjectLoadFromMem; */
  oiccObjectScan /* oyCMMobjectScan; */
};
oyCMMobjectType_s * icc_objects[] = {
  &icc_profile,
  0
};


void             oiccChangeNodeOption( oyOptions_s       * f_options,
                                       oyOptions_s       * db_options,
                                       const char        * key,
                                       oyConversion_s    * cc,
                                       int                 verbose )
{
  oyOption_s * o = 0,
             * db_o = 0;
  const char * tmp = 0;

  o = oyOptions_Find( f_options, key );
  /* only set missing options */
              if((o &&
                  !o->source & oyOPTIONSOURCE_USER &&
                  !o->source & oyOPTIONSOURCE_DATA &&
                  !o->flags & oyOPTIONATTRIBUTE_EDIT) ||
                 !o)
              {
                db_o = oyOptions_Find( db_options, key );
                if(db_o)
                {
                  db_o->flags |= oyOPTIONATTRIBUTE_AUTOMATIC;
                  oyOptions_MoveIn( f_options, &db_o, -1 );
                  if(oy_debug || verbose)
                    WARNc2_S("set %s: %s", key,
                             oyOptions_FindString(f_options,
                                                  key, 0) );
                } else
                  WARNc1_S("no in filter defaults \"%s\" found.", key);
              } else
              {
                tmp = oyOptions_FindString(f_options, key, 0);
                message( oyMSG_DBG,(oyStruct_s*)f_options,
                         "%s:%d \"%s\" is already set = %s",__FILE__,__LINE__,
                         key, tmp?tmp:"????");
              }
}

int           oiccConversion_Correct ( oyConversion_s    * conversion,
                                       oyOptions_s       * options )
{
  int error = 0, i,j,k,n,m,os_n,
      icc_nodes_n = 0,
      search, old_id, it;
  int verbose = oyOptions_FindString( options, "verbose", 0 ) ? 1:0;
  oyFilterGraph_s * g = 0;
  oyFilterNode_s * node = 0;
  oyFilterPlug_s * edge = 0;
  oyConversion_s * s = conversion;
  oyProfiles_s * proofs =  0;
  oyProfile_s * proof =  0;
  oyOptions_s * db_options = 0,
              * f_options = 0;
  oyOption_s * o = 0;
  const char * val = 0;

  if(s->input)
    g = oyFilterGraph_FromNode( s->input, 0 );
  else
    g = oyFilterGraph_FromNode( s->out_, 0 );

  n = oyFilterNodes_Count( g->nodes );
  for(i = 0; i < n; ++i)
  {
    node = oyFilterNodes_Get( g->nodes, i );
    if(oyFilterRegistrationMatch( node->core->registration_,
                                  "//" OY_TYPE_STD "/icc", 0 ))
    {
      if(verbose)
        WARNc2_S( "node: %s[%d]",
                  node->core->registration_, oyFilterNode_GetId( node ));
      ++icc_nodes_n;
    }
    oyFilterNode_Release( &node );
  }

  /* How far is this ICC node from the output node? */
  if(verbose)
    oyShowGraph_( conversion->input, 0 );

  m = oyFilterPlugs_Count( g->edges );
  old_id = -1;
  /* start from out_ and search all ICC CMMs */
  if(s->out_)
  for(i = 0; i < n; ++i)
  {
    node = oyFilterNodes_Get( g->nodes, i );

    if(oyFilterNode_GetId( node ) == oyFilterNode_GetId( s->out_ ))
      search = 1;
    else
      search = 0;

    old_id = oyFilterNode_GetId( node );
    if(verbose && search)
      printf("ICC CMM search: %d - ", old_id);

    /* search for a path to a "icc" CMM */
    while(search)
    {
      it = 0;

      /* follow the path along the filter node IDs */
      for(j = 0; j < m; ++j)
      {
        edge = oyFilterPlugs_Get( g->edges, j );
        if(oyFilterNode_GetId( edge->node ) == old_id)
        {
          /* select only application level "data" coonectors; follow the data */
          if(oyFilterRegistrationMatch( edge->pattern->connector_type,
                                        "//" OY_TYPE_STD "/data", 0) &&
             oyFilterRegistrationMatch( edge->remote_socket_->pattern->connector_type,
                                        "//" OY_TYPE_STD "/data", 0))
          {
            node = oyFilterNode_Copy( edge->remote_socket_->node, 0 );
            old_id = oyFilterNode_GetId( node );
            ++it;
            /* stop at the first hit if "icc" */
            if( oyFilterRegistrationMatch( node->core->registration_,
                                           "//" OY_TYPE_STD "/icc", 0))
            {
              search = 0;

              /* apply the found policy settings */
              db_options = oyOptions_ForFilter( node->core->registration_, 0,
                                                oyOPTIONATTRIBUTE_ADVANCED |
                                                oyOPTIONATTRIBUTE_FRONT, 0 );
              f_options = oyFilterNode_OptionsGet( node, 0 );
              os_n = oyOptions_Count(f_options);
              if(oy_debug || verbose)
              for(k = 0; k < os_n; k++)
              {
                o = oyOptions_Get( f_options, k );
                printf("%d: \"%s\": \"%s\" %s %d\n", k, 
                       oyOption_GetText( o, oyNAME_DESCRIPTION ),
                       o->value->string,
           oyFilterRegistrationToText( oyOption_GetText( o, oyNAME_DESCRIPTION),
                                       oyFILTER_REG_OPTION, 0 ),
                o->flags );

                oyOption_Release( &o );
              }
              os_n = oyOptions_Count(db_options);
              if(oy_debug || verbose)
              for(k = 0; k < os_n; k++)
              {
                o = oyOptions_Get( db_options, k );
                printf("%d: \"%s\": \"%s\" %s %d\n", k, 
                       oyOption_GetText( o, oyNAME_DESCRIPTION ),
                       o->value->string,
           oyFilterRegistrationToText( oyOption_GetText( o, oyNAME_DESCRIPTION),
                                       oyFILTER_REG_OPTION, 0 ),
                o->flags );

                oyOption_Release( &o );
              }

              oiccChangeNodeOption( f_options, db_options,
                                    "proof_soft", s, verbose);
              oiccChangeNodeOption( f_options, db_options,
                                    "proof_hard", s, verbose);
              oiccChangeNodeOption( f_options, db_options,
                                    "rendering_intent", s, verbose);
              oiccChangeNodeOption( f_options, db_options,
                                    "rendering_bpc", s, verbose);
              oiccChangeNodeOption( f_options, db_options,
                                    "rendering_intent_proof", s, verbose);
              oiccChangeNodeOption( f_options, db_options,
                                    "rendering_gamut_warning", s, verbose);
              oiccChangeNodeOption( f_options, db_options,
                                    "rendering_high_precission", s, verbose);

              /* TODO @todo add proofing profile */
              o = oyOptions_Find( f_options, "profiles_simulation" );
              if(!o)
              {
                proof = oyProfile_FromStd( oyPROFILE_PROOF, 0 );
                proofs = oyProfiles_New(0);
                val = oyProfile_GetText( proof, oyNAME_NICK );
                oyProfiles_MoveIn( proofs, &proof, -1 );
                oyOptions_MoveInStruct( &f_options,
                                    "//" OY_TYPE_STD "/icc/profiles_simulation",
                                        (oyStruct_s**)& proofs,
                                        OY_CREATE_NEW );
                if(verbose)
                  message( oyMSG_WARN,(oyStruct_s*)node,
                           "%s:%d set \"profiles_simulation\": %s",
                           __FILE__,__LINE__, val);
              } else if(verbose)
                message( oyMSG_WARN,(oyStruct_s*)node,
                         "%s:%d \"profiles_simulation\" is already set",
                         __FILE__,__LINE__);

              oyOption_Release( &o );

              oyOptions_Release( &db_options );
              oyOptions_Release( &f_options );

              break;
            }
          }
        }
        oyFilterPlug_Release( &edge );
      }

      if(verbose)
      {
        if(search)
          printf("%d - ", old_id);
        else
          printf("%d[icc]\n", old_id );
      }
    }
    oyFilterNode_Release( &node );
  }

  if(verbose)
  WARNc_S("not completely implemented");

  return error;
}


/** @instance oicc_api9
 *  @brief    oicc oyCMMapi9_s implementation
 *
 *  a policy ashuring plug-in interpreter for ICC CMM's
 *
 *  @version Oyranos: 0.1.10
 *  @since   2008/11/13 (Oyranos: 0.1.9)
 *  @date    2009/07/23
 */
oyCMMapi9_s  oicc_api9 = {

  oyOBJECT_CMM_API9_S, /* oyStruct_s::type */
  0,0,0, /* unused oyStruct_s fileds; keep to zero */
  0, /* oyCMMapi_s * next */
  
  oiccFilterInit, /* oyCMMInit_f */
  oiccFilterMessageFuncSet, /* oyCMMMessageFuncSet_f */

  /* registration */
  OY_TOP_SHARED OY_SLASH OY_DOMAIN_INTERNAL OY_SLASH OY_TYPE_STD OY_SLASH "icc." CMM_NICK,

  {0,0,1}, /* int32_t version[3] */
  {0,1,10},                  /**< int32_t module_api[3] */
  0,   /* id_; keep empty */
  0,   /* oyCMMapi5_s    * api5_; keep empty */

  oicc_defaultICCValidateOptions, /* oyCMMFilter_ValidateOptions_f */
  oiccWidgetEvent, /* oyWidgetEvent_f */

  oicc_default_colour_icc_options,   /* options */
  oiccGetDefaultColourIccOptionsUI,  /* oyCMMuiGet */
  (char*)CMM_NICK"=\"http://www.oyranos.org/2009/oyranos_icc\"", /* xml_namespace */

  icc_objects,  /* object_types */

  0,  /* getText */
  0,  /* texts */

  /** oyConversion_Correct_f oyConversion_Correct; check a graph */
  oiccConversion_Correct,

  /** const char * pattern; a pattern supported by oiccConversion_Correct */
  "//" OY_TYPE_STD
};


/**
 *  This function implements oyCMMGetText_f.
 *
 *  @version Oyranos: 0.1.10
 *  @since   2008/12/23 (Oyranos: 0.1.10)
 *  @date    2008/12/30
 */
const char * oiccInfoGetText         ( const char        * select,
                                       oyNAME_e            type )
{
         if(strcmp(select, "name")==0)
  {
         if(type == oyNAME_NICK)
      return _(CMM_NICK);
    else if(type == oyNAME_NAME)
      return _("Oyranos ICC policy");
    else
      return _("Oyranos ICC policy module");
  } else if(strcmp(select, "manufacturer")==0)
  {
         if(type == oyNAME_NICK)
      return _("Kai-Uwe");
    else if(type == oyNAME_NAME)
      return _("Kai-Uwe Behrmann");
    else
      return _("Oyranos project; www: http://www.oyranos.com; support/email: ku.b@gmx.de; sources: http://www.oyranos.com/wiki/index.php?title=Oyranos/Download");
  } else if(strcmp(select, "copyright")==0)
  {
         if(type == oyNAME_NICK)
      return _("newBSD");
    else if(type == oyNAME_NAME)
      return _("Copyright (c) 2005-2009 Kai-Uwe Behrmann; newBSD");
    else
      return _("new BSD license: http://www.opensource.org/licenses/bsd-license.php");
  } else if(strcmp(select, "help")==0)
  {
         if(type == oyNAME_NICK)
      return _("help");
    else if(type == oyNAME_NAME)
      return _("The filter is provides policy settings. These settings can be applied to a graph through the user function oyConversion_Correct().");
    else
      return _("The module is responsible for many settings in the Oyranos colour management settings panel. If applied the module care about rendering intents, simulation, mixed colour documents and default profiles.");
  }
  return 0;
}
const char *oicc_texts[5] = {"name","copyright","manufacturer","help",0};


/** @instance oicc_cmm_module
 *  @brief    oicc module infos
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/07/23 (Oyranos: 0.1.10)
 *  @date    2009/07/23
 */
oyCMMInfo_s oicc_cmm_module = {

  oyOBJECT_CMM_INFO_S,
  0,0,0,
  CMM_NICK,
  "0.1.10",
  oiccInfoGetText,                     /**< oyCMMGetText_f getText */
  (char**)oicc_texts,                  /**<texts; list of arguments to getText*/
  OYRANOS_VERSION,

  (oyCMMapi_s*) & oicc_api9,

  {oyOBJECT_ICON_S, 0,0,0, 0,0,0, "oyranos_logo.png"},
};

