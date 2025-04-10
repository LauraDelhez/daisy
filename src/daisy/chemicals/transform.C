// transform.C --- Transformation between two soil components.
// 
// Copyright 2002 Per Abrahamsen and KVL.
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

#include "daisy/chemicals/transform.h"
#include "object_model/block_model.h"
#include "object_model/librarian.h"

const char *const Transform::component = "transform";

symbol
Transform::library_id () const
{
  static const symbol id (component);
  return id;
}

bool
Transform::check (const Units&, const Geometry&, 
                  const Soil&, const SoilWater&, const SoilHeat&, 
		  Treelog&) const
{ return true; }

void
Transform::initialize (const Units&, const Geometry&, 
                       const Soil&, const SoilWater&, const SoilHeat&, 
                       Treelog&)
{ }

Transform::Transform (const BlockModel&)
{ }

Transform::~Transform ()
{ }

static struct TransformInit : public DeclareComponent 
{
  void load_frame (Frame& frame) const
  { 
    Model::load_model (frame);
  }

  TransformInit ()
    : DeclareComponent (Transform::component, "\
Generic transformations between soil components.")
  { }
} Transform_init;
