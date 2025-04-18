// depth.C --- Depth as a function of time.
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

#include "util/depth.h"
#include "object_model/block_model.h"
#include "daisy/daisy_time.h"
#include "object_model/plf.h"
#include "util/lexer_data.h"
#include "daisy/output/output.h"
#include "object_model/parameter_types/number.h"
#include "object_model/treelog.h"
#include "object_model/units.h"
#include "object_model/check.h"
#include "object_model/vcheck.h"
#include "util/assertion.h"
#include "object_model/librarian.h"
#include "util/mathlib.h"
#include "util/path.h"
#include "object_model/frame_submodel.h"
#include "util/lexer_table.h"
#include <string>
#include <sstream>

// depth component.

const char *const Depth::component = "depth";

symbol
Depth::library_id () const
{
  static const symbol id (component);
  return id;
}

Depth::Depth (const BlockModel& al)
  : name (al.type_name ())
{ }

Depth::Depth (const symbol n)
  : name (n)
{ }

Depth::~Depth ()
{ }

static struct DepthInit : public DeclareComponent
{
  DepthInit ()
    : DeclareComponent (Depth::component, "\
Depth below soil surface.")
  { }
} Depth_init;

// const model.
void DepthConst::tick (const Time&, const Scope&, Treelog&)
{ }
double DepthConst::operator()() const
{ return value; }
void DepthConst::initialize (const Time&, const Scope&, Treelog&)
{ }
bool DepthConst::check (const Scope&, Treelog&) const
{ return true; }
DepthConst::DepthConst (const BlockModel& al)
  : Depth (al),
    value (al.number ("value"))
{ }
DepthConst::DepthConst (const double height)
  : Depth (symbol ("create")),
    value (height)
{ }
DepthConst::~DepthConst ()
{ }


Depth*
Depth::create (const double height)
{ return new DepthConst (height); }

static struct DepthConstSyntax : public DeclareModel
{
  Model* make (const BlockModel& al) const
  { return new DepthConst (al); }
  DepthConstSyntax ()
    : DeclareModel (Depth::component, "const", "Constant depth.")
  { }
  void load_frame (Frame& frame) const
  {
    frame.declare ("value", "cm", Check::non_positive (), Attribute::Const,
                "Constant depth.");
    frame.order ("value");
  }
} DepthConst_syntax;

// deep param.

static struct DepthDeepSyntax : public DeclareParam
{
  DepthDeepSyntax ()
    : DeclareParam (Depth::component, "deep", "const", "\
Way down.")
  { }
  void load_frame (Frame& frame) const
  {
    frame.set ("value", -1000.0 * 100.0); // 1 km below ground.
  }
} DepthDeep_syntax;

// extern model.

void DepthExtern::tick (const Time&, const Scope& scope, Treelog& msg)
{
  if (!expr->tick_value (units, value, Units::cm (), scope, msg))
    if (!approximate (value, 42.0))
      {
        msg.error ("External depth not found");
        value = 42.0;
      }
}

double DepthExtern::operator()() const
{ return value; }

void DepthExtern::initialize (const Time&, const Scope& scope, Treelog& msg)
{ expr->initialize (units, scope, msg); }

bool DepthExtern::check (const Scope& scope, Treelog& msg) const
{

  bool ok = true;
  if (!expr->check_dim (units, scope, Units::cm (), msg))
    ok = false;
  return ok;
}
DepthExtern::DepthExtern (symbol name, const Units& units, Number* expr, double value)
  : Depth (name),
    units (units),
    expr (expr),
    value (value)
{ }
DepthExtern::DepthExtern (const BlockModel& al)
  : DepthExtern (al.type_name (),
                 al.units (),
                 Librarian::build_item<Number> (al, "value"),
                 al.number ("initial_value", -42.42e42))
{ }
DepthExtern::~DepthExtern ()
{ }

static struct DepthExternSyntax : public DeclareModel
{
  Model* make (const BlockModel& al) const
  { return new DepthExtern (al); }
  DepthExternSyntax ()
    : DeclareModel (Depth::component, "extern", "\
Look up depth in an scope.")
  { }
  void load_frame (Frame& frame) const
  {
    frame.declare_object ("value", Number::component,
                       Attribute::Const, Attribute::Singleton, "\
Expression that evaluates to a depth.");
    frame.declare ("initial_value", "cm", Check::none (), Attribute::OptionalConst,
		"Initial depth.");

  }
} DepthExtern_syntax;

// PLF model.
void DepthPLF::tick (const Time& time, const Scope&, Treelog&)
{ current_value = value (Time::fraction_hours_between (start, time)); }

double DepthPLF::operator()() const
{ return current_value; }


void DepthPLF::initialize (const Time& time, const Scope& scope, Treelog& msg)
{ tick (time, scope, msg); }
bool DepthPLF::check (const Scope&, Treelog&) const
{ return true; }
PLF DepthPLF::convert_to_plf (const std::vector<boost::shared_ptr<const FrameSubmodel>/**/>& table)
{
  daisy_assert (table.size () > 0);
  const Time start (table[0]->submodel ("time"));
  PLF result;
  for (size_t i = 0; i < table.size (); i++)
    {
      const Time now (table[i]->submodel ("time"));
      const double value = table[i]->number ("value");
      result.add (Time::fraction_hours_between (start, now), value);
    }
  return result;
}
DepthPLF::DepthPLF (const BlockModel& al)
  : Depth (al),
    start (al.submodel_sequence ("table")[0]->submodel ("time")),
    value (convert_to_plf (al.submodel_sequence ("table"))),
    current_value (-42.42e42)
{ }
DepthPLF::~DepthPLF ()
{ }


// GCC 2.95 can't link if this class is nested.
static const class CheckTable : public VCheck
{
  bool verify (const Metalib&, const Frame& frame, const symbol key,
               Treelog& msg) const
  {
    daisy_assert (frame.check (key));

    const std::vector<boost::shared_ptr<const FrameSubmodel>/**/>& table
      = frame.submodel_sequence (key);
    if (table.size () < 2)
      {
        msg.error ("You must list at least two entries");
        return false;
      }
    Time last (table[0]->submodel ("time"));
    for (size_t i = 1; i < table.size (); i++)
      {
        Time next ((table[i]->submodel ("time")));
        if (next <= last)
          {
            msg.error ("Time must be increasing");
            return false;
          }
        last = next;
      }
    return true;
  }
 public:
  CheckTable ()
  { }
} check_table;

static struct DepthPLFSyntax : public DeclareModel
{
  static void entry_syntax (Frame& frame)
  {
    frame.declare_submodule ("time", Attribute::Const, "Time.",
                          Time::load_syntax);
    frame.declare ("value", "cm", Check::non_positive (), Attribute::Const,
                "Depth.");
    frame.order ("time", "value");
  }
  Model* make (const BlockModel& al) const
  { return new DepthPLF (al); }
  DepthPLFSyntax ()
    : DeclareModel (Depth::component, "PLF", "Linear interpolation of depth.")
  { }
  void load_frame (Frame& frame) const
  {
    frame.declare_submodule_sequence ("table", Attribute::Const,
                          "Height as a function of time.\n\
This is a list where each element has the form (TIME VALUE).\n\
The TIME entries must be increasing cronologically.  The corresponding\n\
VALUE represents the value at that time.  In order to find\n\
the depth for other times, linear interpolation between the entries\n\
in the list will be used.", entry_syntax);
    frame.set_check ("table", check_table);
    frame.order ("table");
  }
} DepthPLF_syntax;

// The 'season' model.

void DepthSeason::tick (const Time& time, const Scope&, Treelog&)
{
  const Time newyear (time.year (), 1, 1, 0);
  const double jday
    = Time::fraction_hours_between (newyear, time) / 24.0 + 1.0;
  daisy_assert (jday >= 1.0);
  daisy_assert (jday <= 367.0);
  current_value = season (jday);
}

double DepthSeason::operator()() const
{ return current_value; }


void DepthSeason::initialize (const Time& time, const Scope& scope, Treelog& msg)
{ tick (time, scope, msg); }
bool DepthSeason::check (const Scope&, Treelog&) const
{ return true; }
DepthSeason::DepthSeason (const BlockModel& al)
  : Depth (al),
    season (al.plf ("season")),
    current_value (-42.42e42)
{ }
DepthSeason::~DepthSeason ()
{ }

static struct DepthSeasonSyntax : public DeclareModel
{
  Model* make (const BlockModel& al) const
  { return new DepthSeason (al); }
  DepthSeasonSyntax ()
    : DeclareModel (Depth::component, "season", "\
Linear interpolation of depth within a year.")
  { }
  void load_frame (Frame& frame) const
  {
    frame.declare ("season", "d", "cm", Attribute::Const,
                   "Depth as a function of Julian day.\n\
First and last entry must be identical.");
    frame.set_check ("season", VCheck::season ());
    frame.order ("season");
  }
} DepthSeason_syntax;

// file model.

void DepthFile::tick (const Time& time, const Scope&, Treelog&)
{
  daisy_assert (state == State::ok);
  current_value = value (Time::fraction_hours_between (start, time));
}
double DepthFile::operator()() const
{ return current_value; }
bool DepthFile::read_date (LexerData& lex, Time& time)
{
  int year;
  int month;
  int day;

  year = lex.get_cardinal ();
  if (year < 0 || year > 9999)
	lex.error ("Bad year");
  if (year < 100)
	year += 1900;
  lex.skip_space ();
  month = lex.get_cardinal ();
  if (month < 1 || month > 12)
	lex.error ("Bad month");
  lex.skip_space ();
  day = lex.get_cardinal ();
  if (day < 1 || day > 31)
	lex.error ("Bad day");

  if (!Time::valid (year, month, day, 23))
	{
	  lex.error ("Bad date");
      return false;
    }
  time = Time (year, month, day, 23);
  return true;
}
void DepthFile::initialize (const Time& time, const Scope& scope, Treelog& msg)
{
  daisy_assert (state == State::uninitialized);
  std::unique_ptr<std::istream> input_stream = path.open_file (file.name ());
  LexerData lex (file.name (), *input_stream, msg);
  lex.skip_space ();
  if (lex.peek () == '#')
    {
      lex.skip_line ();
      lex.next_line ();
    }

  // Start.
  read_date (lex, start);
  lex.skip_space ();
  value.add (0, lex.get_number ());
  lex.next_line ();

  int lines = 1;
  Time last = start;
  while (lex.good ())
    {
      Time next (1,1,1,1);
      if (!read_date (lex, next))
        lex.skip_line ();
      else
        {
          lex.skip_space ();
          const double height = lex.get_number ();
          if (next <= last)
            lex.error ("Time should be increasing");
          else if (height >= 0.0)
            lex.error ("Height should be negative");
          else
            {
              value.add (Time::fraction_hours_between (start, next), height);
              last = next;
              lines++;
            }
        }
      lex.next_line ();
    }
  lex.eof ();
  if (lex.get_error_count () > 0)
    state = State::error;
  else if (lines < 2)
    {
      msg.error ("There should be at least two entries");
      state = State::error;
    }
  else
    state = State::ok;

  if (state == State::ok)
    tick (time, scope, msg);
}
bool DepthFile::check (const Scope&, Treelog&) const
{ return state == State::ok; }
DepthFile::DepthFile (const BlockModel& al)
  : Depth (al),
    path (al.path ()),
    file (al.name ("file")),
    state (State::uninitialized),
    start (1, 1, 1, 1),
    current_value (-42.42e42)
{ }
DepthFile::~DepthFile ()
{ }

static struct DepthFileSyntax : public DeclareModel
{
  Model* make (const BlockModel& al) const
  { return new DepthFile (al); }

  DepthFileSyntax ()
    : DeclareModel (Depth::component, "file", "Linear interpolation of depth read from file.")
  { }
  void load_frame (Frame& frame) const
  {
    frame.declare_string ("file", Attribute::Const,
                "Name of file to read data from.\n\
The format of each line in the file is 'YEAR MONTH DAY HEIGHT',\n\
where HEIGHT should in cm above ground (i.e. a negative number).\n\
Linear interpolation is used between the datapoints.");
    frame.order ("file");
  }
} DepthFile_syntax;

// table model.


void DepthTable::tick (const Time& time, const Scope&, Treelog& msg)
{
  read_line (time);

  switch (entry_count)
    {
    case -2:
      // End of file.
      break;
    case -1:
      daisy_panic ("uninitialized");
    case 0:
      msg.error ("No depth data, using 0 cm");
      current_value = 0.0;
      entry_count = -2;
      break;
    case 1:
      current_value = next_value;
      break;
    default:
      // Interpolate.
      daisy_assert (prev_time < next_time);
      daisy_assert (prev_time <= time);
      daisy_assert (time <= next_time);
      const double fraction_time
        = Time::fraction_hours_between (prev_time, time)
        / Time::fraction_hours_between (prev_time, next_time);;
      daisy_assert (fraction_time >= 0.0);
      daisy_assert (fraction_time <= 1.0);
      current_value = prev_value + (next_value - prev_value) * fraction_time;
    }
}

double DepthTable::operator()() const
{
  return current_value + offset;
}

void DepthTable::read_line (const Time& time)
{
  std::vector<std::string> entries;

  while (next_time < time)
    {

      prev_time = next_time;
      prev_value = next_value;

      if (!lex.good ())
        {
          if (entry_count > 0 )
            {
              lex.error ("End of file, reusing last value");
              entry_count = -2;
              current_value = next_value;
            }
          return;
        }
      if (!lex.get_entries (entries))
        continue;

      if (!lex.get_time_dh (entries, next_time, 12))
        continue;

      entry_count++;

      if (entry_count > 1 && next_time <= prev_time)
        {
          std::ostringstream tmp;
          tmp << "Entries should be in chronological order, "
              << prev_time.print ()
              << " is not before " << next_time.print ();
          lex.error (tmp.str ());
        }

      next_value = lex.convert_to_double (entries[level_c]);
      if (units.can_convert (file_unit, Units::cm (), next_value))
        next_value = units.convert (file_unit, Units::cm (), next_value);
      else
        {
          std::ostringstream tmp;
          tmp << "Can't convert " << next_value << "[" << file_unit
              << "] to [cm]";
          lex.error (tmp.str ());
        }
    }
}

void DepthTable::initialize (const Time& time, const Scope& scope, Treelog& msg)
{
  TREELOG_SUBMODEL (msg, "depth table");
  daisy_assert (entry_count == -1);

  if (!lex.read_header (msg))
    return;

  level_c = lex.find_tag ("Level");
  if (level_c < 0)
    {
      lex.error ("'Level' not found");
      return;
    }
  file_unit = lex.dimension (level_c);
  if (!units.can_convert (file_unit, Units::cm (), msg))
    return;
  entry_count = 0;
  tick (time, scope, msg);
}
bool DepthTable::check (const Scope&, Treelog&) const
{ return entry_count >= 0; }

DepthTable::DepthTable (const BlockModel& al)
  : Depth (al),
    units (al.units ()),
    offset (al.number ("offset")),
    lex (al),
    level_c (-1),
    file_unit (Units::cm ()),
    entry_count (-1),
    prev_time (9999,1,1,1),
    prev_value (-42.42e42),
    next_time (1,1,1,1),
    next_value (42.42e42),
    current_value (-100.0)
{ }
DepthTable::~DepthTable ()
{ }

static struct DepthTableSyntax : public DeclareModel
{
  Model* make (const BlockModel& al) const
  { return new DepthTable (al); }

  DepthTableSyntax ()
    : DeclareModel (Depth::component, "table", "\
Linear interpolation of depth read from file.")
  { }
  void load_frame (Frame& frame) const
  {
    LexerTable::load_syntax (frame);
    frame.declare ("offset", Units::cm (), Attribute::Const, "\
Add this number to the Level read from the table.");
    frame.set ("offset", 0.0);
  }
} DepthTable_syntax;

// depth.C ends here.
