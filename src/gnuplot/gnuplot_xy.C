// gnuplot_xy.C -- 2D plot with Daisy.
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
#include "gnuplot/gnuplot_base.h"
#include "object_model/block_model.h"
#include "gnuplot/xysource.h"
#include "object_model/treelog.h"
#include "util/mathlib.h"
#include "util/memutils.h"
#include "object_model/librarian.h"
#include "object_model/frame.h"
#include <sstream>

struct GnuplotXY : public GnuplotBase
{
  // Ranges.
  const bool xmin_flag;
  const double xmin;
  const bool xmax_flag;
  const double xmax;
  const bool x2min_flag;
  const double x2min;
  const bool x2max_flag;
  const double x2max;
  const bool ymin_flag;
  const double ymin;
  const bool ymax_flag;
  const double ymax;
  const bool y2min_flag;
  const double y2min;
  const bool y2max_flag;
  const double y2max;

  // Source.
  const std::vector<XYSource*> source;

  // Use.
  bool initialize (const Units& units, Treelog& msg);
  bool plot (std::ostream& out, Treelog& msg);
  
  // Create and Destroy.
  explicit GnuplotXY (const BlockModel& al);
  ~GnuplotXY ();
};

bool
GnuplotXY::initialize (const Units& units, Treelog& msg)
{ 
  bool ok = true;
  for (size_t i = 0; i < source.size(); i++)
    {
      std::ostringstream tmp;
      tmp << objid << "[" << i << "]: " << source[i]->objid 
          << " '" << source[i]->title () << "'";
      Treelog::Open nest (msg, tmp.str ());
      if (!source[i]->load (units, msg))
        ok = false;
      else if (source[i]->x ().size () < 1)
        msg.error ("No data in plot, ignoring");
    }
  return ok;
}

static void adjust_range (double& min, double& max)
{
  if (max > min)
    return;
  
  const double delta = std::abs (max * 0.1);
  min -= delta;
  max += delta;

  if (max > min)
    return;

  std::swap (max, min);
  
  if (max > min)
    return;

  min -= 1.0;
  max += 1.0;
  
  if (max > min)
    return;

  min = -42e42;
  max = -42e42;

  daisy_assert (min < max);
}

bool
GnuplotXY::plot (std::ostream& out, Treelog& msg)
{ 
  // Header.
  plot_header (out);
  out << "\
set xtics nomirror\n\
set ytics nomirror\n\
set xdata\n\
set style data lines\n";

  // Dimensions.
  std::vector<symbol> x_dims;
  std::vector<int> x_axis;
  std::vector<symbol> y_dims;
  std::vector<int> y_axis;
  for (size_t i = 0; i < source.size (); i++)
    {
      if (source[i]->x ().size () < 1)
        {
          x_axis.push_back (-42);
          y_axis.push_back (-42);
          continue;
        }

      const symbol x_dim = source[i]->x_dimension ();
      
      for (size_t j = 0; j < x_dims.size (); j++)
        if (x_dim == x_dims[j])
          {
            x_axis.push_back (j);
            goto cont2;
          }
      x_axis.push_back (x_dims.size ());
      x_dims.push_back (x_dim);

    cont2: 
      const symbol y_dim = source[i]->y_dimension ();
      
      for (size_t j = 0; j < y_dims.size (); j++)
        if (y_dim == y_dims[j])
          {
            y_axis.push_back (j);
            goto cont3;
          }
      y_axis.push_back (y_dims.size ());
      y_dims.push_back (y_dim);
    cont3: 
      ;
    }
  bool ok = true;
  switch (x_dims.size ())
    {
    case 2:
      out << "set x2tics\n";
      out << "set x2label " << quote (x_dims[1]) << "\n";
      out << "set xlabel " << quote (x_dims[0]) << "\n";
      break;
    case 1:
      out << "unset x2tics\n";
      out << "unset x2label\n";
      out << "set xlabel " << quote (x_dims[0]) << "\n";
      break;
    case 0:
      msg.error ("Nothing to be plotted");
      return false;
    default:
      msg.error ("Can only plot one or two x units at a time");
      ok = false;
    }

  switch (y_dims.size ())
    {
    case 2:
      out << "set y2tics\n";
      out << "set y2label " << quote (y_dims[1]) << "\n";
      out << "set ylabel " << quote (y_dims[0]) << "\n";
      break;
    case 1:
      out << "unset y2tics\n";
      out << "unset y2label\n";
      out << "set ylabel " << quote (y_dims[0]) << "\n";
      break;
    default:
      msg.error ("Can only plot one or two y units at a time");
      ok = false;
    }
  if (!ok)
    return false;

  // Find ranges.
  double soft_xmin = 1e99;
  double soft_xmax = -soft_xmin;
  double soft_x2min = soft_xmin;
  double soft_x2max = soft_xmax;
  double soft_ymin = 1e99;
  double soft_ymax = -soft_ymin;
  double soft_y2min = soft_ymin;
  double soft_y2max = soft_ymax;

  for (size_t i = 0; i < source.size (); i++)
    if (source[i]->x ().size () < 1)
      /**/;
    else if (x_axis[i] == 0)
      if (y_axis[i] == 0)
        source[i]->limit (soft_xmin, soft_xmax, soft_ymin, soft_ymax);
      else
        source[i]->limit (soft_xmin, soft_xmax, soft_y2min, soft_y2max);
    else
      if (y_axis[i] == 0)
        source[i]->limit (soft_x2min, soft_x2max, soft_ymin, soft_ymax);
      else
        source[i]->limit (soft_x2min, soft_x2max, soft_y2min, soft_y2max);

  if (xmin_flag)
    soft_xmin = xmin;
  if (xmax_flag)
    soft_xmax = xmax;
  if (x2min_flag)
    soft_x2min = x2min;
  if (x2max_flag)
    soft_x2max = x2max;
  if (ymin_flag)
    soft_ymin = ymin;
  if (ymax_flag)
    soft_ymax = ymax;
  if (y2min_flag)
    soft_y2min = y2min;
  if (y2max_flag)
    soft_y2max = y2max;

  // Avoid empty range.
  adjust_range (soft_xmin, soft_xmax);
  adjust_range (soft_x2min, soft_x2max);
  adjust_range (soft_ymin, soft_ymax);
  adjust_range (soft_y2min, soft_y2max);

  // Legend.
  if (legend == "auto")
    {

      // Find distances.
      double nw = 1.0;
      double ne = 1.0;
      double sw = 1.0;
      double se = 1.0;
      for (size_t i = 0; i < source.size (); i++)
	if (source[i]->x ().size () < 1)
          /**/;
        else if (x_axis[i] == 0)
          if (y_axis[i] == 0)
            source[i]->distance (soft_xmin, soft_xmax, soft_ymin, soft_ymax,
                                 nw, ne, sw, se);
          else
            source[i]->distance (soft_xmin, soft_xmax, soft_y2min, soft_y2max,
                                 nw, ne, sw, se);
        else
          if (y_axis[i] == 0)
            source[i]->distance (soft_x2min, soft_x2max, soft_ymin, soft_ymax,
                                 nw, ne, sw, se);
          else
            source[i]->distance (soft_x2min, soft_x2max,
                                 soft_y2min, soft_y2max,
                                 nw, ne, sw, se);

      // Choose closest.
      const double max_distance = std::max (std::max (nw, ne), 
					    std::max (sw, se));
      if (max_distance < 0.05)
	legend = "outside";
      else if (approximate (max_distance, ne))
	legend = "ne";
      else if (approximate (max_distance, nw))
	legend = "nw";
      else if (approximate (max_distance, se))
	legend = "se";
      else
	{
	  daisy_assert (approximate (max_distance, sw));
	  legend = "sw";
	}
    }
  out << "set key " << legend_table[legend] << "\n";

  // X range
  out << "set xrange [" << soft_xmin << ":" << soft_xmax << "]\n";
  if (x_dims.size () == 2)
    out << "set x2range [" << soft_x2min << ":" << soft_x2max << "]\n";
  out << "set yrange [" << soft_ymin << ":" << soft_ymax << "]\n";
  if (y_dims.size () == 2)
    out << "set y2range [" << soft_y2min << ":" << soft_y2max << "]\n";

  // Extra.
  for (size_t i = 0; i < extra.size (); i++)
    out << extra[i].name () << "\n";

  // Plot.
  out << "plot ";
  int points = 0;
  int lines = 0;
  bool first = true;
  daisy_assert (x_axis.size () == source.size ());
  daisy_assert (y_axis.size () == source.size ());
  for (size_t i = 0; i < source.size (); i++)
    {
      if (source[i]->x ().size () < 1)
        continue;
      const symbol with = source[i]->with ();
      if (first)
        first = false;
      else
        out << ", ";
      out << "'-' using 1:2";
      if (with == "xerrorbars" || with == "yerrorbars")
	out << ":3";
      else if (with == "xyerrorbars")
	out << ":3:4";
      out << " title " << quote (source[i]->title ());
      out << " axes x" << x_axis[i] + 1 << "y" << y_axis[i] + 1;
      out << " with ";	
      const int style = source[i]->style ();
      out << with;
      if (with == "points" || with == "xerrorbars" || with == "yerrorbars" 
          || with == "xyerrorbars")
	out << " ls "
            << (style < 0 ? ++points : ((style == 0) ? points : style));
      else if (with == "lines")
	out << " ls "
            << (style < 0 ? ++lines :  ((style == 0) ? lines : style));
      else 
	{
	  if (style >= 0)
	    out << " ls " << style;
	}
    }
  out << "\n";
  
  // Data.
  for (size_t i = 0; i < source.size (); i++)
    {
      Treelog::Open nest (msg, source[i]->title (), i, source[i]->objid);
      if (source[i]->x ().size () < 1)
        continue;
      const size_t size = source[i]->x ().size ();
      const symbol with = source[i]->with ();
      daisy_assert (size == source[i]->y ().size ());
      if ((with == "xerrorbars" || with == "xyerrorbars") 
          &&  source[i]->xbar ().size () != size)
        {
          msg.error ("Bad x errorbars");
          return false;
        }
      if ((with == "yerrorbars" || with == "xyerrorbars") 
          &&  source[i]->ybar ().size () != size)
        {
          msg.error ("Bad y errorbars");
          return false;
        }
      for (size_t j = 0; j < size; j++)
        {
          out <<  source[i]->x ()[j]  << "\t" <<  source[i]->y ()[j];
          if (with == "xerrorbars" || with == "xyerrorbars")
            out << "\t" << source[i]->xbar ()[j];
          if (with == "yerrorbars" || with == "xyerrorbars")
            out << "\t" << source[i]->ybar ()[j];
          out << "\n";
        }
      out << "e\n";
    }

  // The end.
  if (device == "screen")
    out << "pause mouse\n";

  return true;
}

GnuplotXY::GnuplotXY (const BlockModel& al)
  : GnuplotBase (al),
    xmin_flag (al.check ("xmin")),
    xmin (al.number ("xmin", 42.42e42)),
    xmax_flag (al.check ("xmax")),
    xmax (al.number ("xmax", 42.42e42)),
    x2min_flag (al.check ("x2min")),
    x2min (al.number ("x2min", 42.42e42)),
    x2max_flag (al.check ("x2max")),
    x2max (al.number ("x2max", 42.42e42)),
    ymin_flag (al.check ("ymin")),
    ymin (al.number ("ymin", 42.42e42)),
    ymax_flag (al.check ("ymax")),
    ymax (al.number ("ymax", 42.42e42)),
    y2min_flag (al.check ("y2min")),
    y2min (al.number ("y2min", 42.42e42)),
    y2max_flag (al.check ("y2max")),
    y2max (al.number ("y2max", 42.42e42)),
    source (Librarian::build_vector<XYSource> (al, "source"))
{ }

GnuplotXY::~GnuplotXY ()
{ sequence_delete (source.begin (), source.end ()); }

static struct GnuplotXYSyntax : public DeclareModel
{
  Model* make (const BlockModel& al) const
  { return new GnuplotXY (al); }
  GnuplotXYSyntax ()
    : DeclareModel (Gnuplot::component, "xy", "common",
                    "Generate a gnuplot graph with up to two x-axes.")
  { }
  void load_frame (Frame& frame) const
  {
    frame.declare ("xmin", Attribute::User (), Attribute::OptionalConst, "\
Fixed lowest value on left x-axis.\n\
By default determine this from the data.");
    frame.declare ("xmax", Attribute::User (), Attribute::OptionalConst, "\
Fixed highest value on right x-axis.\n\
By default determine this from the data.");
    frame.declare ("x2min", Attribute::User (), Attribute::OptionalConst, "\
Fixed lowest value on left x-axis.\n\
By default determine this from the data.");
    frame.declare ("x2max", Attribute::User (), Attribute::OptionalConst, "\
Fixed highest value on right x-axis.\n\
By default determine this from the data.");
    frame.declare ("ymin", Attribute::User (), Attribute::OptionalConst, "\
Fixed lowest value on left y-axis.\n\
By default determine this from the data.");
    frame.declare ("ymax", Attribute::User (), Attribute::OptionalConst, "\
Fixed highest value on right y-axis.\n\
By default determine this from the data.");
    frame.declare ("y2min", Attribute::User (), Attribute::OptionalConst, "\
Fixed lowest value on left y-axis.\n\
By default determine this from the data.");
    frame.declare ("y2max", Attribute::User (), Attribute::OptionalConst, "\
Fixed highest value on right y-axis.\n\
By default determine this from the data.");
                
    frame.declare_object ("source", XYSource::component, Attribute::State, 
                       Attribute::Variable, "\
XY series to plot.");
  }
} GnuplotXY_syntax;
