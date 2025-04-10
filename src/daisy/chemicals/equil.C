// equil.C --- Find equilibrium between two soil chemicals.
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


#include "daisy/chemicals/equil.h"
#include "object_model/block_model.h"
#include "object_model/librarian.h"
#include "object_model/frame.h"

const char *const Equilibrium::component = "equilibrium";

symbol
Equilibrium::library_id () const
{
  static const symbol id (component);
  return id;
}

Equilibrium::Equilibrium (const BlockModel& al) // FIXME: Why is al not used?
{ }

Equilibrium::~Equilibrium ()
{ }

static struct EquilibriumInit : public DeclareComponent 
{
  EquilibriumInit ()
    : DeclareComponent (Equilibrium::component, "\
Find equilibrium between two soil chemicals.")
  { }
  void load_frame (Frame& frame) const
  { Model::load_model (frame); }
} Equilibrium_init;
