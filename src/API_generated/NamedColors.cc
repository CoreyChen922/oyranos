/** @file NamedColors.cc

   [Template file inheritance graph]
   +-> NamedColors.template.cc
   |
   +-- Base.cc

 *  Oyranos is an open source Colour Management System
 *
 *  @par Copyright:
 *            2004-2013 (C) Kai-Uwe Behrmann
 *
 *  @author   Kai-Uwe Behrmann <ku.b@gmx.de>
 *  @par License:
 *            new BSD - see: http://www.opensource.org/licenses/bsd-license.php
 *  @date     2013/08/23
 */



#include "NamedColors.hh"

using namespace oy;


NamedColors::NamedColors(Object object)
{
  m_oy = oyNamedColors_New(object);
}

NamedColors::~NamedColors()
{
  oyNamedColors_Release(&m_oy);
}

// Overloaded constructor methods


// Specific NamedColors methods


// Static functions
