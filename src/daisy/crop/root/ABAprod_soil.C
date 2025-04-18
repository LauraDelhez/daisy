// ABAprod_soil.C  -- ABA production based on soil location.
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
#include "util/scope_exchange.h"
#include "daisy/soil/transport/geometry.h"
#include "daisy/soil/soil_water.h"
#include "util/assertion.h"
#include "object_model/librarian.h"
#include "object_model/frame.h"
#include "object_model/block_model.h"
#include "object_model/treelog.h"

struct ABAProdSoil : public ABAProd
{
  // Units.
  static const symbol h_name;
  static const symbol L_name;
  static const symbol S_name;
  static const symbol ABA_unit;

  // Parameters.
  mutable ScopeExchange scope;
  const std::unique_ptr<Number> expr;
  
  // Solve.
  void production (const Geometry&, const SoilWater&,
		   const std::vector<double>& S /* [cm^3/cm^3/h] */,
		   const std::vector<double>& L /* [cm/cm^3] */,
		   std::vector<double>& ABA /* [g/cm^3/h] */,
		   Treelog&) const;
  void output (Log&) const
  { }

  // Create and Destroy.
  void initialize (Treelog&);
  bool check (Treelog&) const;
  ABAProdSoil (const BlockModel& al);
  ~ABAProdSoil ();
};

const symbol 
ABAProdSoil::h_name ("h");

const symbol 
ABAProdSoil::L_name ("L");

const symbol 
ABAProdSoil::S_name ("S");

const symbol 
ABAProdSoil::ABA_unit ("g/cm^3/h");

void
ABAProdSoil::production (const Geometry& geo, const SoilWater& soil_water,
			 const std::vector<double>& S /* [cm^3/cm^3/h] */,
			 const std::vector<double>& L /* [cm/cm^3] */,
			 std::vector<double>& ABA    /* [g/cm^3/h] */,
			 Treelog& msg) const
{
  // Check input.
  const size_t cell_size = geo.cell_size ();
  daisy_assert (ABA.size () == cell_size);
  daisy_assert (L.size () == cell_size);
  daisy_assert (S.size () == cell_size);
  
  // For all cells.
  for (size_t c = 0; c < cell_size; c++)
    {
      // Set up values in scope.
      scope.set (h_name, soil_water.h (c));
      scope.set (L_name, L[c]);
      scope.set (S_name, S[c]);

      // Find expr value.
      double value = 0.0;
      if (!expr->tick_value (units, value, ABA_unit, scope, msg))
	msg.error ("No ABA production value found");

      // Find ABA uptake.
      ABA[c] = value; 		// [g/cm^3 S/h]
    }
}

void 
ABAProdSoil::initialize (Treelog& msg)
{ expr->initialize (units, scope, msg); }

bool 
ABAProdSoil::check (Treelog& msg) const
{
  bool ok = true;
  
  if (!expr->check_dim (units, scope, ABA_unit, msg))
    ok = false;

  return ok;
}

ABAProdSoil::ABAProdSoil (const BlockModel& al)
  : ABAProd (al),
    scope (__FUNCTION__),
    expr (Librarian::build_item<Number> (al, "expr"))
{
  scope.add_item (new ExchangeNumber (h_name, "cm", "Soil water pressure."));
  scope.add_item (new ExchangeNumber (L_name, "cm/cm^3", "Root density."));
  scope.add_item (new ExchangeNumber (S_name, "cm^3/cm^3/h", "Water uptake."));
  scope.done ();
 }

ABAProdSoil::~ABAProdSoil ()
{ }

static struct ABAProdSoilSyntax : public DeclareModel
{
  Model* make (const BlockModel& al) const
  { return new ABAProdSoil (al); }
  ABAProdSoilSyntax ()
    : DeclareModel (ABAProd::component, "soil", "\
ABA production based on soil location.")
  { }
  void load_frame (Frame& frame) const
  {
    frame.declare_object ("expr", Number::component, 
                      Attribute::Const, Attribute::Singleton, "\
Expression to evaluate to ABA uptake [g/cm^3/h].\n\
The symbol 'h' will be bound to the water pressure [cm].\n\
The symbol 'L' will be bound to the root density [cm/cm^3].\n\
The symbol 'S' will be bound to the water uptake [cm^3/cm^3/h].");
  }
} ABAProdSoil_syntax;

// ABAprod_soil.C ends here
