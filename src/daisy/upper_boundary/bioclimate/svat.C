// svat.C  -- Soil, Vegetation and ATmostphere.
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

#include "daisy/upper_boundary/bioclimate/svat.h"
#include "daisy/output/log.h"
#include "object_model/block_model.h"
#include "object_model/librarian.h"

const char *const SVAT::component = "svat";

symbol
SVAT::library_id () const
{
  static const symbol id (component);
  return id;
}

void
SVAT::output (Log&) const
{ }

bool
SVAT::stable () const
{ return true; }

SVAT::SVAT (const BlockModel& al)
  : ModelDerived (al.type_name ())
{ }

SVAT::~SVAT ()
{ }

static struct SVATInit : public DeclareComponent 
{
  SVATInit ()
    : DeclareComponent (SVAT::component, "\
The task of the 'svat' component is to calculate the production\n\
stress, given the potential evapotranspiration, the actual\n\
evaporation from the surface, meteorological data, and the vegetation\n\
and soil state.")
  { }
  void load_frame (Frame& frame) const
  { Model::load_model (frame); }
} SVAT_init;

// svat.C ends here.
