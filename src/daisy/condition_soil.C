// condition_soil.C --- Checking soil state.
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
#include "daisy/condition.h"
#include "object_model/block_model.h"
#include "object_model/frame.h"
#include "daisy/field.h"
#include "daisy/daisy.h"
#include "object_model/check.h"
#include "object_model/librarian.h"
#include "object_model/treelog.h"

struct ConditionSoilTemperature : public Condition
{
  const double temperature;
  const double height;

  bool match (const Daisy& daisy, const Scope&, Treelog&) const
  { 
    if (daisy.field ().soil_temperature (height) > temperature)
      return true;
    return false;
  }
  void output (Log&) const
  { }

  void tick (const Daisy&, const Scope&, Treelog&)
  { }

  void initialize (const Daisy&, const Scope&, Treelog&)
  { }

  bool check (const Daisy&, const Scope&, Treelog&) const
  { return true; }

  ConditionSoilTemperature (const BlockModel& al)
    : Condition (al),
      temperature (al.number ("temperature")),
      height (al.number ("height"))
  { }
};

struct ConditionSoilPotential : public Condition
{
  const double potential;
  const double height;

  bool match (const Daisy& daisy, const Scope&, Treelog&) const
  { return (daisy.field ().soil_water_potential (height) > potential); }
  void output (Log&) const
  { }

  void tick (const Daisy&, const Scope&, Treelog&)
  { }

  void initialize (const Daisy&, const Scope&, Treelog&)
  { }

  bool check (const Daisy&, const Scope&, Treelog&) const
  { return true; }

  ConditionSoilPotential (const BlockModel& al)
    : Condition (al),
      potential (al.number ("potential")),
      height (al.number ("height"))
  { }
};

struct ConditionSoilWater : public Condition
{
  const double water;		// [mm]
  const double from;		// [cm]
  const double to;		// [cm]

  bool match (const Daisy& daisy, const Scope&, Treelog&) const
  { return (daisy.field ().soil_water_content (from, to) * 10 > water); }
  void output (Log&) const
  { }

  void tick (const Daisy&, const Scope&, Treelog&)
  { }

  void initialize (const Daisy&, const Scope&, Treelog&)
  { }

  bool check (const Daisy&, const Scope&, Treelog&) const
  { return true; }

  ConditionSoilWater (const BlockModel& al)
    : Condition (al),
      water (al.number ("water")),
      from (al.number ("from")),
      to (al.number ("to"))
  { }
};

struct ConditionSoilN_min : public Condition
{
  const double amount;		// [mm]
  const double from;		// [cm]
  const double to;		// [cm]

  bool match (const Daisy& daisy, const Scope&, Treelog&) const
  { return (daisy.field ().soil_inorganic_nitrogen (from, to)  > amount); }
  void output (Log&) const
  { }

  void tick (const Daisy&, const Scope&, Treelog&)
  { }

  void initialize (const Daisy&, const Scope&, Treelog&)
  { }

  bool check (const Daisy&, const Scope&, Treelog&) const
  { return true; }

  ConditionSoilN_min (const BlockModel& al)
    : Condition (al),
      amount (al.number ("amount")),
      from (al.number ("from")),
      to (al.number ("to"))
  { }
};

static struct ConditionSoilTemperatureSyntax : public DeclareModel
{
  Model* make (const BlockModel& al) const
  { return new ConditionSoilTemperature (al); }
  ConditionSoilTemperatureSyntax ()
    : DeclareModel (Condition::component, "soil_temperature_above", "\
Test if the soil is warmer than the specified temperature.")
  { }
  void load_frame (Frame& frame) const
  {
    frame.declare ("temperature", "dg C", Attribute::Const, "\
Lowest soil temperature for which the condition is true.");
    frame.declare ("height", "cm", Check::non_positive (), Attribute::Const, "\
Soil depth in which to test the temperature.");
  }
} ConditionSoilTemperature_syntax;

static struct ConditionSoilPotentialSyntax : public DeclareModel
{
  Model* make (const BlockModel& al) const
  { return new ConditionSoilPotential (al); }
  ConditionSoilPotentialSyntax ()
    : DeclareModel (Condition::component, "soil_water_pressure_above", "\
Test if the soil is wetter than the specified pressure potential.")
  { }
  void load_frame (Frame& frame) const
  {
    frame.declare ("potential", "cm", Attribute::Const, "\
The soil should be wetter than this for the condition to be true.");
    frame.declare ("height", "cm", Check::non_positive (), Attribute::Const, "\
Depth at which to example the pressure potential.");
  }
} ConditionSoilPotential_syntax;

static bool check_water_content (const Metalib&, const Frame& al, Treelog& err)
{
  bool ok = true;

  const double from = al.number ("from");
  const double to = al.number ("to");
  if (from < to)
    {
      err.entry ("'from' must be higher than 'to' in"
                 " the measured area");
      ok = false;
    }
  return ok;
}

static struct ConditionSoilWaterSyntax : public DeclareModel
{
  Model* make (const BlockModel& al) const
  { return new ConditionSoilWater (al); }
  ConditionSoilWaterSyntax ()
    : DeclareModel (Condition::component, "soil_water_content_above", "\
Test if the soil contains more water than the specified amount.")
  { }
  void load_frame (Frame& frame) const
  {
    frame.add_check (check_water_content);
    frame.declare ("water", "mm", Check::non_negative (), Attribute::Const, "\
The soil should contain more water than this for the condition to be true.");
    frame.declare ("from", "cm", Check::non_positive (), Attribute::Const, "\
Top of interval to measure soil water content in.");
    frame.set ("from", 0.0);
    frame.declare ("to", "cm", Check::non_positive (), Attribute::Const, "\
Bottom of interval to measure soil water content in.");
    frame.order ("water");
  }
} ConditionSoilWater_syntax;

static struct ConditionSoilN_minSyntax : public DeclareModel
{
  Model* make (const BlockModel& al) const
  { return new ConditionSoilN_min (al); }
  ConditionSoilN_minSyntax ()
    : DeclareModel (Condition::component, "soil_inorganic_N_above", "\
Test if the soil contains more mineral nitrogen than the specified amount.")
  { }
  void load_frame (Frame& frame) const
  {
    frame.add_check (check_water_content);
    frame.declare ("amount", "kg N/ha",
               Check::non_negative (), Attribute::Const, "\
The soil should contain more inorganic nitrogen than this for\n\
the condition to be true.");
    frame.declare ("from", "cm", Check::non_positive (), Attribute::Const, "\
Top of interval to measure soil content in.");
    frame.set ("from", 0.0);
    frame.declare ("to", "cm", Check::non_positive (), Attribute::Const, "\
Bottom of interval to measure soil content in.");
    frame.order ("amount");
  }
} ConditionSoilN_min_syntax;

// condition_soil.C ends here.
