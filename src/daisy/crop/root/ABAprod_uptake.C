// ABAprod_uptake.C  -- ABA production based on water uptake.
// 
// Copyright 2007 Per Abrahamsen and KVL.
//
// This file is part of Daisy.
// 
// Daisy is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser Public License as published by
// the Free Software Foundation; either version 2.1 of the License, or
// (at your option) any later version.5
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

#include "daisy/crop/root/ABAprod.h"
#include "object_model/parameter_types/number.h"
#include "util/scope_id.h"
#include "daisy/soil/transport/geometry.h"
#include "daisy/soil/soil_water.h"
#include "object_model/units.h"
#include "util/assertion.h"
#include "object_model/librarian.h"
#include "object_model/frame.h"
#include "object_model/treelog.h"
#include "util/mathlib.h"
#include "object_model/block_model.h"
#include <sstream>

struct ABAProdUptake : public ABAProd
{
  // Units.
  static const symbol h_name;
  static const symbol ABA_unit;

  // Parameters.
  mutable ScopeID scope;
  const std::unique_ptr<Number> expr;
  
  // Solve.
  void production (const Geometry&, const SoilWater&,
		   const std::vector<double>& S /* [cm^3/cm^3/h] */,
		   const std::vector<double>& l /* [cm/cm^3] */,
		   std::vector<double>& ABA /* [g/cm^3/h] */,
		   Treelog&) const;
  void output (Log&) const
  { }

  // Create and Destroy.
  void initialize (Treelog&);
  bool check (Treelog&) const;
  ABAProdUptake (const BlockModel& al);
  ~ABAProdUptake ();
};

const symbol 
ABAProdUptake::h_name ("h");

const symbol 
ABAProdUptake::ABA_unit ("g/cm^3");

void
ABAProdUptake::production (const Geometry& geo, const SoilWater& soil_water,
                           const std::vector<double>& S /* [cm^3/cm^3/h] */,
                           const std::vector<double>& /* l [cm/cm^3] */,
                           std::vector<double>& ABA    /* [g/cm^3/h] */,
                           Treelog& msg) const
{
  // Check input.
  const size_t cell_size = geo.cell_size ();
  daisy_assert (ABA.size () == cell_size);
  daisy_assert (S.size () == cell_size);
  
  // For all cells.
  for (size_t c = 0; c < cell_size; c++)
    {
      const double h = soil_water.h (c);
      // Set up 'h' in scope.
      scope.set (h_name, soil_water.h (c));

      // Find soil value.
      double value = 0.0;
      if (!expr->tick_value (units, value, ABA_unit, scope, msg))
	msg.error ("No ABA production value found");
      if (!std::isfinite (value) || value < 0.0)
        {
          if (h > -14000) // We are not concerned with ABA below wilting point.
            {
              std::ostringstream tmp;
              tmp << "ABA in cell " << c << " with h = " << h
                  << " was " << value << " [" << ABA_unit << "], using 0 instead";
              msg.warning (tmp.str ());
            }
          value = 0.0;
        }
      daisy_assert (std::isfinite (S[c]));

      // Find ABA uptake.
      ABA[c] = value * S[c];
      // [g/cm^3 S/h] = [g/cm^3 W] * [cm^3 W/cm^3 S/h]
      daisy_assert (std::isfinite (ABA[c]));
    }
}

void 
ABAProdUptake::initialize (Treelog& msg)
{ expr->initialize (units, scope, msg); }

bool 
ABAProdUptake::check (Treelog& msg) const
{
  bool ok = true;

  if (!expr->check_dim (units, scope, ABA_unit, msg))
    ok = false;

  return ok;
}

ABAProdUptake::ABAProdUptake (const BlockModel& al)
  : ABAProd (al),
    scope (h_name, Units::cm ()),
    expr (Librarian::build_item<Number> (al, "expr"))
{ }

ABAProdUptake::~ABAProdUptake ()
{ }

static struct ABAProdUptakeSyntax : DeclareModel
{
  Model* make (const BlockModel& al) const
  { return new ABAProdUptake (al); }
  ABAProdUptakeSyntax ()
    : DeclareModel (ABAProd::component, "uptake", "\
ABA production based on concentration in water uptake.\n\
\n\
The assumption is water uptake from roots in specific region of the soil\n\
comes with a specific ABA concentration, which depends solely on the\n\
water pressure in that region.")
  { }
  void load_frame (Frame& frame) const
  {
    frame.declare_object ("expr", Number::component, 
                      Attribute::Const, Attribute::Singleton, "\
Expression to evaluate to ABA concentration in water uptake [g/cm^3].\n\
The symbol 'h' will be bound to the water pressure [cm].");
  }
} ABAProdUptake_syntax;

// ABAprod_uptake.C ends here

