// tortuosity_M_Q.C --- Millington-Quirk
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

#include "daisy/soil/tortuosity.h"
#include "daisy/soil/hydraulic.h"
#include "util/mathlib.h"
#include "object_model/librarian.h"
#include "object_model/frame.h"

class TortuosityM_Q : public Tortuosity
{
  // Simulation.
public:
  double factor (const Hydraulic& hydraulic, double Theta) const
  {
    const double n = hydraulic.Theta (0.0);
    return pow (Theta, 7.0 / 3.0) / (n * n); // Tortuosity factor []
  }

  // Create.
public:
  TortuosityM_Q (const BlockModel& al)
    : Tortuosity (al)
  { }
  TortuosityM_Q ()
    : Tortuosity ("M_Q")
  { }
};

std::unique_ptr<Tortuosity>
Tortuosity::create_default ()
{ return std::unique_ptr<Tortuosity> (new TortuosityM_Q ());  }

static struct TortuosityM_QSyntax : public DeclareModel
{
  Model* make (const BlockModel& al) const
  { return new TortuosityM_Q (al); }
  TortuosityM_QSyntax ()
    : DeclareModel (Tortuosity::component, "M_Q", "\
Millington-Quirk.  Theta^(7/3) / Theta_sat^2.")
  { }
  void load_frame (Frame&) const
  { }
} TortuosityM_Q_syntax;

// tortuosity_M_Q.C ends here.
