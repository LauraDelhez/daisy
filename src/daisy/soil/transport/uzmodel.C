// uzmodel.C
// 
// Copyright 1996-2001 Per Abrahamsen and Søren Hansen
// Copyright 2000-2001 KVL.
//
// This file is part of Daisy.
// 
// Daisy is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser Public License as published by
// the Free Software Foundation; either version 2.1 of the License, or
// (at your option) any later version.
// 
// Daisy is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser Public License for more details.
// 
// You should have received a copy of the GNU Lesser Public License
// along with Daisy; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#define BUILD_DLL

#include "daisy/soil/transport/uzmodel.h"
#include "object_model/block_model.h"
#include "object_model/librarian.h"

UZmodel::UZmodel (const BlockModel& al)
  : name (al.type_name ())
{ }

UZmodel::~UZmodel ()
{ }

const char *const UZmodel::component = "uzmodel";

symbol
UZmodel::library_id () const
{
  static const symbol id (component);
  return id;
}

static struct UZmodelInit : public DeclareComponent 
{
  UZmodelInit ()
    : DeclareComponent (UZmodel::component, "\
The 'uzmodel' component handles the vertical water movement in the\n\
unsaturated zone soil matrix.")
  { }
} UZmodel_init;

