/** @file oyCMMapiFilter_s.h

   [Template file inheritance graph]
   +-> oyCMMapiFilter_s.template.h
   |
   +-> oyCMMapi_s.template.h
   |
   +-> Base_s.h
   |
   +-- oyStruct_s.template.h

 *  Oyranos is an open source Color Management System
 *
 *  @par Copyright:
 *            2004-2013 (C) Kai-Uwe Behrmann
 *
 *  @author   Kai-Uwe Behrmann <ku.b@gmx.de>
 *  @par License:
 *            new BSD - see: http://www.opensource.org/licenses/bsd-license.php
 *  @date     2013/08/23
 */



#ifndef OY_CMM_API_FILTER_S_H
#define OY_CMM_API_FILTER_S_H

#ifdef __cplusplus
/*namespace oyranos {*/
  extern "C" {
#endif /* __cplusplus */


  
#include <oyranos_object.h>

typedef struct oyCMMapiFilter_s oyCMMapiFilter_s;


#include "oyStruct_s.h"


/* Include "CMMapiFilter.public.h" { */

/* } Include "CMMapiFilter.public.h" */


/* Include "CMMapiFilter.dox" { */
/** @struct  oyCMMapiFilter_s
 *  @ingroup module_api
 *  @extends oyCMMapi_s
 *  @brief   the module API 4,6,7 interface base
 *  
 *  The registration should provide keywords for selection.
 *  The api5_ member is missed for oyCMMapi5_s.
 *
 *  @version Oyranos: 0.1.10
 *  @since   2008/12/28 (Oyranos: 0.1.10)
 *  @date    2008/12/28
 */

/* } Include "CMMapiFilter.dox" */

struct oyCMMapiFilter_s {
/* Include "Struct.members.h" { */
oyOBJECT_e           type_;          /**< @private struct type */
oyStruct_Copy_f      copy;           /**< copy function */
oyStruct_Release_f   release;        /**< release function */
oyObject_s           oy_;            /**< @private features name and hash */

/* } Include "Struct.members.h" */
};


OYAPI oyCMMapiFilter_s* OYEXPORT
  oyCMMapiFilter_New( oyObject_s object );
OYAPI oyCMMapiFilter_s* OYEXPORT
  oyCMMapiFilter_Copy( oyCMMapiFilter_s *cmmapifilter, oyObject_s obj );
OYAPI int OYEXPORT
  oyCMMapiFilter_Release( oyCMMapiFilter_s **cmmapifilter );



/* Include "CMMapiFilter.public_methods_declarations.h" { */

/* } Include "CMMapiFilter.public_methods_declarations.h" */


#ifdef __cplusplus
} /* extern "C" */
/*}*/ /* namespace oyranos */
#endif /* __cplusplus */

#endif /* OY_CMM_API_FILTER_S_H */
