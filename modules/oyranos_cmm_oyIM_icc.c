/** @file oyranos_cmm_oyIM_icc.c
 *
 *  Oyranos is an open source Colour Management System 
 *
 *  @par Copyright:
 *            2008-2009 (C) Kai-Uwe Behrmann
 *
 *  @brief    meta backend for ICC support in Oyranos
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
#include "oyranos_cmm_oyIM.h"
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

/** Function oyIMFilter_CanHandle
 *  @brief   dummy
 *
 *  @version Oyranos: 0.1.9
 *  @since   2008/11/13 (Oyranos: 0.1.9)
 *  @date    2008/11/13
 */
int    oyIMFilter_CanHandle          ( oyCMMQUERY_e      type,
                                       uint32_t          value )
{
  int ret = -1;

  return ret;
}

/** Function oyIM_defaultICCValidateOptions
 *  @brief   dummy
 *
 *  @version Oyranos: 0.1.9
 *  @since   2008/11/13 (Oyranos: 0.1.9)
 *  @date    2008/11/13
 */
oyOptions_s* oyIM_defaultICCValidateOptions
                                     ( oyFilterCore_s    * filter,
                                       oyOptions_s       * validate,
                                       int                 statical,
                                       uint32_t          * result )
{
  uint32_t error = !filter;

  if(!error)
    error = filter->type_ != oyOBJECT_FILTER_CORE_S;

  *result = error;

  return 0;
}


/*
 <xf:model> <xf:instance> - must be added in Oyranos to make the model complete
 */
char oyIM_default_colour_icc_options[] = {
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

int oyIMGetDefaultColourIccOptionsUI ( oyOptions_s        * options,
                                       char              ** ui_text,
                                       oyAlloc_f            allocateFunc )
{
  char * tmp = 0;

  oyStringCopy_( "\
  <h3>Oyranos ", oyAllocateFunc_ );

  A(       _("Default Profiles"));
  A(                         ":</h3>\n\
  <table>\n\
   <tr>\n\
    <td>" );
  A( _("Editing Rgb"));
  A(              ":</td>\n\
    <td>\n\
     <xf:select1 ref=\"/" OY_TOP_SHARED "/" OY_DOMAIN_STD "/default/profile/editing_rgb\">\n\
      <xf:choices label=\"" );
  A(                   _("Editing Rgb"));
  A(                                "\">\n\
       <sta:profiles cspace1=\"RGB\" class1=\"prtr\" class2=\"mntr\" class3=\"scnr\"/>\n\
       <xf:item>\n\
        <xf:label>sRGB.icc</xf:label>\n\
        <xf:value>sRGB.icc</xf:value>\n\
       </xf:item>\n\
       <xf:item>\n\
        <xf:label>eciRGB_v2.icc</xf:label>\n\
        <xf:value>eciRGB_v2.icc</xf:value>\n\
       </xf:item>\n\
      </xf:choices>\n\
     </xf:select1>\n\
    </td>\n\
   </tr>\n\
   <tr>\n\
    <td>" );
  A( _("Editing Cmyk"));
  A(               ":</td>\n\
    <td>\n\
     <xf:select1 ref=\"/" OY_TOP_SHARED "/" OY_DOMAIN_STD "/default/profile/editing_cmyk\">\n\
      <xf:choices>\n\
       <sta:profiles cspace1=\"CMYK\" class1=\"prtr\"/>\n\
       <xf:item>\n\
        <xf:label>coated_FOGRA39L_argl.icc</xf:label>\n\
        <xf:value>coated_FOGRA39L_argl.icc</xf:value>\n\
       </xf:item>\n\
      </xf:choices>\n\
     </xf:select1>\n\
    </td>\n\
   </tr>\n\
   <tr>\n\
    <td>" );
  A( _("Editing Lab"));
  A(              ":</td>\n\
    <td>\n\
     <xf:select1 ref=\"/" OY_TOP_SHARED "/" OY_DOMAIN_STD "/default/profile/editing_lab\">\n\
      <xf:choices xml:lang=\"en\" label=\"Editing Lab\">\n\
       <sta:profiles cspace1=\"Lab\" class1=\"prtr\" class2=\"mntr\" class3=\"scnr\"/>\n\
       <xf:item>\n\
        <xf:label>Lab.icc</xf:label>\n\
        <xf:value>Lab.icc</xf:value>\n\
       </xf:item>\n\
       <xf:item>\n\
        <xf:label>CIELab.icc</xf:label>\n\
        <xf:value>CIELab.icc</xf:value>\n" );
  A(  "</xf:item>\n\
      </xf:choices>\n\
     </xf:select1>\n\
    </td>\n\
   </tr>\n\
   <tr>\n\
    <td>" );
  A( _("Editing XYZ") );
  A(              ":</td>\n\
    <td>\n\
     <xf:select1 ref=\"/" OY_TOP_SHARED "/" OY_DOMAIN_STD "/" OY_TYPE_STD "/profile/editing_xyz\">\n\
      <xf:choices>\n\
       <sta:profiles cspace1=\"XYZ\" class1=\"prtr\" class2=\"mntr\" class3=\"scnr\"/>\n\
       <xf:item>\n\
        <xf:label>XYZ.icc</xf:label>\n\
        <xf:value>XYZ.icc</xf:value>\n\
       </xf:item>\n\
       <xf:item>\n\
        <xf:label>CIEXYZ.icc</xf:label>\n\
        <xf:value>CIEXYZ.icc</xf:value>\n\
       </xf:item>\n\
      </xf:choices>\n\
     </xf:select1>\n\
    </td>\n\
   </tr>\n\
   <tr>\n\
    <td>" );
  A( _("Editing Gray"));
  A(               ":</td>\n\
    <td>\n\
     <xf:select1 ref=\"/" OY_TOP_SHARED "/" OY_DOMAIN_STD "/" OY_TYPE_STD "/profile/editing_gray\">\n\
      <xf:choices>\n\
       <sta:profiles cspace1=\"Gray\" class1=\"prtr\" class2=\"mntr\" class3=\"scnr\"/>\n\
       <xf:item>\n\
        <xf:label>Grau.icc</xf:label>\n\
        <xf:value>Grau.icc</xf:value>\n\
       </xf:item>\n\
       <xf:item>\n\
        <xf:label>Gray.icc</xf:label>\n\
        <xf:value>Gray.icc</xf:value>\n\
       </xf:item>\n\
      </xf:choices>\n\
     </xf:select1>\n\
    </td>\n\
   </tr>\n\
  </table>\n\
" );

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

char * oyIMstructGetText             ( oyStruct_s        * item,
                                       oyNAME_e            type,
                                       int                 flags,
                                       oyAlloc_f           allocateFunc )
{
  char * text = 0;
  oyProfile_s * prof = 0;
  oyImage_s * image = 0;

  if(item->type_ == oyOBJECT_PROFILE_S)
  {
    text = oyStringCopy_( oyProfile_GetText( prof, oyNAME_DESCRIPTION ),
                          allocateFunc );
  } else if(item->type_ == oyOBJECT_IMAGE_S)
  {
    image = (oyImage_s*) item;

    if(flags == oyOBJECT_PROFILE_S)
      text = oyStringCopy_( oyProfile_GetText( image->profile_,
                                               type ),
                            allocateFunc );
    else
      text = oyStringCopy_( oyObject_GetName( image->oy_, type ),
                            allocateFunc );
  }

  return text;
}

char * oyIMDataGetText               ( oyStruct_s        * data,
                                       oyNAME_e            type,
                                       int                 pos,
                                       int                 flags,
                                       oyAlloc_f           allocateFunc )
{
  int n = 0;
  oyStructList_s * list = 0;
  oyStruct_s * item = 0;
  char * text = 0;

  if(!data)
  {
    if(type == oyNAME_NAME)
      text = oyStringCopy_( _("ICC profile"), allocateFunc );
    else if(type == oyNAME_DESCRIPTION)
      text = oyStringCopy_( _("ICC colour profile for colour transformations"),
                            allocateFunc );
    else
      text = oyStringCopy_( OY_TYPE_STD, allocateFunc );
  } else
  {
    if(data->type_ == oyOBJECT_STRUCT_LIST_S)
    {
      list = (oyStructList_s*) data;
      n = oyStructList_Count( list );
      item = oyStructList_GetRef( list, pos );
    } else
      item = data;

    if(item &&
       !(item->type_ == oyOBJECT_PROFILE_S ||
         item->type_ == oyOBJECT_IMAGE_S))
      item = 0;

    if(item)
      text = oyIMstructGetText( item, type, flags, allocateFunc );
  }

  return text;
}

/** Function oyIMICCDataLoadFromMem
 *  @brief   load a ICC profile from a in memory data blob
 *
 *  @version Oyranos: 0.1.9
 *  @since   2008/11/23 (Oyranos: 0.1.9)
 *  @date    2008/11/23
 */
oyStruct_s * oyIMICCDataLoadFromMem  ( size_t              buf_size,
                                       const oyPointer     buf,
                                       uint32_t            flags,
                                       oyObject_s          object )
{
  return (oyStruct_s*) oyProfile_FromMem( buf_size, buf, flags, object );
}

/** Function oyIMICCDataScan
 *  @brief   load ICC profile informations from a in memory data blob
 *
 *  @version Oyranos: 0.1.9
 *  @since   2008/11/23 (Oyranos: 0.1.9)
 *  @date    2008/11/23
 */
int          oyIMICCDataScan         ( oyPointer           buf,
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

oyCMMDataTypes_s icc_data[] = {
 {
  oyOBJECT_CMM_API5_S, /* oyStruct_s::type oyOBJECT_CMM_API5_S */
  0,0,0, /* unused oyStruct_s fileds; keep to zero */
  0, /* id */
  "color/icc", /* sub paths */
  "icc:icm", /* file name extensions */
  oyIMDataGetText, /* oyCMMDataGetText */
  oyIMICCDataLoadFromMem, /* oyCMMDataLoadFromMem */
  oyIMICCDataScan /* oyCMMDataScan */
 },{0} /* zero list end */
};

/**
 *  @brief   oyIM oyCMMapi5_s implementation
 *
 *  A filter interpreter loading. This function implements
 *  oyCMMFilterLoad_f for oyCMMapi5_s::oyCMMFilterLoad().
 *
 *  @version Oyranos: 0.1.10
 *  @since   2008/12/17 (Oyranos: 0.1.10)
 *  @date    2008/12/17
 */
oyCMMapiFilter_s * oyIMFilterLoad    ( oyPointer           data,
                                       size_t              size,
                                       const char        * file_name,
                                       oyOBJECT_e          type,
                                       int                 num )
{
  oyCMMapiFilter_s * api = 0;
  api = (oyCMMapiFilter_s*) oyCMMsGetApi__( type, file_name, 0,0, num );
  return api;
}

#ifdef NO_OPT
#define DLOPEN 1
#endif

/**
 *  @brief   oyIM oyCMMapi5_s implementation
 *
 *  A interpreter preview for filters. This function implements
 *  oyCMMFilterScan_f for oyCMMapi5_s::oyCMMFilterScan().
 *
 *  @version Oyranos: 0.1.10
 *  @since   2008/12/13 (Oyranos: 0.1.10)
 *  @date    2008/12/13
 */
int          oyIMFilterScan          ( oyPointer           data,
                                       size_t              size,
                                       const char        * lib_name,
                                       oyOBJECT_e          type,
                                       int                 num,
                                       char             ** registration,
                                       char             ** name,
                                       oyAlloc_f           allocateFunc,
                                       oyCMMInfo_s      ** info,
                                       oyObject_s          object )
{
  oyCMMInfo_s * cmm_info = 0;
  oyCMMapi_s * api = 0;
  oyCMMapi4_s * api4 = 0;
  int error = !lib_name;
  int ret = -2;
  char * cmm = oyCMMnameFromLibName_(lib_name);

  if(!error)
  {
#if DLOPEN
    oyPointer dso_handle = 0;

    if(!error)
    {
      if(lib_name)
        dso_handle = dlopen( lib_name, RTLD_LAZY );

      error = !dso_handle;

      if(error)
      {
        WARNc2_S( "\n  dlopen( %s, RTLD_LAZY):\n  \"%s\"", lib_name, dlerror() );
        system("  echo $LD_LIBRARY_PATH");
      }
    }
#endif

    /* open the module */
    if(!error)
    {
#if DLOPEN
      char * info_sym = oyAllocateFunc_(24);

      oySprintf_( info_sym, "%s%s", cmm, OY_MODULE_NAME );
#endif

#if DLOPEN
      cmm_info = (oyCMMInfo_s*) dlsym (dso_handle, info_sym);

      if(info_sym)
        oyFree_m_(info_sym);
#else
      cmm_info = oyCMMInfoFromLibName_( lib_name );
#endif

      error = !cmm_info;

      if(error)
        WARNc2_S("\n  %s:\n  \"%s\"", lib_name, dlerror() );

      if(!error)
        if(oyCMMapi_Check_( cmm_info->api ))
          api = cmm_info->api;

      if(!error && api)
      {
        int x = 0;
        int found = 0;
        while(!found)
        {
          if(api && api->type == type)
          {
            if(x == num)
              found = 1;
            else
              ++x;
          }
          if(!api)
            found = 1;
          if(!found)
            api = api->next;
        }

        if(api && found)
        {
          if(api->type == type)
            api4 = (oyCMMapi4_s *) api;
          if(registration)
            *registration = oyStringCopy_( api4->registration, allocateFunc );
          if(name)
            *name = oyStringCopy_( api4->name.name, allocateFunc );
          if(info)
            *info = oyCMMInfo_Copy( cmm_info, object );
          ret = 0;
        } else
          ret = -1;
      }
    }

#if DLOPEN
    if(dso_handle)
      dlclose( dso_handle );
    dso_handle = 0;
#endif
  }

  if(error)
    ret = error;

  if(cmm)
    oyDeAllocateFunc_(cmm);
  cmm = 0;

  return ret;
}

/** Function oyFilterSocket_MatchImagingPlug
 *  @brief   verify connectors matching each other
 *  @ingroup backend_api
 *  @memberof oyCMMapi5_s
 *
 *  @param         socket              a filter socket
 *  @param         plug                a filter plug
 *  @return                            1 on success, otherwise 0
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/04/20 (Oyranos: 0.1.10)
 *  @date    2009/04/28
 */
int          oyFilterSocket_MatchImagingPlug (
                                       oyFilterSocket_s  * socket,
                                       oyFilterPlug_s    * plug )
{
  int match = 0;
  oyConnectorImaging_s * a = 0,  * b = 0;
  oyImage_s * image = 0;
  int colours_n = 0, n, i, j;
  int coff = 0;
  oyDATATYPE_e data_type = 0;

  if(socket && socket->type_ == oyOBJECT_FILTER_SOCKET_S &&
     socket->pattern && socket->pattern->type_ == oyOBJECT_CONNECTOR_IMAGING_S)
    a = (oyConnectorImaging_s*)socket->pattern;

  if(plug && plug->type_ == oyOBJECT_FILTER_PLUG_S &&
     plug->pattern && plug->pattern->type_ == oyOBJECT_CONNECTOR_IMAGING_S)
    b = (oyConnectorImaging_s*) plug->pattern;

  if(a && b)
  {
    match = 1;
    image = oyImage_Copy( (oyImage_s*)socket->data, 0 );

    if(!b->is_plug)
      match = 0;

    /** For a zero set pixel layout we skip most tests and assume it will be
        checked later. */
    if(image && image->layout_[oyLAYOUT] && match)
    {
      coff = oyToColourOffset_m( image->layout_[oyLAYOUT] );

      /* channel counts */
      colours_n = oyProfile_GetChannelsCount( image->profile_ );
      if(image->layout_[oyCHANS] < b->min_channels_count ||
         image->layout_[oyCHANS] > b->max_channels_count ||
         colours_n < b->min_colour_count ||
         colours_n > b->max_colour_count)
        match = 0;

      /* data types */
      if(match)
      {
        data_type = oyToDataType_m( image->layout_[oyLAYOUT] );
        n = b->data_types_n;
        match = 0;
        for(i = 0; i < n; ++i)
          if(b->data_types[i] == data_type)
            match = 1;
      }

      /* planar and interwoven capabilities */
      if(b->max_colour_offset < image->layout_[oyCOFF] ||
         (!b->can_planar && oyToPlanar_m(image->layout_[oyCOFF])) ||
         (!b->can_interwoven && !oyToPlanar_m(image->layout_[oyCOFF])))
        match = 0;

      /* swap and byteswapping capabilities */
      if((!b->can_swap && oyToSwapColourChannels_m(image->layout_[oyCOFF])) ||
         (!b->can_swap_bytes && oyToByteswap_m(image->layout_[oyCOFF])))
        match = 0;

      /* revert or chockolat and vanilla */
      if((!b->can_revert && oyToFlavor_m(image->layout_[oyCOFF])))
        match = 0;

      /* channel types */
      if(match && b->channel_types)
      {
        n = image->layout_[oyCHANS];
        for(i = 0; i < b->channel_types_n; ++i)
        {
          match = 0;
          for(j = 0; j < n; ++j)
            if(b->channel_types[i] == image->channel_layout[j] &&
               !(!b->can_nonpremultiplied_alpha &&
                 image->channel_layout[j] == oyCHANNELTYPE_COLOUR_LIGHTNESS) &&
               !(!b->can_premultiplied_alpha &&
                 image->channel_layout[j] == oyCHANNELTYPE_COLOUR_LIGHTNESS_PREMULTIPLIED))
              match = 1;
          if(!match)
            break;
        }
      }

      /* subpixels */
      if(image->sub_positioning && !b->can_subpixel)
        match = 0;
    }
  }

  oyImage_Release( &image );
  oyConnectorImaging_Release( &a );

  return match;
}

/** @instance oyIM_api5
 *  @brief    oyIM oyCMMapi5_s implementation
 *
 *  a interpreter for ICC CMM's
 *
 *  @version Oyranos: 0.1.10
 *  @since   2008/11/13 (Oyranos: 0.1.9)
 *  @date    2008/11/30
 */
oyCMMapi5_s  oyIM_api5_colour_icc = {

  oyOBJECT_CMM_API5_S, /* oyStruct_s::type */
  0,0,0, /* unused oyStruct_s fileds; keep to zero */
  0, /* oyCMMapi_s * next */
  
  oyIMCMMInit, /* oyCMMInit_f */
  oyIMCMMMessageFuncSet, /* oyCMMMessageFuncSet_f */
  oyIMFilter_CanHandle, /* oyCMMCanHandle_f */

  /* @todo registration: for what? */
  OY_TOP_INTERNAL OY_SLASH OY_DOMAIN_INTERNAL OY_SLASH OY_TYPE_STD ".image.tonemap.colour" OY_SLASH "icc." CMM_NICK,

  {0,0,1}, /* int32_t version[3] */
  0,   /* id_; keep empty */

  OY_CMMSUBPATH, /* sub_paths */
  0, /* ext */
  0, /* data_type - 0: libs - libraries,\n  1: scripts - platform independent filters */

  oyIMFilterLoad, /* oyCMMFilterLoad_f oyCMMFilterLoad */
  oyIMFilterScan, /* oyCMMFilterScan_f */

  oyIM_defaultICCValidateOptions, /* oyCMMFilter_ValidateOptions_f */
  oyIMWidgetEvent, /* oyWidgetEvent_f */

  oyIM_default_colour_icc_options,   /* options */
  oyIMGetDefaultColourIccOptionsUI,  /* oyCMMuiGet */

  icc_data,  /* data_types */
  0,  /* getText */
  0,  /* texts */
  oyFilterSocket_MatchImagingPlug /* oyCMMFilterSocket_MatchPlug_f */
};


