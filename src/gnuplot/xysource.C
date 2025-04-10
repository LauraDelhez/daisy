// xysource.C -- 2D Data series for gnuplot interface 
// 
// Copyright 2005 Per Abrahamsen and KVL.
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

#include "gnuplot/xysource.h"
#include "object_model/block_model.h"
#include "util/assertion.h"
#include "object_model/librarian.h"

const char *const XYSource::component = "xysource";

symbol
XYSource::library_id () const
{
  static const symbol id (component);
  return id;
}

const std::vector<double>& 
XYSource::xbar () const
{ 
  static const std::vector<double> empty;
  return empty;
}

const std::vector<double>& 
XYSource::ybar () const
{ 
  static const std::vector<double> empty;
  return empty;
}

void 
XYSource::limit (double& xmin, double& xmax, double& ymin, double& ymax) const
{
  const std::vector<double>& xs = x ();
  const std::vector<double>& ys = y ();
  daisy_assert (xs.size () == ys.size ());

  for (size_t i = 0; i < xs.size (); i++)
    {
      if (xs[i] < xmin)
	xmin = xs[i];
      if (xs[i] > xmax)
	xmax = xs[i];
      if (ys[i] < ymin)
	ymin = ys[i];
      if (ys[i] > ymax)
	ymax = ys[i];
    }
}

void 
XYSource::distance (const double xmin, const double xmax, 
                    const double ymin, const double ymax,
                    double& nw, double& ne, double& sw, double& se) const
  // Find relative distances to each corner.
{
  if (xmin >= xmax || ymin >= ymax)
    // Null plot.
    return;
    
  const std::vector<double>& xs = x ();
  const std::vector<double>& ys = y ();
  daisy_assert (xs.size () == ys.size ());

  for (size_t i = 0; i < xs.size (); i++)
    {
      const double xr = (xs[i] - xmin) / (xmax - xmin);
      if (xr < 0.0 || xr > 1.0)
	// Outside graph.
	continue;
      const double yr = (ys[i] - ymin) / (ymax - ymin);
      if (yr < 0.0 || yr > 1.0)
	// Outside graph.
	continue;
      
      // Distance from borders.
      const double  west = xr;
      const double  east = 1.0 - xr;
      const double north = 1.0 - yr;
      const double south = yr;
      
      // Distance from corners.
      nw = std::min (nw, std::max (north, west));
      ne = std::min (ne, std::max (north, east));
      sw = std::min (sw, std::max (south, west));
      se = std::min (se, std::max (south, east));
    }
}

XYSource::XYSource (const BlockModel& al)
  : objid (al.type_name ())
{ }

XYSource::~XYSource ()
{ }

static struct XYSourceInit : public DeclareComponent 
{
  void load_frame (Frame& frame) const
  { Model::load_model (frame); }
  XYSourceInit ()
    : DeclareComponent (XYSource::component, "\
XY data series.")
  { }
} XYSource_init;
