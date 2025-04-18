// output.C -- Handle output from a Daisy simulation.
// 
// Copyright 2007 Per Abrahamsen and KVL.
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

#include "daisy/output/output.h"
#include "daisy/daisy.h"
#include "daisy/output/log_all.h"
#include "object_model/treelog.h"
#include "daisy/daisy_time.h"
#include "daisy/timestep.h"
#include "object_model/block_model.h"
#include "object_model/frame.h"
#include "util/assertion.h"
#include "object_model/librarian.h"
#include "util/scope_model.h"

void
Output::initial_logs (const Daisy& daisy, const Time& previous, Treelog& msg)
{
  activate_output->tick (daisy, Scope::null (), msg);

  if (activate_output->match (daisy, Scope::null (), msg))
    {
      if (!logging)
	{
	  msg.message ("Start logging");
	  // get initial values for previous day.
	  for (size_t i = 0; i < active_logs.size (); i++)
	    {
	      Log& log = *active_logs[i];
	      if (log.initial_match (daisy, previous, msg))
		{
		  output_submodule (previous, "time", log);
                  daisy.output (log);
                  output_list (logs, "output", log, Log::component);
		  log.initial_done (time_columns, previous, msg);
		}
	    }
	}
      logging = true;
    }
  else if (logging)
    {
      msg.message ("End logging");
      logging = false;
    }
}

void
Output::tick (const Daisy& daisy, const Time& time, const double dt, 
              Treelog& msg)
{
  if (logging)
    {
      for (size_t i = 0; i < active_logs.size (); i++)
	{
	  Log& log = *active_logs[i];
	  if (log.match (daisy, msg))
	    {
              daisy.output (log);
              output_list (logs, "output", log, Log::component);
	      log.done (time_columns, time, dt, msg);
	    }
	}
    }
}

void
Output::summarize (Treelog& msg) const
{
  // Print log file summaries at end of simulation.
  {
    Treelog::Open nest (msg, "Summary");
    for (size_t i = 0; i < logs.size (); i++)
      logs[i]->summarize (msg);
  }
}

size_t
Output::scope_size () const
{ return my_scopes.size (); }

const Scope&
Output::scope (size_t i) const
{ 
  daisy_assert (i < my_scopes.size ());
  return *my_scopes[i];
}

const std::vector<const Scope*>& 
Output::scopes () const
{ return my_scopes; }

bool
Output::check (const Border& field, Treelog& msg)
{
  bool ok = true;

  for (std::vector<Log*>::const_iterator i = logs.begin ();
       i != logs.end ();
       i++)
    {
      if (*i == NULL || !(*i)-> check (field, msg))
        ok = false;
    }

  return ok;
}

void
Output::initialize (const Metalib& metalib, Treelog& msg)
{
  for (size_t i = 0; i < logs.size (); i++)
    logs[i]->initialize_common (log_prefix, log_suffix, metalib, msg);
  log_all->initialize_common (log_prefix, log_suffix, metalib, msg);
}

void 
Output::add_log (Log* log)
{ 
  if (LogSelect* sel = dynamic_cast<LogSelect*> (log))
    log_all->attach_log (sel);
  else
    active_logs.push_back (log); 
}

const std::vector<Log*> 
Output::find_active_logs (const std::vector<Log*>& logs, LogAll& log_all)
{
  std::vector<Log*> result;

  for (size_t i = 0; i < logs.size (); i++)
    if (!dynamic_cast<LogSelect*> (logs[i]))
      result.push_back (logs[i]);
  
  result.push_back (&log_all);
  
  return result;
}

const std::vector<const Scope*> 
Output::find_extern_logs (const std::vector<Log*>& logs, 
                          const std::vector<MScope*>& exchanges)
{
  std::vector<const Scope*> result;

  for (size_t i = 0; i < exchanges.size (); i++)
    result.push_back (exchanges[i]);

  for (size_t i = 0; i < logs.size (); i++)
    logs[i]->find_scopes (result);
  
  return result;
}

Output::Output (const BlockModel& al)
  : logging (false),
    exchanges (Librarian::build_vector<MScope> (al, "exchange")),
    logs (Librarian::build_vector<Log> (al, "output")),
    log_all (new LogAll (logs)),
    active_logs (find_active_logs (logs, *log_all)),
    my_scopes (find_extern_logs (logs, exchanges)),
    activate_output (Librarian::build_item<Condition> (al, "activate_output")),
    time_columns (Time::find_time_components 
                  /**/ (al.name_sequence ("log_time_columns"))),
    log_prefix (al.name ("log_prefix")),
    log_suffix (al.name ("log_suffix"))
{ }

Output::Output ()
  : logging (false),
    exchanges (std::vector<MScope*> ()),
    logs (std::vector<Log*> ()),
    active_logs (std::vector<Log*> ()),
    my_scopes (std::vector<const Scope*> ()),
    log_prefix (""),
    log_suffix ("")
{ }

Output::~Output ()
{ }

void
Output::load_syntax (Frame& frame)
{
  frame.declare_object ("output", Log::component,
                     Attribute::State, Attribute::Variable,
                     "List of logs for output during the simulation.");
  frame.declare_object ("activate_output", Condition::component,
                     "Activate output logs when this condition is true.\n\
You can use the 'after' condition to avoid logging during an initialization\n\
period.");
  frame.set ("activate_output", "true");

  frame.declare_object ("exchange", MScope::component,
                     Attribute::Const, Attribute::Variable, "\
List of exchange items for communicating with external models.");
  frame.set_empty ("exchange");

  // The log_time paramater.
  Time::declare_time_components (frame, "log_time_columns", Attribute::Const, "\
List of default time components to include in log files.");

  std::vector<symbol> default_time;
  default_time.push_back (symbol ("year"));
  default_time.push_back (symbol ("month"));
  default_time.push_back (symbol ("mday"));
  default_time.push_back (symbol ("hour"));
  frame.set ("log_time_columns", default_time);

   frame.declare_string ("log_prefix", Attribute::Const, "\
Prefix for log file names.  Set it to 'log/' to put all files in a subdir.");
   frame.set ("log_prefix", "");
   frame.declare_string ("log_suffix", Attribute::Const, "\
Suffix for log file names.  Set it to '.csv' to make Excel happy.");
   frame.set ("log_suffix", "");
}

// output.C ends here.
