// model.C -- Base class for all model in Daisy.
// 
// Copyright 2007 Per Abrahamsen and KVL.
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

#include "object_model/model.h"
#include "object_model/frame.h"

// Base class 'Model'

symbol
Model::obsolete ()
{
  static const symbol name ("obsolete");
  return name;
}

void 
Model::declare_obsolete (Frame& frame, const symbol message)
{ frame.declare_text (obsolete (), Attribute::OptionalConst, message); }

void 
Model::load_model (Frame& frame)
{ 
  frame.declare_text ("description", Attribute::OptionalConst, "\
Description of this model or parameterization.\n\
The value will appear in the reference manual, and may also appear in some \
GUI front ends.");
  frame.declare_string ("cite", Attribute::Const, Attribute::Variable, "\
BibTeX keys that would be relevant for this model or parameterization.");
  frame.set_empty ("cite");
}

Model::Model ()
{ }

Model::~Model ()
{ }

// model.C ends here.
