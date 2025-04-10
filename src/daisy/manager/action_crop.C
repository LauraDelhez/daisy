// action_crop.C
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

#include "daisy/manager/action.h"
#include "daisy/daisy.h"
#include "daisy/field.h"
#include "daisy/crop/crop.h"
#include "daisy/daisy_time.h"
#include "daisy/organic_matter/am.h"
#include "daisy/output/log.h"
#include "daisy/output/harvest.h"
#include "object_model/check_range.h"
#include "daisy/chemicals/im.h"
#include "object_model/submodeler.h"
#include "object_model/vcheck.h"
#include "util/memutils.h"
#include "util/mathlib.h"
#include "object_model/librarian.h"
#include "daisy/upper_boundary/vegetation/vegetation.h"
#include "object_model/units.h"
#include "object_model/treelog.h"
#include "object_model/frame_model.h"
#include "object_model/frame_submodel.h"
#include <sstream>

struct ActionCrop : public Action
{
  const Metalib& metalib;

  // Submodules.
  struct MM_DD			// Dates.
  {
    // Parameters.
    const int month;
    const int day;
    const int hour;

    // Simulation.
    bool match (const Time& time) const;
    
    // Create and Destroy.
    static bool check_alist (const Metalib&, const Frame& al, Treelog&);
    static void load_syntax (Frame&);
    MM_DD (const FrameSubmodel&);
    ~MM_DD ();
  };

  struct Sow			// Sowing.
  {
    // Parameters.
    const MM_DD date;
    const std::unique_ptr<const FrameModel> crop;
    
    // State.
    bool done;
  
    // Simulation.
    void doIt (const Metalib&, Daisy&, const Scope&, Treelog&);
    void output (Log&) const;

    // Create and Destroy.
    static bool check_alist (const Metalib&, const Frame& al, Treelog&);
    static void load_syntax (Frame&);
    Sow (const FrameSubmodel&);
    ~Sow ();
  };
  Sow *const primary;
  Sow *const secondary;

  struct Annual			// Annual harvest.
  {
    // Parameters.
    MM_DD latest;
    const double loss;
    const bool remove_residuals;

    // State.
    bool done;
  
    // Simulation.
    bool doIt (Daisy&, const Scope&, Treelog&, symbol name);
    void output (Log&) const;

    // Create and Destroy.
    static bool check_alist (const Metalib&, const Frame& al, Treelog&);
    static void load_syntax (Frame&);
    Annual (const FrameSubmodel&);
    ~Annual ();
  };
  Annual *const harvest_annual;

  struct Perennial		// Perinnial harvest.
  {
    // Content.
    const int seasons;
    MM_DD end;
    const double DS;
    const double DM;
    int year_of_last_harvest;
    const std::vector<boost::shared_ptr<const FrameModel>/**/> *const fertilize;
    int fertilize_index;
    const std::vector<boost::shared_ptr<const FrameModel>/**/> *const fertilize_rest;
    int fertilize_rest_index;
    int fertilize_year;

    // Simulation.
  private:
    void harvest (Daisy&, Treelog&);
  public:
    bool doIt (Daisy&, const Scope&, Treelog&, symbol name);
    bool doIt (Daisy&, const Scope&, Treelog&, 
               symbol primary, symbol secondary);
    bool done (const Daisy&, Treelog&) const;
    void output (Log&) const;

    // Create and Destroy.
    static bool check_alist (const Metalib&, const Frame& al, Treelog&);
    static void load_syntax (Frame&);
    Perennial (const FrameSubmodel&);
    ~Perennial ();
  };
  Perennial *const harvest_perennial;

  struct Fertilize		// Fertilizing.
  {
    const int month;
    const int day;
    const std::unique_ptr<const FrameModel> what;
    
    static bool check_alist (const Metalib&, const Frame& al, Treelog&);
    static void load_syntax (Frame&);
    Fertilize (const Block&);
    ~Fertilize ();
  };
  const std::vector <const Fertilize*> fertilize_at;
  int fertilize_at_index;
  const bool fertilize_incorporate;
  void fertilize (const Metalib&, Daisy&, Treelog&, const FrameModel& am) const;

  struct Tillage		// Tillage operations.
  {
    // Content.
    const int month;
    const int day;
    std::unique_ptr<Action> operation;

    // Simulation.
    void output (Log&) const;

    // Create and Destroy.
    static bool check_alist (const Metalib&, const Frame& al, Treelog&);
    static void load_syntax (Frame&);
    Tillage (const Block&);
    ~Tillage ();
  };
  const std::vector<const Tillage*> tillage;
  int tillage_index;

  struct Spray		// Spray operations.
  {
    // Content.
    const int month;
    const int day;
    const symbol name;
    const double amount;
    
    // Simulation.
    void output (Log&) const;

    // Create and Destroy.
    static bool check_alist (const Metalib&, const Frame& al, Treelog&);
    static void load_syntax (Frame&);
    Spray (const Frame&);
    ~Spray ();
  };
  const std::vector<const Spray*> spray;
  int spray_index;

  struct Irrigation		// Irrigation.
  {
    // Content.
    const MM_DD from;
    const MM_DD to;
    const double flux;         // Application speed [mm/h]
    const double amount;
    const double potential;

    // Simulation.
    bool doIt (Daisy&, const Scope&, Treelog&) const;

    // Create and Destroy.
    static void load_syntax (Frame&);
    Irrigation (const FrameSubmodel&);
    ~Irrigation ();
  };
  const Irrigation *const irrigation;
  const Irrigation *const irrigation_rest;
  int irrigation_year;
  int irrigation_delay;

  // Simulation.
  void tick (const Daisy&, const Scope&, Treelog&);
  void doIt (Daisy&, const Scope&, Treelog&);
  bool done (const Daisy&, const Scope&, Treelog&) const;
  void output (Log&) const;

  // Create and Destroy.
  void initialize (const Daisy&, const Scope&, Treelog&);
  bool check (const Daisy&, const Scope&, Treelog& err) const;
  ActionCrop (const BlockModel& al);
  ~ActionCrop ();
};

bool 
ActionCrop::MM_DD::match (const Time& time) const
{ return time.month () == month 
    && time.mday () == day 
    && time.hour () == hour; }

bool 
ActionCrop::MM_DD::check_alist (const Metalib&, const Frame& frame, Treelog& err)
{
  bool ok = true;

  const int mm = frame.integer ("month");
  const int dd = frame.integer ("day");
  const int hh = frame.integer ("hour");

  if (mm < 1 || mm > 12)
    {
      err.entry ("month should be between 1 and 12");
      ok = false;
    }
  // don't test for bad month.
  else if (dd < 1 || dd > Time::month_length (1 /* not a leap year */, mm))
    {
      std::ostringstream tmp;
      tmp << "day should be between 1 and " << Time::month_length (1, mm);
      err.entry (tmp.str ());
      ok = false;
    }
  if (hh < 0 || hh > 23)
    {
      err.entry ("hour should be between 0 and 23");
      ok = false;
    }
  return ok;
}

void 
ActionCrop::MM_DD::load_syntax (Frame& frame)
{
  frame.add_check (check_alist);
  frame.declare_integer ("month", Attribute::Const, 
	      "Month in the year.");
  frame.declare_integer ("day", Attribute::Const, 
	      "Day in the month.");
  frame.declare_integer ("hour", Attribute::Const, 
	      "Hour in the day.");
  frame.set ("hour", 8);
  frame.order ("month", "day");
}

ActionCrop::MM_DD::MM_DD (const FrameSubmodel& al)
  : month (al.integer ("month")),
    day (al.integer ("day")),
    hour (al.integer ("hour"))
{ }

ActionCrop::MM_DD::~MM_DD ()
{ }

void 
ActionCrop::Sow::doIt (const Metalib& metalib, Daisy& daisy,
                       const Scope&, Treelog& msg)
{
  if (!done && date.match (daisy.time ()))
    {
      msg.message ("Sowing " + crop->type_name ());      
      daisy.field ().sow (metalib, *crop, 0.0, 0.0, -42.42e42,
                        daisy.time (), msg); 
      done = true;
    }
}
void 
ActionCrop::Sow::output (Log& log) const
{
  output_variable (done, log);
}

bool 
ActionCrop::Sow::check_alist (const Metalib&, const Frame&, Treelog&)
{
  bool ok = true;
  return ok;
}

void 
ActionCrop::Sow::load_syntax (Frame& frame)
{ 
  frame.add_check (check_alist);
  frame.declare_submodule ("date", Attribute::Const, "Date to sow.",
                       MM_DD::load_syntax);
  frame.declare_object ("crop", Crop::component, "Crop to sow.");
  frame.declare_boolean ("done", Attribute::State, 
	      "True iff the crop has been sowed.");
  frame.set ("done", false);
}

ActionCrop::Sow::Sow (const FrameSubmodel& al)
  : date (al.submodel ("date")),
    crop (&al.model ("crop")),
    done (al.flag ("done"))
{ }

ActionCrop::Sow::~Sow ()
{ }

bool
ActionCrop::Annual::doIt (Daisy& daisy,
                          const Scope&, Treelog& msg, symbol name)
{
  if (!done && (daisy.field ().crop_ds (name) >= 2.0
		|| latest.match (daisy.time ())))
    {
      const double stub = 8.0;
      const double stem = remove_residuals ? 1.0 : 0.0;
      const double leaf = remove_residuals ? 1.0 : 0.0;
      const double sorg = (1.0 - loss);
      daisy.field ().harvest (daisy.time (), 
                              Vegetation::all_crops (), stub, stem, leaf, sorg, 
                              false, daisy.harvest (), msg);
      msg.message ("Annual harvest of " + name);
      done = true;
      return true;
    }
  return false;
}

void 
ActionCrop::Annual::output (Log& log) const
{
  output_variable (done, log);
}

bool 
ActionCrop::Annual::check_alist (const Metalib&, const Frame&, Treelog&)
{
  bool ok = true;
  return ok;
}

void 
ActionCrop::Annual::load_syntax (Frame& frame)
{ 
  frame.add_check (check_alist);
  frame.declare_fraction ("loss", Attribute::Const, "Fraction lost during harvest.");
  frame.declare_boolean ("remove_residuals", Attribute::Const,
	      "Remove residuals at harvest.");
  frame.declare_submodule ("latest", Attribute::Const, 
			"Latest harvest date.\n\
If the crop is ripe before this date, it will be harvested at that point.",
			MM_DD::load_syntax);
  frame.declare_boolean ("done", Attribute::State, 
	      "True iff the crop has been sowed.");
  frame.set ("done", false);
}

ActionCrop::Annual::Annual (const FrameSubmodel& al)
  : latest (al.submodel ("latest")),
    loss (al.number ("loss")),
    remove_residuals (al.flag ("remove_residuals")),
    done (al.flag ("done"))
{ }

ActionCrop::Annual::~Annual ()
{ }

void
ActionCrop::Perennial::harvest (Daisy& daisy, Treelog& msg)
{
  const double stub = 8.0;
  const double stem = 1.0;
  const double leaf = 1.0;
  const double sorg = 1.0;
  daisy.field ().harvest (daisy.time (), 
                          Vegetation::all_crops (), stub, stem, leaf, sorg, 
                          false, daisy.harvest (), msg);
  msg.message ("Perennial harvest");
}

bool
ActionCrop::Perennial::doIt (Daisy& daisy,
                             const Scope&, Treelog& msg, symbol name)
{
  const double stub = 8.0;

  if (year_of_last_harvest < 0)
    year_of_last_harvest = daisy.time ().year () + seasons - 1;

  if (daisy.field ().crop_ds (name) >= DS 
      || daisy.field ().crop_dm (name, stub) >= DM)
    {
      harvest (daisy, msg);
      return true;
    }
  return false;
}

bool
ActionCrop::Perennial::doIt (Daisy& daisy, const Scope&, Treelog& msg,
			     symbol primary, symbol secondary)
{
  const double stub = 8.0;

  if (year_of_last_harvest < 0)
    year_of_last_harvest = daisy.time ().year () + seasons - 1;

  if (daisy.field ().crop_ds (primary) >= DS 
      || daisy.field ().crop_ds (secondary) >= DS 
      || (daisy.field ().crop_dm (primary, stub)
	  + daisy.field ().crop_dm (secondary, stub)) >= DM)
    {
      harvest (daisy, msg);
      return true;
    }
  return false;
}

bool
ActionCrop::Perennial::done (const Daisy& daisy, Treelog&) const
{ 
  if (year_of_last_harvest < 0)
    return false;

  daisy_assert (daisy.time ().year () <= year_of_last_harvest);
  return daisy.time ().year () == year_of_last_harvest && end.match (daisy.time ());
}

void 
ActionCrop::Perennial::output (Log& log) const
{
  if (year_of_last_harvest >= 0)
    output_variable (year_of_last_harvest, log);
  output_variable (fertilize_index, log);
  output_variable (fertilize_rest_index, log);
  if (fertilize_year >= 0)
    output_variable (fertilize_year, log);
}

bool 
ActionCrop::Perennial::check_alist (const Metalib&, const Frame& al, Treelog& err)
{
  bool ok = true;

  if (al.integer ("seasons") < 1)
    {
      err.entry ("Perennial harvest should last at least 1 season");
      ok = false;
    }
  if (al.check ("fertilize_rest") && !al.check ("fertilize"))
    {
      err.entry ("'fertilize_rest' specified, but 'fertilize' isn't");
      ok = false;
    }
  
  return ok;
}

void 
ActionCrop::Perennial::load_syntax (Frame& frame)
{ 
  frame.add_check (check_alist);
  frame.declare_integer ("seasons", Attribute::Const, 
	      "Number of seasons to harvest crop.\n\
The crop will be harvested whenever the specified DS or DM are reached.\n\
The first season is the year the crop management starts.");
  frame.declare_submodule ("end", Attribute::Const, 
			"End management this date the last season.",
			MM_DD::load_syntax);
  static RangeEI ds_range (0.0, 2.0);
  frame.declare ("DS", Attribute::None (), ds_range, Attribute::Const, 
	      "Development stage at or above which to initiate harvest.");
  frame.declare ("DM", "kg DM/ha", Check::positive (), Attribute::Const, 
	      "Dry matter at or above which to initiate harvest.");
  frame.declare_integer ("year_of_last_harvest", Attribute::OptionalState, 
	      "Year of last season.");
  frame.declare_object ("fertilize", AM::component,
                        Attribute::OptionalConst, Attribute::Variable, "\
Fertilizer applications after harvest first season.\n\
First season is defined as the year where the first harvest occurs.");
  frame.declare_integer ("fertilize_index", Attribute::State,
	      "Next entry in 'fertilize' to execute.");
  frame.set ("fertilize_index", 0);
  frame.declare_object ("fertilize_rest", AM::component,
                     Attribute::OptionalConst, Attribute::Variable,"\
Fertilizer applications after harvest remaining seasons.\n\
If missing, use the same fertilizer as first season.");
  frame.declare_integer ("fertilize_rest_index", Attribute::State,
	      "Next entry in 'fertilize_rest' to execute.");
  frame.set ("fertilize_rest_index", 0);
  frame.declare_integer ("fertilize_year", Attribute::OptionalState, 
	      "Year last fertilization was applid.");
}

ActionCrop::Perennial::Perennial (const FrameSubmodel& al)
  : seasons (al.integer ("seasons")),
    end (al.submodel ("end")),
    DS (al.number ("DS")),
    DM (al.number ("DM")),
    year_of_last_harvest (al.check ("year_of_last_harvest")
			  ? al.integer ("year_of_last_harvest")
			  : -1),
    fertilize (al.check ("fertilize")
	       ? &al.model_sequence ("fertilize")
	       : NULL),
    fertilize_index (al.integer ("fertilize_index")),
    fertilize_rest (al.check ("fertilize_rest")
		    ? &al.model_sequence ("fertilize_rest")
		    : NULL),
    fertilize_rest_index (al.integer ("fertilize_rest_index")),
    fertilize_year (al.check ("fertilize_year")
		    ? al.integer ("fertilize_year")
		    : -1)
{ }

ActionCrop::Perennial::~Perennial ()
{ }

bool 
ActionCrop::Fertilize::check_alist (const Metalib&, const Frame& al, Treelog& err)
{
  bool ok = true;
  const int mm = al.integer ("month");
  const int dd = al.integer ("day");

  if (mm < 1 || mm > 12)
    {
      err.entry ("month should be between 1 and 12");
      ok = false;
    }
  // don't test for bad month.
  else if (dd < 1 || dd > Time::month_length (1 /* not a leap year */, mm))
    {
      std::ostringstream tmp;
      tmp << "day should be between 1 and " << Time::month_length (1, mm);
      err.entry (tmp.str ());
      ok = false;
    }
  return ok;
}

void 
ActionCrop::Fertilize::load_syntax (Frame& frame)
{ 
  frame.add_check (check_alist);
  frame.declare_integer ("month", Attribute::Const, 
	      "Month in the year.");
  frame.declare_integer ("day", Attribute::Const, 
	      "Day in the month.");
  frame.declare_object ("what", AM::component, "Fertilizer to apply.");
  frame.order ("month", "day", "what");
}

ActionCrop::Fertilize::Fertilize (const Block& al)
  : month (al.integer ("month")),
    day (al.integer ("day")),
    what (&al.model ("what"))
{ }

ActionCrop::Fertilize::~Fertilize ()
{ }

void
ActionCrop::fertilize (const Metalib& metalib, Daisy& daisy, Treelog& msg,
		       const FrameModel& am) const
{
  msg.message (std::string ("[Fertilizing ") + am.type_name () + "]");

  const double from = 0.0;
  const double to = -18.0;
      
  if (fertilize_incorporate)
    daisy.field ().fertilize (metalib, am, from, to, 
                            daisy.time (), msg);
  else
    daisy.field ().fertilize (metalib, am, daisy.time (), msg);
}

void 
ActionCrop::Tillage::output (Log& log) const
{
  output_object (operation, "operation", log);
}

bool 
ActionCrop::Tillage::check_alist (const Metalib&, const Frame& al, Treelog& err)
{
  bool ok = true;
  const int mm = al.integer ("month");
  const int dd = al.integer ("day");

  if (dd > Time::month_length (1 /* not a leap year */, mm))
    {
      std::ostringstream tmp;
      tmp << "day should be between 1 and " << Time::month_length (1, mm);
      err.entry (tmp.str ());
      ok = false;
    }
  return ok;
}

void 
ActionCrop::Tillage::load_syntax (Frame& frame)
{ 
  frame.add_check (check_alist);
  frame.declare_integer ("month", Attribute::Const, 
	      "Month in the year.");
  frame.set_check ("month", VCheck::valid_month ());
  frame.declare_integer ("day", Attribute::Const, 
	      "Day in the month.");
  frame.set_check ("mday", VCheck::valid_mday ());
  frame.declare_object ("operation", Action::component, 
                     "Tillage operation.");
  frame.order ("month", "day", "operation");
}

ActionCrop::Tillage::Tillage (const Block& al)
  : month (al.integer ("month")),
    day (al.integer ("day")),
    operation (Librarian::build_item<Action> (al, "operation"))
{ }

ActionCrop::Tillage::~Tillage ()
{ }

void 
ActionCrop::Spray::output (Log&) const
{ }

bool 
ActionCrop::Spray::check_alist (const Metalib&, const Frame& al, Treelog& err)
{
  bool ok = true;
  const int mm = al.integer ("month");
  const int dd = al.integer ("day");

  if (mm < 1 || mm > 12)
    {
      err.entry ("month should be between 1 and 12");
      ok = false;
    }
  // don't test for bad month.
  else if (dd < 1 || dd > Time::month_length (1 /* not a leap year */, mm))
    {
      std::ostringstream tmp;
      tmp << "day should be between 1 and " << Time::month_length (1, mm);
      err.entry (tmp.str ());
      ok = false;
    }

  return ok;
}

void 
ActionCrop::Spray::load_syntax (Frame& frame)
{ 
  frame.add_check (check_alist);
  frame.declare_integer ("month", Attribute::Const, 
	      "Month in the year.");
  frame.declare_integer ("day", Attribute::Const, 
	      "Day in the month.");
  frame.declare_string ("name", Attribute::Const,
	      "Name of chemichal to spray.");
  frame.declare ("amount", "g/ha", Check::non_negative (), Attribute::Const,
	      "Amount of chemichal to spray.");
  frame.order ("month", "day", "name", "amount");
}

ActionCrop::Spray::Spray (const Frame& al)
  : month (al.integer ("month")),
    day (al.integer ("day")),
    name (al.name ("name")),
    amount (al.number ("amount"))
{ }

ActionCrop::Spray::~Spray ()
{ }

bool
ActionCrop::Irrigation::doIt (Daisy& daisy, const Scope&, Treelog& msg) const
{
  if (iszero (amount))
    return false;

  const int mm = daisy.time ().month ();
  const int dd = daisy.time ().yday ();

  if (mm < from.month || (mm == from.month && dd < from.day))
    return false;
  if (mm > to.month || (mm == to.month && dd > to.day))
    return false;

  const double depth = -20;

  if (daisy.field ().soil_water_potential (depth) >= potential)
    return false;

  std::ostringstream tmp;
  tmp << "Irrigating " << amount << " mm";
  msg.message (tmp.str ());
  const Units& units = daisy.units ();
  daisy.field ().irrigate (amount/flux, flux, ::Irrigation::at_air_temperature,
                         ::Irrigation::overhead, 
                         IM (units.get_unit (IM::solute_unit ())),
                         boost::shared_ptr<Volume> (), false, msg);  
  return true;
}

void 
ActionCrop::Irrigation::load_syntax (Frame& frame)
{ 
  frame.declare_submodule ("from", Attribute::Const, 
			"Start of irrigation period.",
			MM_DD::load_syntax);
  frame.declare_submodule ("to", Attribute::Const, 
			"End of irrigation period.",
			MM_DD::load_syntax);
  frame.declare ("flux", "mm/h", Check::positive (), Attribute::Const,
                 "Water application speed.");
  frame.set ("flux", 2.0);
  frame.declare ("amount", "mm", Check::non_negative (), Attribute::Const, 
	      "Amount of water to apply on irrigation.");
  frame.declare ("potential", "cm", Check::negative (), Attribute::Const, 
	      "Soil potential at which to irrigate.");
}

ActionCrop::Irrigation::Irrigation (const FrameSubmodel& al)
  : from (al.submodel ("from")),
    to (al.submodel ("to")),
    flux (al.number ("flux")),
    amount (al.number ("amount")),
    potential (al.number ("potential"))
{ }

ActionCrop::Irrigation::~Irrigation ()
{ }

void
ActionCrop::doIt (Daisy& daisy, const Scope& scope, Treelog& msg)
{
  // Sowing.
  primary->doIt (metalib, daisy, scope, msg);
  if (secondary)
    secondary->doIt (metalib, daisy, scope, msg);

  // Harvesting.
  bool harvested = false;
  if (harvest_annual && harvest_perennial)
    {
      // We have both annual and perennial crops.
      daisy_assert (secondary);
      if (harvest_annual->done)
	{
	  // If annual done, do perennial.
	  if (secondary->done 
	      && harvest_perennial->doIt (daisy, scope, msg,
					  secondary->crop->type_name ()))
	    harvested = true;
	}
      else if (primary->done 
	       && harvest_annual->doIt (daisy, scope, msg, 
					primary->crop->type_name ()))
	// else do annual.
	harvested = true;
    }
  else if (harvest_annual)
    {
      // We have only annual crops.  Let 'primary' when they are harvested.
      if (primary->done 
	  && harvest_annual->doIt (daisy, scope, 
                                   msg, primary->crop->type_name ()))
	harvested = true;
    }
  else
    { 
      // We have only perennial crops.
      daisy_assert (harvest_perennial);
      if (secondary)
	{
	  // If we have two, let them both control.
	  if ((primary->done || secondary->done)
	      && harvest_perennial->doIt (daisy, scope, msg,
					  primary->crop->type_name (),
					  secondary->crop->type_name ()))
	    harvested = true;
	}
      else if (primary->done 
	       && harvest_perennial->doIt (daisy, scope, msg, 
					   primary->crop->type_name ()))
	// If we have only one, it is of course in control.
	harvested = true;
    }

  // Fertilize.
  if (fertilize_at_index < fertilize_at.size ()
      && daisy.time ().hour () == 8
      && daisy.time ().month () == fertilize_at[fertilize_at_index]->month
      && daisy.time ().mday () == fertilize_at[fertilize_at_index]->day)
    {
      // Fertilize by date.
      fertilize (metalib, daisy, msg, *fertilize_at[fertilize_at_index]->what);
      fertilize_at_index++;
    }
  if (harvested && harvest_perennial && harvest_perennial->fertilize)
    {
      // Fertilize after harvest.
      if (harvest_perennial->fertilize_year < 0)
	{
	  // First season initialization.
	  harvest_perennial->fertilize_year = daisy.time ().year ();
	  harvest_perennial->fertilize_rest_index
	    = harvest_perennial->fertilize_rest->size ();
	}
      else if (harvest_perennial->fertilize_year < daisy.time ().year ())
	{
	  // New season initialization.
	  if (harvest_perennial->fertilize_rest)
	    {
	      // If we have 'fertilize_rest', use it.
	      harvest_perennial->fertilize_rest_index = 0;
	      harvest_perennial->fertilize_index
		= harvest_perennial->fertilize->size ();
	    }
	  else 
	    {
	      // Otherwise, reuse 'fertilize'.
	      harvest_perennial->fertilize_index = 0;
	    }
	  harvest_perennial->fertilize_year = daisy.time ().year ();
	}
      if (harvest_perennial->fertilize_index 
	  < harvest_perennial->fertilize->size ())
	{
	  // If 'fertilize' is active, use it.
          const FrameModel& model = *(*harvest_perennial->fertilize) 
            [harvest_perennial->fertilize_index];
	  fertilize (metalib, daisy, msg, model);
	  harvest_perennial->fertilize_index++;
	}
      else if (harvest_perennial->fertilize_rest 
	       && (harvest_perennial->fertilize_rest_index
		   < harvest_perennial->fertilize_rest->size ()))
	{
	  // Else, if 'fertilize_rest' is active, us that.
          const FrameModel& model = *(*harvest_perennial->fertilize_rest)
            [harvest_perennial->fertilize_rest_index];
          fertilize (metalib, daisy, msg, model);
	  harvest_perennial->fertilize_rest_index++;
	}
    }

  // Tillage.
  if (tillage_index < tillage.size ()
      && daisy.time ().hour () == 8
      && daisy.time ().month () == tillage[tillage_index]->month
      && daisy.time ().mday () == tillage[tillage_index]->day)
    {
      tillage[tillage_index]->operation->doIt (daisy, scope, msg);
      tillage_index++;
    }

  // Spray.
  if (spray_index < spray.size ()
      && daisy.time ().hour () == 8
      && daisy.time ().month () == spray[spray_index]->month
      && daisy.time ().mday () == spray[spray_index]->day)
    {
      symbol chemical = spray[spray_index]->name;
      const double amount = spray[spray_index]->amount;
      msg.message ("Spraying " + chemical);
      daisy.field ().spray_overhead (chemical, amount, msg); 

      spray_index++;
    }

  // Irrigation.
  if (irrigation_year < 0)
    irrigation_year = daisy.time ().year ();
  if (irrigation_delay > 0)
    irrigation_delay--;
  else if (irrigation)
    {
      if (irrigation_rest && irrigation_year != daisy.time ().year ())
	{
	  if (irrigation_rest->doIt (daisy, scope, msg))
	    irrigation_delay = 48;
	}
      else if (irrigation->doIt (daisy, scope, msg))
	irrigation_delay = 48;
    }
}

bool 
ActionCrop::done (const Daisy& daisy, const Scope&, Treelog& msg) const
{ return (!harvest_annual || harvest_annual->done) 
    && (!harvest_perennial || harvest_perennial->done (daisy, msg)); }

void 
ActionCrop::output (Log& log) const
{ 
  output_submodule (*primary, "primary", log);
  if (secondary)
    output_submodule (*secondary, "secondary", log);
  if (harvest_annual)
    output_submodule (*harvest_annual, "harvest_annual", log);
  if (harvest_perennial)
    output_submodule (*harvest_perennial, "harvest_perennial", log);
  output_variable (fertilize_at_index, log);
  output_ordered (tillage, "tillage", log);
  output_variable (tillage_index, log);
  output_ordered (spray, "spray", log);
  output_variable (spray_index, log);
  if (irrigation_year >= 0)
    output_variable (irrigation_year, log);
  output_variable (irrigation_delay, log);
}

void 
ActionCrop::tick (const Daisy& daisy, const Scope& scope, Treelog& msg)
{ 
  for (std::vector<const Tillage*>::const_iterator i = tillage.begin ();
       i != tillage.end ();
       i++)
    (*i)->operation->tick (daisy, scope, msg);
}

void 
ActionCrop::initialize (const Daisy& daisy, const Scope& scope, Treelog& msg)
{ 
  for (std::vector<const Tillage*>::const_iterator i = tillage.begin ();
       i != tillage.end ();
       i++)
    (*i)->operation->initialize (daisy, scope, msg);
}

bool
ActionCrop::check (const Daisy& daisy, const Scope& scope, Treelog& msg) const
{ 
  bool ok = true;

  for (std::vector<const Tillage*>::const_iterator i = tillage.begin ();
       i != tillage.end ();
       i++)
    if (!(*i)->operation->check (daisy, scope, msg))
      ok = false;
  
  return ok;
}

ActionCrop::ActionCrop (const BlockModel& al)
  : Action (al),
    metalib (al.metalib ()),
    primary (new Sow (al.submodel ("primary"))),
    secondary (al.check ("secondary") 
	       ? new Sow (al.submodel ("secondary"))
	       : NULL),
    harvest_annual (al.check ("harvest_annual") 
	       ? new Annual (al.submodel ("harvest_annual"))
	       : NULL),
    harvest_perennial (al.check ("harvest_perennial") 
		       ? new Perennial (al.submodel ("harvest_perennial"))
		       : NULL),
    fertilize_at (map_submodel_const<Fertilize> (al, "fertilize_at")),
    fertilize_at_index (al.integer ("fertilize_at_index")),
    fertilize_incorporate (al.flag ("fertilize_incorporate")),
    tillage (map_submodel_const<Tillage> (al, "tillage")),
    tillage_index (al.integer ("tillage_index")),
    spray (map_construct_const<Spray> (al.submodel_sequence ("spray"))),
    spray_index (al.integer ("spray_index")),
    irrigation (al.check ("irrigation") 
	       ? new Irrigation (al.submodel ("irrigation"))
	       : NULL),
    irrigation_rest (al.check ("irrigation_rest") 
		     ? new Irrigation (al.submodel ("irrigation_rest"))
		     : NULL),
    irrigation_year (al.check ("irrigation_year")
		     ? al.integer ("irrigation_year")
		     : -1),
    irrigation_delay (al.check ("irrigation_delay")
		      ? al.integer ("irrigation_delay")
		      : 0)
{ }

ActionCrop::~ActionCrop ()
{ 
  delete primary;
  delete secondary;
  delete harvest_annual;
  delete harvest_perennial;
  sequence_delete (fertilize_at.begin (), fertilize_at.end ());
  sequence_delete (tillage.begin (), tillage.end ());
  sequence_delete (spray.begin (), spray.end ());
  delete irrigation;
  delete irrigation_rest;
}

// Add the ActionCrop syntax to the syntax table.
static struct ActionCropSyntax : public DeclareModel
{
  Model* make (const BlockModel& al) const
  { return new ActionCrop (al); }

  static bool check_alist (const Metalib&, const Frame& al, Treelog& err)
  {
    bool ok = true;

    if (al.check ("irrigation_rest") && !al.check ("irrigation"))
      {
	err.entry ("'irrigation_rest' specified, but 'irrigation' isn't");
	ok = false;
      }
    if (!al.check ("harvest_annual") && !al.check ("harvest_perennial"))
      {
	err.entry ("No harvest specified");
	ok = false;
      }
    if (al.check ("harvest_annual") && al.check ("harvest_perennial")
	&& !al.check ("secondary"))
      {
	err.entry ("You must specify a secondary crop when you specify both "
		   "annual and perennial harvest");
	ok = false;
      }
    return ok;
  }

  ActionCropSyntax ()
    : DeclareModel (Action::component, "crop", "\
Manage a specific crop or multicrop.")
  { }
  void load_frame (Frame& frame) const
  { 
    frame.add_check (check_alist);

    frame.declare_submodule ("primary", Attribute::State,
			  "Primary crop.", ActionCrop::Sow::load_syntax);
    frame.declare_submodule ("secondary", Attribute::OptionalState, 
			  "Secondary crop, if any.",
			  ActionCrop::Sow::load_syntax);
    frame.declare_submodule ("harvest_annual",
			  Attribute::OptionalState,
			  "Harvest parameters for annual crops.", 
			  ActionCrop::Annual::load_syntax);
    frame.declare_submodule ("harvest_perennial",
			  Attribute::OptionalState, "\
Harvest conditions for perennial crops.",
			  ActionCrop::Perennial::load_syntax);
    frame.declare_submodule_sequence("fertilize_at", Attribute::Const, "\
Fertilizer application by date.", ActionCrop::Fertilize::load_syntax);
    frame.set_empty ("fertilize_at");
    frame.declare_integer ("fertilize_at_index", Attribute::State,
		"Next entry in 'fertilize_at' to execute.");
    frame.set ("fertilize_at_index", 0);
    frame.declare_boolean ("fertilize_incorporate", Attribute::Const,
		"Incorporate organic fertilizer in plowing zone.");
    frame.set ("fertilize_incorporate", false);
    frame.declare_submodule_sequence ("tillage", Attribute::State, "\
List of tillage operations to apply.", ActionCrop::Tillage::load_syntax);
    frame.set_empty ("tillage");
    frame.declare_integer ("tillage_index", Attribute::State,
		"Next entry in 'tillage' to execute.");
    frame.set ("tillage_index", 0);
    frame.declare_submodule_sequence ("spray", Attribute::State, "\
List of chemicals to apply.", ActionCrop::Spray::load_syntax);
    frame.set_empty ("spray");
    frame.declare_integer ("spray_index", Attribute::State,
		"Next entry in 'spray' to execute.");
    frame.set ("spray_index", 0);
    frame.declare_submodule ("irrigation", Attribute::OptionalConst, "\
Irrigation model for first season.  If missing, don't irrigate.", 
			  ActionCrop::Irrigation::load_syntax);
    frame.declare_submodule ("irrigation_rest", Attribute::OptionalConst, "\
Irrigation model for remaining seasons.\n\
If missing, use the same model as first season.",
			  ActionCrop::Irrigation::load_syntax);
    frame.declare_integer ("irrigation_year", Attribute::OptionalState, 
		"Year management started.\n\
Negative number means it hasn't started yet.");
    frame.declare_integer ("irrigation_delay", Attribute::OptionalState, 
		"Hours we test for irrigation again.\n\
This is set at each irrigation, to avoid multiple applications.");
      
  }
} ActionCrop_syntax;

// action_crop.C ends here.
