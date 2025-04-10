// volume.C - a subset of 3D space.
// 
// Copyright 2006 Per Abrahamsen and KVL.
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

#include "daisy/soil/transport/volume.h"
#include "daisy/soil/transport/geometry.h"
#include "object_model/block_model.h"
#include "util/assertion.h"
#include "object_model/librarian.h"

class Log;

const char *const Volume::component = "volume";

symbol
Volume::library_id () const
{
  static const symbol id (component);
  return id;
}

const std::vector<double>&
Volume::density (const Geometry& geo) const
{
  density_map::const_iterator i = densities.find (&geo);
  
  if (i != densities.end ())
    return (*i).second;

  const size_t cell_size = geo.cell_size ();
  std::vector<double> result (cell_size);
  for (size_t c = 0; c < cell_size; c++)
    result[c] = geo.fraction_in_volume (c, *this);
  densities[&geo] = result;
  return densities[&geo];
}

std::unique_ptr<Volume>
Volume::build_obsolete (const BlockModel& al)
{
  Volume *const vol = Librarian::build_item<Volume> (al, "volume");
  daisy_assert (vol);
  if (al.check ("from"))
    {
      const double from = al.number ("from");
      if (from < 0)
        vol->limit_top (from);
    }
  if (al.check ("to"))
    {
      const double to = al.number ("to");
      if (to < 0)
        vol->limit_bottom (to);
    }
  return std::unique_ptr<Volume> (vol);
}

Volume::Volume (const BlockModel& al)
  : ModelDerived (al.type_name ())
{ }

Volume::Volume (const char *const id)
  : ModelDerived (id)
{ }

Volume::~Volume ()
{ }

static struct VolumeInit : public DeclareComponent 
{
  VolumeInit ()
    : DeclareComponent (Volume::component, "\
A subset of 3D space.")
  { }
} Volume_init;

// volume.C ends here.
