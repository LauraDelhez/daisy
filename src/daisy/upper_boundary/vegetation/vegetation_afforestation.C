// vegetation_afforestation.C
// 
// Copyright 1996-2003 Per Abrahamsen and Søren Hansen
// Copyright 2000-2003 KVL.
// Copyright 2013 KU.
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
#include "daisy/upper_boundary/vegetation/vegetation.h"
#include "object_model/plf.h"
#include "util/mathlib.h"
#include "daisy/output/log.h"
#include "daisy/crop/root/root_system.h"
#include "daisy/crop/canopy_simple.h"
#include "daisy/daisy_time.h"
#include "daisy/soil/transport/geometry.h"
#include "daisy/soil/soil.h"
#include "daisy/crop/crop.h"
#include "daisy/organic_matter/am.h"
#include "daisy/organic_matter/aom.h"
#include "daisy/organic_matter/organic.h"
#include "object_model/submodeler.h"
#include "object_model/check.h"
#include "object_model/librarian.h"
#include "object_model/frame_submodel.h"
#include "daisy/upper_boundary/bioclimate/bioclimate.h"
#include "daisy/soil/soil_heat.h"
#include "object_model/block_model.h"
#include "object_model/metalib.h"
#include <sstream>
#include <deque>

struct VegetationAfforestation : public Vegetation
{
  const Metalib& metalib;
  // Afforestation parameters.
  const Time planting_time;
  const PLF canopy_height;
  const PLF root_depth;
  const PLF LAI_shape;
  const PLF LAI_min;
  const PLF LAI_max;
  const PLF N_nonleaves;
  const double N_per_LAI;       // kg N/ha
  const PLF litterfall_shape;   // d -> []
  const double litterfall_integrated;   // [shape days]
  const PLF litterfall_total;   // y -> Mg DM/ha
  const double litterfall_C_per_DM; // kg C/kg DM
  const double litterfall_C_per_N; // kg C/kg N 
  const PLF rhizodeposition_shape; // d -> []
  const double rhizodeposition_integrated;   // [shape days]
  const PLF rhizodeposition_total;  // y -> Mg DM/ha
  const double rhizodeposition_C_per_DM; // kg C/ kg DM
  const double rhizodeposition_C_per_N; // kg C/kg N 

  // Vegetation.
  const std::unique_ptr<CanopySimple> canopy;
  double cover_;		// Fraction of soil covered by crops [0-1]
  PLF HvsLAI_;			// Height with LAI below [f: R -> cm]

  // Nitrogen.
  double N_demand;		// Current potential N content. [g N/m^2]
  double N_actual;		// Current N content. [g N/m^2]
  AM* AM_litter;                // Dead leaves.
  AM* AM_root;                  // Dead roots.
  double N_uptake;		// N uptake this hour. [g N/m^2/h]
  double N_litter;		// N litterfall rate. [g N/m^2/h]
  double N_rhizo;		// N rhizodeposition rate. [g N/m^2/h]
  const std::vector<boost::shared_ptr<const FrameModel>/**/>& litter_am;
  const std::vector<boost::shared_ptr<const FrameModel>/**/>& root_am;

  // Root.
  const std::unique_ptr<RootSystem> root_system;
  const double WRoot;		// Root dry matter weight [g DM/m^2]

  // Radiation.
  const double albedo_;		// Another reflection factor.

  // Queries.
  double rs_min () const	// Minimum transpiration resistance.
  { return canopy->rs_min; }
  double rs_max () const	// Maximum transpiration resistance.
  { return canopy->rs_max; }
  double shadow_stomata_conductance () const
  { return 1.0 / rs_min (); }
  double sunlit_stomata_conductance () const
  { return 1.0 / rs_min (); }
  double N () const             // [kg N/ha]
  { 
    // kg/ha -> g/m^2
    const double conv = 1000.0 / (100.0 * 100.0);
    return N_actual / conv; 
  }
  double N_fixated () const     // [kg N/ha/h]
  { return 0.0; }
  double LAI () const
  { return canopy->CAI; }
  double height () const
  { return canopy->Height; }
  double leaf_width () const
  { return canopy->leaf_width (1.0  /* arbitrary */); }
  double cover () const
  { 
    daisy_assert (cover_ >= 0.0);
    return cover_; 
  }
  const PLF& LAIvsH () const
  { return canopy->LAIvsH; }
  const PLF& HvsLAI () const
  { return HvsLAI_; }
  double ACExt_PAR () const
  { return canopy->PARext; }
  double ACRef_PAR () const
  { return canopy->PARref; }
  double ACExt_NIR () const
  { return canopy->PARext; }
  double ACRef_NIR () const
  { return canopy->PARref; }
  double ARExt () const
  { return canopy->EPext; }
  double EpFactorDry () const
  { return canopy->EpFactorDry (1.0 /* arbitrary */); }
  double EpFactorWet () const
  { return canopy->EpFactorWet (1.0 /* arbitrary */); }
  double albedo () const
  { return albedo_; }
  double interception_capacity () const
  { return canopy->IntcpCap * LAI (); }


  // Individual crop queries.
  double DS_by_name (symbol) const
  {   return Crop::DSremove; }
  double stage_by_name (symbol) const
  {   return Crop::DSremove; }
  double DM_by_name (symbol, double) const
  { return 0.0; }
  double SOrg_DM_by_name (symbol) const
  { return 0.0; }
  std::string crop_names () const
  { return objid.name (); }

  const std::vector<double>& effective_root_density () const
  { return root_system->effective_density (); }
  const std::vector<double>& effective_root_density (symbol crop) const
  { 
    static const std::vector<double> empty;
    return empty;
  }

  // Simulation.
  void reset_canopy_structure (double y, double d);
  double transpiration (double potential_transpiration,
			double canopy_evaporation,
                        const Geometry& geo,
			const Soil& soil, const SoilWater& soil_water, 
			double dt, Treelog&);
  void find_stomata_conductance (const Time&, 
                                 const Bioclimate&, double, Treelog&)
  { }
  void tick (const Scope&, const Time& time, const Bioclimate&, 
             const Geometry& geo, const Soil&, const SoilHeat&,
             SoilWater&, Chemistry&, OrganicMatter&,
             double& residuals_DM,
             double& residuals_N_top, double& residuals_C_top,
             std::vector<double>& residuals_N_soil,
             std::vector<double>& residuals_C_soil,
             double dt,
             Treelog&);
  void clear ()
  { }
  void force_production_stress  (double)
  { }
  void emerge (const symbol, Treelog&)
  { }
  void kill_all (symbol, const Time&, const Geometry&,
		 std::vector<AM*>&, double&, double&, double&, 
		 std::vector<double>&, std::vector<double>&, Treelog&)
  { }
  void harvest (symbol, symbol,
		const Time&, const Geometry&, 
		double, double, double, double, 
		std::vector<const Harvest*>&,
                double&, double&, double&,
		double&, double&, double&,
                std::vector<AM*>&, 
		double&, double&, double&,
                std::vector<double>&, std::vector<double>&,
                const bool,
		Treelog&)
  { }
  void pluck (symbol, symbol,
              const Time&, const Geometry&, 
              double, double, double,
              std::vector<const Harvest*>&,
              double&, double&, double&, double&, double&,
              std::vector<AM*>&, 
              double&, double&, double&,
              std::vector<double>&, std::vector<double>&,
              Treelog&)
  { }
  void sow (const Scope&, const FrameModel&, 
            const double, const double, const double,
            const Geometry&, const Soil&, OrganicMatter&, 
            double&, double&, const Time&, Treelog&)
  { throw "Can't sow on afforestation vegetation"; }
  void sow (const Scope&, Crop&, const double, const double, const double,
            const Geometry&, const Soil&, OrganicMatter&, 
            double&, double&, const Time&, Treelog&)
  { throw "Can't sow on afforestation vegetation"; }
  void output (Log&) const;

  // Create and destroy.
  void initialize (const Scope&, const Time& time, const Geometry& geo,
                   const Soil& soil, OrganicMatter&, 
                   Treelog&);
  bool check (const Scope&, const Geometry&, Treelog&) const;
  VegetationAfforestation (const BlockModel&);
  ~VegetationAfforestation ();
};

void
VegetationAfforestation::reset_canopy_structure (const double y, const double d)
{
  // Calculate actual LAI (L) from LAI shape (S).
  const double L_min = LAI_min (y);
  const double L_max = LAI_max (y);
  const double S = LAI_shape (d);
  const double S_min = 1.0;
  const double S_max = 5.0;
  const double L = L_min + (S - S_min) * (L_max - L_min) / (S_max - S_min);
    
  canopy->Height = canopy_height (y);
  canopy->CAI = L;
  cover_ =  1.0 - exp (-(canopy->EPext * canopy->CAI));
  daisy_assert (cover_ >= 0.0);
  daisy_assert (cover_ <= 1.0);
  canopy->LAIvsH.clear ();
  canopy->LAIvsH.add (0.0, 0.0);
  canopy->LAIvsH.add (canopy->Height, canopy->CAI);
  HvsLAI_ = canopy->LAIvsH.inverse ();
}
void
VegetationAfforestation::tick (const Scope&,
			       const Time& time, const Bioclimate& bioclimate, 
                               const Geometry& geo, const Soil& soil, 
                               const SoilHeat& soil_heat,
                               SoilWater& soil_water, Chemistry& chemistry,
                               OrganicMatter& organic_matter,
                               double& residuals_DM,
                               double& residuals_N_top, double& residuals_C_top,
                               std::vector<double>& residuals_N_soil,
                               std::vector<double>& residuals_C_soil,
                               const double dt, Treelog& msg)
{
  // Time.
  const double y = Time::fraction_hours_between (planting_time, time) 
    / (24.0 * 365.2425);
  const double d = time.yday () 
    + ((time.hour () 
        + ((time.minute () 
            + ((time.second () + time.microsecond () / 1e6)
               / 60.0))
           / 60.0))
       / 24.0);
  const double d_end = d + dt / 24.0;

  // Canopy.
  reset_canopy_structure (y, d);

  // Root system.
  const double day_fraction = bioclimate.day_fraction (dt);
  root_system->tick_dynamic (geo, soil_heat, soil_water, day_fraction, dt, msg);

  // Nitrogen uptake.
  N_demand = 0.1 * (canopy->CAI * N_per_LAI + N_nonleaves (y)); // [g/m^2]
  daisy_assert (N_actual >= 0.0);
  N_uptake = root_system->nitrogen_uptake (geo, soil, soil_water, 
                                           chemistry, 0.0, 0.0,
                                           (N_demand - N_actual) / 1.0);

  static const symbol vegetation_symbol ("vegetation");
  static const double g_per_Mg = 1.0e6;
  static const double ha_per_cm2 = 1.0e-8;
  static const double m2_per_cm2 = 1.0e-4;

  // Rhizodeposition.
  if (rhizodeposition_integrated > 0.0)
    {
      const double interval = rhizodeposition_shape.integrate (d, d_end);
      const double amount       // [kg DM/ha]
        = rhizodeposition_total (y) * interval / rhizodeposition_integrated;

      const double DM = amount  // // [g DM/m^2/h]
        * g_per_Mg * ha_per_cm2 / m2_per_cm2 / dt; 
      const double C = DM * rhizodeposition_C_per_DM;
      const double N = C / rhizodeposition_C_per_N / dt;
      if (!AM_root)
        {
          static const symbol root_symbol ("root");

          AM_root = &AM::create (metalib, geo, time, root_am,
                                   vegetation_symbol, root_symbol,
                                   AM::Locked, msg);
          organic_matter.add (*AM_root);

        }
      const std::vector<double>& Density = root_system->actual_density ();
      AM_root->add_surface (geo,
                            C * m2_per_cm2 * dt, N * m2_per_cm2 * dt,
                            Density);

      N_rhizo = N;
      residuals_DM += DM;
      geo.add_surface (residuals_C_soil, Density, C * m2_per_cm2);
      geo.add_surface (residuals_N_soil, Density, N * m2_per_cm2);
    }
  
  // Litterfall.
  if (litterfall_integrated > 0.0)
    {
      const double interval 
        = litterfall_shape.integrate (d, d_end);
      const double amount       // [kg DM/ha]
        = litterfall_total (y) * interval / litterfall_integrated;
      const double DM = amount  // // [g DM/m^2/h]
        * g_per_Mg * ha_per_cm2 / m2_per_cm2 / dt; 
      const double C = DM * litterfall_C_per_DM;
      const double N = C / litterfall_C_per_N / dt;
      if (!AM_litter)
        {
          static const symbol dead_symbol ("dead");

          AM_litter = &AM::create (metalib, geo, time, litter_am,
                                   vegetation_symbol, dead_symbol,
                                   AM::Locked, msg);
          organic_matter.add (*AM_litter);

        }
      AM_litter->add (C * m2_per_cm2 * dt, N * m2_per_cm2 * dt);
      N_litter = N;
      residuals_N_top += N_litter;
      residuals_DM += DM;
      residuals_C_top += C;
      
      // TODO
    }

  N_actual += (N_uptake - N_litter - N_rhizo) * dt;
}

double
VegetationAfforestation::transpiration (const double potential_transpiration,
					const double canopy_evaporation,
					const Geometry& geo,
					const Soil& soil, 
					const SoilWater& soil_water,
					const double dt, 
					Treelog& msg)
{
  if (canopy->CAI > 0.0)
    return  root_system->water_uptake (potential_transpiration, 
                                       geo, soil, soil_water, 
                                       canopy_evaporation, 
                                       dt, msg);
  return 0.0;
}

void
VegetationAfforestation::output (Log& log) const
{
  Vegetation::output (log);
  output_submodule (*canopy, "Canopy", log);
  output_variable (N_demand, log);
  output_variable (N_actual, log);
  output_variable (N_uptake, log);
  output_variable (N_litter, log);
  output_variable (N_rhizo, log);
  output_submodule (*root_system, "Root", log);
}

void
VegetationAfforestation::initialize (const Scope&, const Time& time, 
                                 const Geometry& geo,
                                 const Soil& soil, 
				 OrganicMatter& organic_matter,
                                 Treelog& msg)
{
  const double y = Time::fraction_hours_between (planting_time, time) 
    / (24.0 * 365.2425);
  const double d = time.yday () + time.hour () / 24.0;
  
  reset_canopy_structure (y, d);
  root_system->initialize (geo, soil, 0.0, 0.0, Crop::DSremove, msg);
  root_system->full_grown (geo, soil, WRoot, msg);

  static const symbol vegetation_symbol ("vegetation");
  static const symbol dead_symbol ("dead");
  static const symbol root_symbol ("root");
  AM_litter = organic_matter.find_am (vegetation_symbol, dead_symbol);
  AM_root = organic_matter.find_am (vegetation_symbol, root_symbol);

  N_demand = 0.1 * (canopy->CAI * N_per_LAI + N_nonleaves (y)); // [g/m^2]
  if (N_actual < -1e10)
    N_actual = N_demand;	// Initialization.
  else
    daisy_assert (N_actual >= 0.0);
}

bool 
VegetationAfforestation::check (const Scope&, const Geometry& geo, Treelog& msg) const 
{ 
  bool ok = true;
  if (!root_system->check (geo, msg))
    ok = false;
  return ok;
}

VegetationAfforestation::VegetationAfforestation (const BlockModel& al)
  : Vegetation (al),
    metalib (al.metalib ()),
    planting_time (al.submodel ("planting_time")),
    canopy_height (al.plf ("canopy_height")),
    root_depth (al.plf ("root_depth")),
    LAI_shape (al.plf ("LAI_shape")),
    LAI_min (al.plf ("LAI_min")),
    LAI_max (al.plf ("LAI_max")),
    N_nonleaves (al.plf ("N_nonleaves")),
    N_per_LAI (al.number ("N_per_LAI")),
    litterfall_shape (al.plf ("litterfall_shape")), 
    litterfall_integrated (litterfall_shape.integrate (0.0, 366.0)),
    litterfall_total (al.plf ("litterfall_total")),
    litterfall_C_per_DM (al.number ("litterfall_C_per_DM")),
    litterfall_C_per_N (al.number ("litterfall_C_per_N")),
    rhizodeposition_shape (al.plf ("rhizodeposition_shape")),
    rhizodeposition_integrated (rhizodeposition_shape.integrate (0.0, 366.0)),
    rhizodeposition_total (al.plf ("rhizodeposition_total")),
    rhizodeposition_C_per_DM (al.number ("rhizodeposition_C_per_DM")),
    rhizodeposition_C_per_N (al.number ("rhizodeposition_C_per_N")),

    canopy (submodel<CanopySimple> (al, "Canopy")),
    cover_ (-42.42e42),
    N_demand (0.0),
    N_actual (al.number ("N_actual", -42.42e42)),
    AM_litter (NULL),
    AM_root (NULL),
    N_uptake (0.0),
    N_litter (0.0),
    N_rhizo (0.0),
    litter_am (al.model_sequence ("litter_am")),
    root_am (al.model_sequence ("root_am")),
    root_system (submodel<RootSystem> (al, "Root")),
    WRoot (al.number ("root_DM") * 100.0), // [Mg DM / ha] -> [g DM / m^2]
    albedo_ (al.number ("Albedo"))
{ }

VegetationAfforestation::~VegetationAfforestation ()
{ }

static struct VegetationAfforestationSyntax : public DeclareModel
{
  Model* make (const BlockModel& al) const
  { return new VegetationAfforestation (al); }

  VegetationAfforestationSyntax ()
    : DeclareModel (Vegetation::component, "afforestation", "\
A growing forest.")
  { }

  void load_frame (Frame& frame) const
  {
    frame.declare_submodule ("planting_time", Attribute::State, "\
Reference time for yearly growth parameters.", Time::load_syntax);
    frame.declare ("canopy_height", "y", "cm", 
                   Check::non_negative (), Attribute::Const, "\
Forest height as a function of years after planting.");
    frame.declare ("root_depth", "y", "cm", 
                   Check::non_negative (), Attribute::Const, "\
Depth of effective root zone as a function of years after planting.");
    frame.declare ("LAI_shape", "d", Attribute::None (), 
                   Check::positive (), Attribute::Const, 
                   "LAI factor as a function of Julian day.\n\
The value `1' represents `LAI_min' and the value `5' represents 'LAI_max'\n\
for the specific year.");
    frame.declare ("LAI_min", "y", "m^2/m^2", 
                   Check::non_negative (), Attribute::Const, "\
Yearly minimum LAI as a function of years after planting.");
    frame.declare ("LAI_max", "y", "m^2/m^2", 
                   Check::positive (), Attribute::Const, "\
Yearly maximum LAI as a function of years after planting.");
    frame.declare ("N_nonleaves", "y", "kg N/ha", Attribute::Const, "\
Nitrogen not accounted for by seasonal LAI variation.\n\
This is a function of years after planting.");
    frame.declare ("N_per_LAI", "kg N/ha",
                   Check::positive (), Attribute::Const,
                   "N content as function of LAI.");
    frame.declare ("litterfall_shape", "d", Attribute::None (), 
                   Check::non_negative (), Attribute::Const, "\
Relative speed of litterfall.\n\
\n\
The total litterfall over a year is specified by 'litterfall_total'.\n\
This parameter specifies how the the litterfall is divided over a\n\
year.  The function is integrated over a year. The litterfall in a\n\
specific period is then the integration of this parameter over the\n\
period, divided by the integration of this function over a year,\n\
multiplied with the total litterfall for that year.");
    frame.declare ("litterfall_total", "y", "Mg DM/ha", 
                   Check::non_negative (), Attribute::Const, "\
Yearly litterfall as function of years after planting.");
    frame.declare ("litterfall_C_per_DM", Attribute::None (),
                   Check::positive (), Attribute::Const,
                   "Carbon content of litter dry matter.");
    frame.set ("litterfall_C_per_DM", 0.42);
    frame.declare ("litterfall_C_per_N", Attribute::None (),
                   Check::positive (), Attribute::Const,
                   "C/N ratio in litter.");

    frame.declare ("rhizodeposition_shape", "d", Attribute::None (), 
                   Check::non_negative (), Attribute::Const, "\
Relative speed of rhizodeposition.\n\
\n\
The total rhizodeposition over a year is specified by\n\
'rhizodeposition_total'.  This parameter specifies how the the\n\
rhizodeposition is divided over a year.  The function is integrated\n\
over a year. The rhizodeposition in a specific period is then the\n\
integration of this parameter over the period, divided by the\n\
integration of this function over a year, multiplied with the total\n\
rhizodeposition for that year.");
    frame.declare ("rhizodeposition_total", "y", "Mg DM/ha", 
                   Check::non_negative (), Attribute::Const, "\
Yearly rhizodeposition as function of years after planting.");
    frame.declare ("rhizodeposition_C_per_DM", Attribute::None (),
                   Check::positive (), Attribute::Const,
                   "Carbon content of litter dry matter.");
    frame.set ("rhizodeposition_C_per_DM", 0.42);
    frame.declare ("rhizodeposition_C_per_N", Attribute::None (),
                   Check::positive (), Attribute::Const,
                   "C/N ratio in rhizodeposition.");

    frame.declare ("N_demand", "g N/m^2", Attribute::LogOnly,
		"Current potential N content.");
    frame.declare ("N_actual", "g N/m^2", Attribute::OptionalState,
		"N uptake until now (default: 'N_demand').");
    frame.declare ("N_uptake", "g N/m^2/h", Attribute::LogOnly,
		"Nitrogen uptake rate.");
    frame.declare ("N_litter", "g N/m^2/h", Attribute::LogOnly,
		"Nitrogen in litterfall.");
    frame.declare ("N_rhizo", "g N/m^2/h", Attribute::LogOnly,
                   "Nitrogen in rhizodeposition.");
    frame.declare_object ("litter_am", AOM::component, 
                      Attribute::Const, Attribute::Variable, "\
Litter AOM parameters.");
    frame.set_check ("litter_am", AM::check_om_pools ());
    frame.set ("litter_am", AM::default_AM ());
    frame.declare_object ("root_am", AOM::component, 
                          Attribute::Const, Attribute::Variable, "\
Rhizodeposition AOM parameters.");
    frame.set_check ("root_am", AM::check_om_pools ());
    frame.set ("root_am", AM::default_AM ());
    frame.declare_submodule("Root", Attribute::State, "Root system.",
                            RootSystem::load_syntax);
    frame.declare ("root_DM", "Mg DM/ha", Check::positive (), Attribute::Const, 
                   "Afforestation root drymatter.");
    frame.set ("root_DM", 2.0);
    frame.declare ("Albedo", 
                   Attribute::None (), Check::positive (), Attribute::Const, 
                   "Reflection factor.");
    frame.set ("Albedo", 0.2);
    frame.declare_submodule("Canopy", Attribute::State, "Canopy.",
                            CanopySimple::load_syntax);
  }
} VegetationAfforestation_syntax;

// vegetation_afforestation.C ends here.
