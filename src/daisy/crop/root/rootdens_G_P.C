// rootdens_G_P.C -- Gerwitz and Page model for calculating root density.
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

#include "daisy/crop/root/rootdens.h"
#include "object_model/block_model.h"
#include "daisy/soil/transport/geometry.h"
#include "daisy/output/log.h"
#include "object_model/check.h"
#include "util/mathlib.h"
#include "object_model/librarian.h"
#include "object_model/treelog.h"
#include "object_model/frame.h"

#include <sstream>

struct Rootdens_G_P : public Rootdens
{
  // Parameters.
  const double DensRtTip;	// Root density at (pot) pen. depth [cm/cm^3]
  const double MinDens;		// Minimal root density [cm/cm^3]
  // Log variables.
  double a;                     // Form "parameter" [cm^-1].
  double L0;                    // Root density at soil surface [cm/cm^3]

  // Simulation.
  static double density_distribution_parameter (double a);
  void set_density (const Geometry& geo, 
		    double SoilDepth, double CropDepth,
		    const double /* CropWidth [cm] */,
		    double WRoot, double DS,
		    std::vector<double>& Density, Treelog&);
  void output (Log& log) const;

  // Create.
  void initialize (const Geometry&, double /* row_width */, double, Treelog&)
  { }
  Rootdens_G_P (const BlockModel&);
};

double
Rootdens_G_P::density_distribution_parameter (double a)
{
  daisy_assert (a > 1.0);
  double x, y, z, x1, y1, z1, x2, y2, z2;

  if (1 + a > exp (1.0))
    {
      x1 = 1.0;
      y1 = exp (x1);
      z1 = 1 + a * x1;
      x2 = 20.0;
      y2 = exp (x2);
      z2 = 1 + a * x2;
      while ((z1 - y1) * (z2 - y2) > 0)
	{
	  x1 = x2;
	  y1 = y2;
	  z1 = z2;
	  x2++;
	  y2 = exp (x2);
	  z2 = 1 + a * x2;
	}
    }
  else 
    {
      x1 = 0.3;
      y1 = exp (x1);
 //     z1 = 1 + a * x1;
      x2 = 1.0;
      y2 = exp (x2);
 //     z2 = 1 + a * x2;
    }

  x = (y2 * (x2 - 1) - y1 * (x1 - 1)) / (y2 - y1);
  y = exp (x);
  z = 1 + a * x;
  while (fabs (2 * (z - y) / (z + y)) > 1.0e-5)
    {
      if (z - y > 0)
	{
	  x1 = x;
	  y1 = y;
	  // z1 = z;
	}
      else
	{
	  x2 = x;
	  y2 = y;
	  // z2 = z;
	}
      x = (y2 * (x2 - 1) - y1 * (x1 - 1)) / (y2 - y1);
      y = exp (x);
      z = 1 + a * x;
    }
  return x;
}

void
Rootdens_G_P::set_density  (const Geometry& geo, 
			    double SoilDepth, double CropDepth,
			    const double /* CropWidth [cm] */,
			    double WRoot, double /* DS */,
			    std::vector<double>& Density, Treelog& msg)
{
  const double Depth = std::min (SoilDepth, CropDepth);
  // Dimensional conversion.
  static const double m_per_cm = 0.01;

  const double MinLengthPrArea = (DensRtTip * 1.2) * CropDepth;
  const double LengthPrArea
    = std::max (m_per_cm * SpRtLength * WRoot, MinLengthPrArea); // [cm/cm^2]
  
  // We find a * depth first.
  const double ad = density_distribution_parameter (LengthPrArea / 
						    (CropDepth * DensRtTip));
  // We find L0 from a d.
  //
  // L0 * exp (-a d) = DensRtTip
  // => L0 = DensRtTip / exp (-a d)
  L0 = DensRtTip * exp (ad);	//  1 / exp (-x) = exp (x)
  a = ad / CropDepth;

  if (Depth < CropDepth)
    {
      double Lz = L0 * exp (-a * Depth);
      a = density_distribution_parameter (LengthPrArea / (Depth * Lz)) / Depth;
    }

  // Check minimum density
  double extra = 0.0;
  if (MinDens > 0.0 && WRoot > 0.0)
    {
      daisy_assert (L0 > 0.0);
      daisy_assert (a > 0.0);
      const double too_low = -log (MinDens / L0) / a; // [cm]

      if (too_low < Depth)
	{
	  // We don't have MinDens all the way down.
	  const double NewLengthPrArea 
	    =  LengthPrArea - MinDens * Depth; // [cm/cm^2]
	  Treelog::Open nest (msg, "RootDens G+P");
	  std::ostringstream tmp;
	  tmp << "too_low = " << too_low 
		 << ", NewLengthPrArea = " << NewLengthPrArea
		 << "MinLengthPrArea = " << MinLengthPrArea;
	  msg.warning (tmp.str ());
	  if (too_low > 0.0 && NewLengthPrArea > too_low * DensRtTip * 1.2)
	    {
	      // There is enough to have MinDens all the way, spend
	      // the rest using the standard model until the point
	      // where the standard model would give too little..
	      a = density_distribution_parameter (NewLengthPrArea
						  / (too_low * DensRtTip));
	      L0 = DensRtTip * exp (a);
	      a /= too_low;
	      extra = MinDens;
	    }
	  else
	    {
	      // There is too little, use uniform density all the way.
	      L0 = 0.0;
	      extra = LengthPrArea / Depth;
	    }
	}
    }

  const size_t size = geo.cell_size ();
  daisy_assert (Density.size () == size);
  for (size_t i = 0; i < size; i++)
    {
      const double f = geo.fraction_in_z_interval (i, 0.0, -Depth);
      const double d = -geo.cell_z (i);
      if (f > 0.01)
        Density[i] = f * (extra + L0 * exp (- a * d));
      else
        Density[i] = 0.0;
    }
}

void 
Rootdens_G_P::output (Log& log) const
{
  output_variable (a, log); 
  output_variable (L0, log); 
}

Rootdens_G_P::Rootdens_G_P (const BlockModel& al)
  : Rootdens (al),
    DensRtTip (al.number ("DensRtTip")),
    MinDens (al.number ("MinDens")),
    a (-42.42e42),
    L0 (-42.42e42)
{ }

std::unique_ptr<Rootdens> 
Rootdens::create_uniform (const Metalib& metalib, Treelog& msg)
{ return std::unique_ptr<Rootdens> 
    (Librarian::build_stock<Rootdens> (metalib, msg, "Gerwitz+Page74",
                                       "uniform")); }

static struct Rootdens_G_PSyntax : public DeclareModel
{
  Model* make (const BlockModel& al) const
  { return new Rootdens_G_P (al); }
  Rootdens_G_PSyntax ()
    : DeclareModel (Rootdens::component, "Gerwitz+Page74", 
	       "Use exponential function for root density.")
  { }
  void load_frame (Frame& frame) const
  {
    frame.set_strings ("cite", "gp74");
    frame.declare ("DensRtTip", "cm/cm^3", Check::positive (), Attribute::Const,
                "Root density at (potential) penetration depth.");
    frame.set ("DensRtTip", 0.1);
    frame.declare ("MinDens", "cm/cm^3", Check::non_negative (), Attribute::Const,
                "Minimal root density\n\
Root density will never be below this, as long as there is enough root mass.\n \
Extra root mass will be distributed according to Gerwitz and Page.\n\
If there are too little root mass, the root will have the same density\n\
all the way down.");
    frame.set ("MinDens", 0.0);
    frame.declare ("a", "cm^-1", Attribute::LogOnly, "Form parameter.\n\
Calculated from 'DensRtTip'.");
    frame.declare ("L0", "cm/cm^3", Attribute::LogOnly,
                "Root density at soil surface.");
  }
} Rootdens_G_P_syntax;

// rootdens_G_P.C ends here.
