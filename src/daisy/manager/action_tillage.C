// action_tillage.C
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
#include "object_model/block_model.h"
#include "daisy/daisy.h"
#include "daisy/field.h"
#include "object_model/check.h"
#include "object_model/librarian.h"
#include "object_model/treelog.h"
#include "object_model/frame.h"

struct ActionMix : public Action
{
  // Content.
  const double depth;           // [cm]
  const double random_roughness; // [m]
  const double penetration;     // [0.1]
  const double surface_loose;   // [0-1]

  // Simulation.
  void doIt (Daisy& daisy, const Scope&, Treelog& msg)
    {
      TREELOG_MODEL (msg);
      daisy.field ().mix (0.0, depth, 
                          penetration, surface_loose, random_roughness,
                          daisy.time (),  msg);
    }

  void tick (const Daisy&, const Scope&, Treelog&)
  { }
  void initialize (const Daisy&, const Scope&, Treelog&)
  { }
  bool check (const Daisy&, const Scope&, Treelog& err) const
  { return true; }

  ActionMix (const BlockModel& al)
    : Action (al),
      depth (al.number ("depth")),
      random_roughness (al.number ("random_roughness")),
      penetration (al.number ("penetration")),
      surface_loose (al.number ("surface_loose", penetration))
    { }
};

static struct ActionMixSyntax : DeclareModel
{
  Model* make (const BlockModel& al) const
    { return new ActionMix (al); }

  ActionMixSyntax ()
    : DeclareModel (Action::component, "mix", "\
Mix soil content down to the specified depth.\n\
The effect is that nitrogen, water, temperature and such are averaged in\n\
the interval.")
  { }
  void load_frame (Frame& frame) const
  { 
      frame.declare ("depth", "cm", Check::negative (), Attribute::Const,
		  "How far down to mix the soil (a negative number).");
      frame.order ("depth");
      frame.declare ("random_roughness", "m", Check::positive (), 
                     Attribute::Const, "\
Random roughhness generated by tillage operation.");
      frame.set ("random_roughness", 0.012);
      frame.declare_fraction ("penetration", Attribute::Const, "\
Fraction of organic matter on surface that are incorporated in the soil\n\
by this operation.");
      frame.set ("penetration", 1.0);
      frame.declare_fraction ("surface_loose", Attribute::OptionalConst, "\
Fraction of surface beeing loosened by opretation.\n\
By defeault, this is equal to 'penetration'.");
    }
} ActionMix_syntax;

struct ActionSwap : public Action
{
  // Content.
  const double middle;
  const double depth;
  const double random_roughness; // [m]

  // Simulation.
  void doIt (Daisy& daisy, const Scope&, Treelog& msg)
  {
    TREELOG_MODEL (msg);
    daisy.field ().swap (0.0, middle, depth, 
                         random_roughness, daisy.time (), msg);
  }

  void tick (const Daisy&, const Scope&, Treelog&)
  { }
  void initialize (const Daisy&, const Scope&, Treelog&)
  { }
  bool check (const Daisy&, const Scope&, Treelog& err) const
  { return true; }

  ActionSwap (const BlockModel& al)
    : Action (al),
      middle (al.number ("middle")),
      depth (al.number ("depth")),
      random_roughness (al.number ("random_roughness"))
  { }
};

static struct ActionSwapSyntax : DeclareModel
{
  Model* make (const BlockModel& al) const
  { return new ActionSwap (al); }

  static bool check_alist (const Metalib&, const Frame& al, Treelog& err)
  {
    const double middle (al.number ("middle"));
    const double depth (al.number ("depth"));
    bool ok = true;
    if (middle <= depth)
      {
        err.entry ("swap middle should be above the depth");
        ok = false;
      }
    return ok;
  }

  ActionSwapSyntax ()
    : DeclareModel (Action::component, "swap", "\
Swap two soil layers.  The top layer start at the surface and goes down to\n\
'middle', and the second layer starts with 'middle' and goes down to\n\
 'depth'.  After the operation, the content (such as heat, water, and\n\
organic matter) will be averaged in each layer, and the bottom layer will\n\
be placed on top of what used to be the top layer.")
  { }
  void load_frame (Frame& frame) const
  {
    frame.add_check (check_alist);
    frame.declare ("middle", "cm", Check::negative (), Attribute::Const, "\
The end of the first layer and the start of the second layer to swap.");
    frame.declare ("depth", "cm", Check::negative (), Attribute::Const, "\
The end of the second layer to swap.");
    frame.declare ("random_roughness", "m", Check::positive (), 
                   Attribute::Const, "\
Random roughhness generated by tillage operation.");
    frame.set ("random_roughness", 0.024);
      
  }
} ActionSwap_syntax;


struct ActionSetPorosity : public Action
{
  // Content.
  const double porosity;
  const double depth;

  // Simulation.
  void doIt (Daisy& daisy, const Scope&, Treelog& msg)
  {
    msg.message ("Adjusting porosity");
    daisy.field ().set_porosity (depth, porosity, msg);
  }

  void tick (const Daisy&, const Scope&, Treelog&)
  { }
  void initialize (const Daisy&, const Scope&, Treelog&)
  { }
  bool check (const Daisy&, const Scope&, Treelog& err) const
  { return true; }

  ActionSetPorosity (const BlockModel& al)
    : Action (al),
      porosity (al.number ("porosity")),
      depth (al.number ("depth"))
    { }
};

static struct ActionSetPorositySyntax : DeclareModel
{
  Model* make (const BlockModel& al) const
  { return new ActionSetPorosity (al); }

  ActionSetPorositySyntax ()
    : DeclareModel (Action::component, "set_porosity", "\
Set the porosity of the horizon at the specified depth.\n\
To get useful results, you need to use a hydraulic model that supports this.")
  { }
  void load_frame (Frame& frame) const
  {
    frame.declare_fraction ("porosity", Attribute::Const, "\
Non-solid fraction of soil.");
    frame.declare ("depth", "cm", Check::non_positive (), Attribute::Const, "\
A point in the horizon to modify.");
    frame.set ("depth", 0.0);
  }
} ActionSetPorosity_syntax;

struct ActionStoreSOM : public Action
{
  // Simulation.
  void doIt (Daisy& daisy, const Scope&, Treelog& msg)
  {
    msg.message ("Storing SOM pools.");
    daisy.field ().store_SOM (msg);
  }

  void tick (const Daisy&, const Scope&, Treelog&)
  { }
  void initialize (const Daisy&, const Scope&, Treelog&)
  { }
  bool check (const Daisy&, const Scope&, Treelog& err) const
  { return true; }

  ActionStoreSOM (const BlockModel& al)
    : Action (al)
  { }
};

static struct ActionStoreSOMSyntax : DeclareModel
{
  Model* make (const BlockModel& al) const
  { return new ActionStoreSOM (al); }

  ActionStoreSOMSyntax ()
    : DeclareModel (Action::component, "store_SOM", "\
Store the content of the soil organic matter pools.")
  { }
  void load_frame (Frame& frame) const
  { }
} ActionStoreSOM_syntax;

struct ActionRestoreSOM : public Action
{
  // Simulation.
  void doIt (Daisy& daisy, const Scope&, Treelog& msg)
  {
    msg.message ("Storing SOM pools.");
    daisy.field ().restore_SOM (msg);
  }

  void tick (const Daisy&, const Scope&, Treelog&)
  { }
  void initialize (const Daisy&, const Scope&, Treelog&)
  { }
  bool check (const Daisy&, const Scope&, Treelog& err) const
  { return true; }

  ActionRestoreSOM (const BlockModel& al)
    : Action (al)
  { }
};

static struct ActionRestoreSOMSyntax : DeclareModel
{
  Model* make (const BlockModel& al) const
  { return new ActionRestoreSOM (al); }

  ActionRestoreSOMSyntax ()
    : DeclareModel (Action::component, "restore_SOM", "\
Restore the content of the soil organic matter pools.")
  { }
  void load_frame (Frame& frame) const
  { }
} ActionRestoreSOM_syntax;

// action_tillage.C ends here.
