/** @internal
 *  @file oyranos_cmm.h
 *
 *  Oyranos is an open source Color Management System 
 * 
 *  @par Copyright:
 *            2007-2009 (C) Kai-Uwe Behrmann
 *
 *  @brief external CMM module API
 *  @author   Kai-Uwe Behrmann <ku.b@gmx.de>
 *
 *  @par License:
 *            new BSD <http://www.opensource.org/licenses/bsd-license.php>
 *  @since    2007/11/12
 */


#ifndef OYRANOS_CMM_H
#define OYRANOS_CMM_H

#include "oyFilterPlug_s.h"
#include "oyPixelAccess_s.h"
#include "oyPointer_s.h"
#include "oyStruct_s.h"

#include "oyranos.h"
#include "oyranos_icc.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/** @brief   CMM data to Oyranos cache
 *
 *  @param[in]     data                the data struct know to the module
 *  @param[in,out] oy                  the Oyranos cache struct to fill by the module
 *  @return                            error
 *
 *  @version Oyranos: 0.1.9
 *  @since   2007/11/00 (Oyranos: 0.1.8)
 *  @date    2008/11/06
 */
typedef int      (*oyCMMobjectOpen_f)( oyStruct_s        * data,
                                       oyPointer_s       * oy );

typedef void     (*oyCMMProgress_f)  ( int                 ID,
                                       double              progress );


typedef icSignature (*oyCMMProfile_GetSignature_f) (
                                       oyPointer_s       * cmm_ptr,
                                       int                 pcs);

int              oyCMMlibMatchesCMM  ( const char        * lib_name,
                                       const char        * cmm );

/*oyPointer          oyCMMallocateFunc ( size_t              size );
void               oyCMMdeallocateFunc(oyPointer           mem );*/


typedef const char* (*oyWidgetGet_f) ( uint32_t          * result );

/* -------------------------------------------------------------------------*/

/* implemented filter functions */
int      oyFilterPlug_ImageRootRun   ( oyFilterPlug_s    * requestor_plug,
                                       oyPixelAccess_s   * ticket );

const char * oyCMMgetText            ( const char        * select,
                                       oyNAME_e            type,
                                       oyStruct_s        * context );
extern const char * oyCMM_texts[4];
const char * oyCMMgetImageConnectorPlugText (
                                       const char        * select,
                                       oyNAME_e            type,
                                       oyStruct_s        * context );
const char * oyCMMgetImageConnectorSocketText (
                                       const char        * select,
                                       oyNAME_e            type,
                                       oyStruct_s        * context );
extern const char * oy_image_connector_texts[2];

/* miscellaneous helpers */
int oyCMMWarnFunc( int code, const oyStruct_s * context, const char * format, ... );

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* OYRANOS_CMM_H */
