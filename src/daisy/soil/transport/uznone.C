// uznone.C --- no water flow.
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
#include "daisy/soil/soil.h"
#include "util/mathlib.h"
#include "object_model/librarian.h"
#include "object_model/frame.h"

class UZNone : public UZmodel
{
public:
  void tick (Treelog&, const GeometryVert& /* geo */,
             const Soil& /* soil */, const SoilHeat&,
	     const unsigned int first, const Surface& /* top */, 
             const size_t /* top_edge */,
	     const unsigned int last, const Groundwater& /* bottom */, 
             const size_t /* bottom_edge */,
	     const std::vector<double>& /* S */,
	     const std::vector<double>& h_old,
	     const std::vector<double>& Theta_old,
	     const std::vector<double>& /* h_ice */,
	     std::vector<double>& h,
	     std::vector<double>& Theta,
             size_t q_offset,
             std::vector<double>& q_base, 
             double /* dt */)
  {
    for (unsigned int i = first; i <= last; i++)
      {
        q_base[q_offset + i] = 0.0;
        Theta[i] = Theta_old[i];
        h[i] = h_old[i];
      }
    q_base[q_offset + last + 1] = 0.0;
  }

  // Create and Destroy.
  void has_macropores (bool)
  { }
public:
  UZNone (const BlockModel& al)
    : UZmodel (al)
    { }
  ~UZNone ()
    { }
};

// Add the UZNone syntax to the syntax table.
static struct UZNoneSyntax : public DeclareModel
{
  Model* make (const BlockModel& al) const
    {
      return new UZNone (al);
    }

  UZNoneSyntax ()
    : DeclareModel (UZmodel::component, "none", "No water movement, and no sink.")
  { }
  void load_frame (Frame& frame) const
    {
    }
} UZNone_syntax;
