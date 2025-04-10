// hydraulic_mod_C.C
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
//
// Modified Campbell retention curve model with Burdine theory.

#define BUILD_DLL

#include "daisy/soil/hydraulic.h"
#include "object_model/block_model.h"
#include "object_model/check.h"
#include "util/mathlib.h"
#include "object_model/librarian.h"
#include "object_model/frame.h"

class Hydraulic_mod_C : public Hydraulic
{
  // Content.
  const double h_b;
  const double b;

  // Use.
public:
  double Theta (double h) const;
  double K (double h) const;
  double Cw2 (double h) const;
  double h (double Theta) const;
  double M (double h) const;
private:
  double Sr (double h) const;
  
  // Create and Destroy.
public:
  Hydraulic_mod_C (const BlockModel&);
  ~Hydraulic_mod_C ();
};

double 
Hydraulic_mod_C::Theta (const double h) const
{
  return Sr (h) * Theta_sat;
}

double 
Hydraulic_mod_C::K (const double h) const
{
  return K_sat * pow (Sr (h), (2 + 3.0 / b) * b);
}

double 
Hydraulic_mod_C::Cw2 (const double h) const
{
  if (h < 0.0)
    {
      return -(Theta_sat 
	       * pow (h / h_b, 4.) 
	       * pow (1. / (pow (h / h_b, 5.) + 1.), 0.2 / b - 1) 
	       / (b * h_b * pow (pow (h / h_b, 5.) + 1., 2)));
    }
  else
    return 0.0;
}

double 
Hydraulic_mod_C::h (const double Theta) const
{
  if (Theta < Theta_sat)
    return h_b * pow (pow (Theta / Theta_sat, -5.0 * b) - 1.0, 1.0/5.0);
  else
    return 0.0;
}

double 
Hydraulic_mod_C::M (double h) const
{
  if (h <= h_b)
    return K_sat * (-h_b / (1.0 + 3.0 / b)) * pow (h_b / h, 1.0 + 3.0 / b);
  else
    return M (h_b) + K_sat * (h - h_b);
}

double 
Hydraulic_mod_C::Sr (double h) const
{
  if (h < 0.0)
    return pow (1.0 / (1.0 + pow (h / h_b, 5.0)), 1.0 / (b * 5.0));
  else
    return 1;
}

Hydraulic_mod_C::Hydraulic_mod_C (const BlockModel& al)
  : Hydraulic (al),
    h_b (al.number ("h_b")),
    b (al.number ("b"))
{ }

Hydraulic_mod_C::~Hydraulic_mod_C ()
{ }

// Add the Hydraulic_mod_C syntax to the syntax table.

static struct Hydraulic_mod_CSyntax : public DeclareModel
{
  Model* make (const BlockModel& al) const
  { return new Hydraulic_mod_C (al); }

  Hydraulic_mod_CSyntax ()
    : DeclareModel (Hydraulic::component, "mod_C", "\
Modified Campbell retention curve model with Burdine theory.")
  { }
  void load_frame (Frame& frame) const
  { 
    Hydraulic::load_Theta_sat (frame);
    Hydraulic::load_K_sat (frame);
    frame.declare ("h_b", "cm", Check::negative (), Attribute::Const,
                "Bubbling pressure.");
    frame.declare ("b", Attribute::None (), Check::positive (), Attribute::Const,
                "Campbell parameter.");

  }
} hydraulic_mod_C_syntax;
