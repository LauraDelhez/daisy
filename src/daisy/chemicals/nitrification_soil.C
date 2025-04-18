// nitrification_soil.C
// 
// Copyright 1996-2001 Per Abrahamsen and SÃ¸ren Hansen
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

#include "daisy/chemicals/nitrification.h"
#include "daisy/soil/abiotic.h"
#include "object_model/block_model.h"
#include "util/mathlib.h"
#include "object_model/plf.h"
#include "object_model/check.h"
#include "object_model/librarian.h"
#include "object_model/frame_model.h"
#include "object_model/intrinsics.h"
#include "object_model/library.h"

class NitrificationSoil : public Nitrification
{
  // Parameters.
private: 
  const double k;
  const double k_10;
  const PLF heat_factor;
  const PLF water_factor;

  // Simulation.
public:
  void tick (const double M, const double C, const double Theta, 
             const double h, const double T,
             double& NH4, double& N2O, double& NO3) const;

  // Create.
public:
  NitrificationSoil (const BlockModel&);
  NitrificationSoil (const Frame&);
};

void 
NitrificationSoil::tick (const double M, const double /* C */, 
                         const double /* Theta */,
			 const double h, const double T,
                         double& NH4, double& N2O, double& NO3) const
{
  const double T_factor = (heat_factor.size () < 1)
    ? Abiotic::f_T2 (T)
    : heat_factor (T);
  const double w_factor = (water_factor.size () < 1)
    ? f_h (h)
    : water_factor (h);

  const double rate = k_10 * w_factor * T_factor * M / (k + M);
  daisy_assert (rate >= 0.0);
  const double M_new = rate;
  if (M_new > 0.0)
    {
      NH4 = M_new;
      N2O = M_new * N2O_fraction;
      NO3 = NH4 - N2O;
    }
  else
    NH4 = N2O = NO3 = 0.0;
}

NitrificationSoil::NitrificationSoil (const BlockModel& al)
  : Nitrification (al),
    k (al.number ("k")),
    k_10 (al.number ("k_10")),
    heat_factor (al.plf ("heat_factor")),
    water_factor (al.plf ("water_factor"))
{ }

NitrificationSoil::NitrificationSoil (const Frame& al)
  : Nitrification (al),
    k (al.number ("k")),
    k_10 (al.number ("k_10")),
    heat_factor (al.plf ("heat_factor")),
    water_factor (al.plf ("water_factor"))
{ }

static struct NitrificationSoilSyntax : public DeclareModel
{
  Model* make (const BlockModel& al) const
  { return new NitrificationSoil (al); }
  NitrificationSoilSyntax ()
    : DeclareModel (Nitrification::component, "soil", 
               "k_10 * M / (k + M).  Michaelis-Menten kinetics,\n\
with nitrification based on total ammonium content.")
  { }
  void load_frame (Frame& frame) const
  {
    frame.declare ("k", "g N/cm^3", Check::positive (), Attribute::Const, 
                "Half saturation constant.");
    frame.set ("k", 5.0e-5); // [g N/cm^3]
    frame.declare ("k_10", "g N/cm^3/h", Check::non_negative (), Attribute::Const,
                "Max rate.");
    frame.set ("k_10", 2.08333333333e-7); // 5e-6/24 [1/h]
    frame.declare ("heat_factor", "dg C", Attribute::None (), Attribute::Const,
                "Heat factor.");
    frame.set ("heat_factor", PLF::empty ());
    frame.declare ("water_factor", "cm", Attribute::None (), Attribute::Const,
                "Water potential factor.");
    frame.set ("water_factor", PLF::empty ());
  }
} NitrificationSoil_syntax;

std::unique_ptr<Nitrification> 
Nitrification::create_default ()
{
  const Library& library = Librarian::intrinsics ().library (component);
  daisy_assert (library.check ("soil"));
  const FrameModel& frame = library.model ("soil");
  return std::unique_ptr<Nitrification> (new NitrificationSoil (frame));
}

// nitrification_soil.C ends here.
