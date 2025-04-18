// log_all.C
// 
// Copyright 2003 Per Abrahamsen and KVL.
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

#include "daisy/output/log_all.h"
#include "daisy/output/select.h"
#include "object_model/metalib.h"
#include "object_model/library.h"
#include "object_model/block_model.h"
#include "object_model/treelog.h"
#include "util/assertion.h"
#include "daisy/field.h"
#include <sstream>
#include "daisy/column.h"

bool 
LogAll::check_leaf (symbol name) const
{ 
  daisy_assert (is_active);
  daisy_assert (!active_leafs.empty ());

  const std::vector<Select*>& current = active_leafs.top ();
  for (std::vector<Select*>::const_iterator i = current.begin (); 
       i != current.end (); 
       i++)
    if (name == (*i)->current_name)
      return true;

  return false;
}

bool 
LogAll::check_interior (symbol name) const
{ 
  daisy_assert (is_active);
  daisy_assert (!active_interiors.empty ());

  const std::vector<Select*>& current = active_interiors.top ();
  for (std::vector<Select*>::const_iterator i = current.begin (); 
       i != current.end (); 
       i++)
    if ((*i)->valid (name))
      return true;

  return false;
}

void
LogAll::insert_active ()
{
  daisy_assert (active_leafs.empty ());
  active_leafs.push (std::vector<Select*> ());
  std::vector<Select*>& leafs = active_leafs.top ();
  daisy_assert (leafs.size () == 0);
  daisy_assert (active_interiors.empty ());
  active_interiors.push (std::vector<Select*> ());
  std::vector<Select*>& interiors = active_interiors.top ();
  daisy_assert (interiors.size () == 0);

  const int depth = 0;
  for (std::vector<Select*>::const_iterator i = entries.begin (); 
       i != entries.end (); 
       i++)
    {
      if ((*i)->is_active)
        {
          if (depth == (*i)->last_index)
            leafs.push_back (*i);
          else
            interiors.push_back (*i);
        }
    }
}


bool 
LogAll::match (const Daisy& daisy, Treelog& out)
{
  msg = &out;
  is_active = false;
  for (std::vector<LogSelect*>::const_iterator i = slaves.begin (); 
       i != slaves.end (); 
       i++)
    if ((*i)->match (daisy, out))
      is_active = true;

  if (is_active)
    insert_active ();

  return is_active;
}

void 
LogAll::done (const std::vector<Time::component_t>& time_columns,
	      const Time& time, const double dt, Treelog& out)
{
  msg = NULL;
  for (std::vector<LogSelect*>::const_iterator i = slaves.begin (); 
       i != slaves.end (); 
       i++)
    if ((*i)->is_active)
      (*i)->done (time_columns, time, dt, out);

  active_leafs.pop ();
  active_interiors.pop ();
}

bool 
LogAll::initial_match (const Daisy& daisy, const Time& previous, Treelog& out)
{
  msg = &out;
  is_active = false;
  for (std::vector<LogSelect*>::const_iterator i = slaves.begin (); 
       i != slaves.end (); 
       i++)
    if ((*i)->initial_match (daisy, previous, out))
      is_active = true;

  if (is_active)
    insert_active ();
  
  return is_active;
}

void 
LogAll::initial_done (const std::vector<Time::component_t>& time_columns,
		      const Time& time, Treelog& out)
{
  msg = NULL;
  for (std::vector<LogSelect*>::const_iterator i = slaves.begin (); 
       i != slaves.end (); 
       i++)
    if ((*i)->is_active)
      (*i)->initial_done (time_columns, time, out);

  active_leafs.pop ();
  active_interiors.pop ();
}

void 
LogAll::done_print (const std::vector<Time::component_t>& time_columns,
              const Time& time)
{ daisy_notreached (); }

void 
LogAll::open_derived_type (const symbol key, const symbol component)
{ 
  daisy_assert (is_active);

  const int depth = active_interiors.size ();
  daisy_assert (depth > 0);
  daisy_assert (depth == active_leafs.size ());

  const std::vector<Select*>& old = active_interiors.top ();
  active_interiors.push (std::vector<Select*> ());
  std::vector<Select*>& interiors = active_interiors.top ();
  active_leafs.push (std::vector<Select*> ());
  std::vector<Select*>& leafs = active_leafs.top ();

  const Library& library = metalib ().library (component);
  const std::set<symbol>& ancestors = library.ancestors (key);

  for (std::vector<Select*>::const_iterator i = old.begin (); 
       i != old.end (); 
       i++)
    if ((*i)->open (ancestors, depth))
      {
        if ((*i)->last_index == depth)
          leafs.push_back (*i);
        else
          interiors.push_back (*i);
      }
}

void 
LogAll::open (const symbol name)
{ 
  daisy_assert (is_active);

  const int depth = active_interiors.size ();
  daisy_assert (depth > 0);
  daisy_assert (depth == active_leafs.size ());

  const std::vector<Select*>& old = active_interiors.top ();
  active_interiors.push (std::vector<Select*> ());
  std::vector<Select*>& interiors = active_interiors.top ();
  active_leafs.push (std::vector<Select*> ());
  std::vector<Select*>& leafs = active_leafs.top ();

  for (std::vector<Select*>::const_iterator i = old.begin (); 
       i != old.end (); 
       i++)
    if ((*i)->open (name, depth))
      {
        if ((*i)->last_index == depth)
          leafs.push_back (*i);
        else
          interiors.push_back (*i);
      }
}

void 
LogAll::close ()
{ 
  const int depth = active_interiors.size () - 2;
  daisy_assert (depth >= 0);

  const std::vector<Select*>& leafs = active_leafs.top ();
  for (std::vector<Select*>::const_iterator i = leafs.begin (); 
       i != leafs.end (); 
       i++)
    (*i)->close (depth);
  const std::vector<Select*>& interiors = active_interiors.top ();
  for (std::vector<Select*>::const_iterator i = interiors.begin (); 
       i != interiors.end (); 
       i++)
    (*i)->close (depth);

  active_leafs.pop ();
  active_interiors.pop ();
}


void 
LogAll::column_clear ()
{
  column_entries.clear ();
  for (std::vector<Select*>::const_iterator i = entries.begin (); 
       i != entries.end (); 
       i++)
    {
      Select& select = **i;
      select.total_weight = 0.0;
    }
}

void 
LogAll::column_add_to_total (const Column& column)
{
  static const symbol collib (Column::component);
  const Library& library = metalib ().library (collib);
  const std::set<symbol>& ancestors = library.ancestors (column.objid);
  
  for (std::vector<Select*>::const_iterator i = entries.begin (); 
       i != entries.end (); 
       i++)
    {
      Select& select = **i;
      if (select.path.size () < 2)
        continue;
      if (select.path[0] != collib)
        continue;
      if (select.path[1] != Select::wildcard
          && ancestors.find (select.path[1]) == ancestors.end ())
        continue;
      column_entries[&column].push_back (&select);
      select.total_weight += column.area;
    }
}

void 
LogAll::column_select (const Column& column)
{
  const std::vector<Select*>& colents = column_entries[&column];
  // const std::vector<Select*>& colents = active_interiors.top ();
  for (std::vector<Select*>::const_iterator i = colents.begin (); 
       i != colents.end (); 
       i++)
    {
      Select& select = **i;
      select.set_column (column, *msg);
      // daisy_assert (select.total_weight > 0.0);
      if (select.total_weight > 0.0)
        select.relative_weight = column.area / select.total_weight;
      else
        select.relative_weight = 1.0;
    }
}

void 
LogAll::output_entry (symbol, const bool)
{ }

void 
LogAll::output_entry (symbol name, const double value)
{ 
  const std::vector<Select*>& sels = active_leafs.top ();

  for (std::vector<Select*>::const_iterator i = sels.begin ();
       i != sels.end ();
       i++)
    if (name == (*i)->current_name)
      (*i)->output_number (value);
}

void 
LogAll::output_entry (symbol name, const int value)
{ 
  const std::vector<Select*>& sels = active_leafs.top ();

  for (std::vector<Select*>::const_iterator i = sels.begin ();
       i != sels.end ();
       i++)
    if (name == (*i)->current_name)
      (*i)->output_integer (value);
}

void 
LogAll::output_entry (symbol name, const symbol value)
{ 
  const std::vector<Select*>& sels = active_leafs.top ();

  for (std::vector<Select*>::const_iterator i = sels.begin ();
       i != sels.end ();
       i++)
    if (name == (*i)->current_name)
      (*i)->output_name (value);
}

void 
LogAll::output_entry (symbol name, const std::vector<double>& value)
{ 
  const std::vector<Select*>& sels = active_leafs.top ();

  for (std::vector<Select*>::const_iterator i = sels.begin ();
       i != sels.end ();
       i++)
    if (name == (*i)->current_name)
      (*i)->output_array (value);
}

void 
LogAll::output_entry (symbol, const PLF&)
{ }

void
LogAll::initialize (const symbol, const symbol, Treelog&)
{ }

void
LogAll::attach_log (LogSelect *const log)
{
  slaves.push_back (log);
  for (unsigned int j = 0; j < log->entries.size (); j++)
    entries.push_back (log->entries[j]);
}

LogAll::LogAll (const std::vector<Log*>& logs)
  : LogSelect ("LogAll"),
    msg (NULL)
{
  // Combine entries.
  for (unsigned int i = 0; i != logs.size (); i++)
    if (LogSelect* log = dynamic_cast<LogSelect*> (logs[i]))
      attach_log (log);
}

LogAll::~LogAll ()
{ 
  // Don't delete entries twice.
  entries.erase (entries.begin (), entries.end ());
}

// log_all.C ends here.
