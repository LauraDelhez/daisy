// assertion.C -- Managed assertions.
// 
// Copyright 2002 Per Abrahamsen and KVL.
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

#include "util/assertion.h"
#include "object_model/treelog.h"
#include "util/mathlib.h"
#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <map>

namespace Assertion
{
  static std::vector<Treelog*>& logs ()
  {
    static std::vector<Treelog*> logs;
    return logs;
  }

  // full counter
  std::map<std::string, int>* counter = NULL;
}

bool
Assertion::full (const char* file, const int line, bool is_debug)
{
  const int max_entries = 5;

  // Find entry.
  std::ostringstream tmp;
  tmp << file << ":" << line;
  const std::string entry = tmp.str ();

  // Prepare counter.
  if (counter == NULL)
    counter = new std::map<std::string, int>;

  if (counter->find (entry) == counter->end ())
    (*counter)[entry] = 0;

  // Update count.
  (*counter)[entry]++;
  
  if ((*counter)[entry] == max_entries)
    {
      if (is_debug)
	debug (entry + ": last message from this line");
      else
	warning (entry + ": one last message from this line");
    }

  return (*counter)[entry] > max_entries;
}

void 
Assertion::message (const std::string& msg)
{
  static std::ios_base::Init init;   // Can be called from static constructor.

  if (logs ().size () == 0)
    std::cout << msg << "\n";

  for (unsigned int i = 0; i < logs ().size (); i++)
    {
      logs ()[i]->message (msg);
      logs ()[i]->flush ();
    }
}

void 
Assertion::error (const std::string& msg)
{
  static std::ios_base::Init init;   // Can be called from static constructor.

  if (logs ().size () == 0)
    std::cerr << msg << "\n";
  
  for (unsigned int i = 0; i < logs ().size (); i++)
    {
      logs ()[i]->error (msg);
      logs ()[i]->flush ();
    }
}

void 
Assertion::warning (const std::string& msg)
{
  static std::ios_base::Init init;   // Can be called from static constructor.

  if (logs ().size () == 0)
    std::cerr << msg << "\n";

  for (unsigned int i = 0; i < logs ().size (); i++)
    {
      logs ()[i]->warning (msg);
      logs ()[i]->flush ();
    }
}

void 
Assertion::debug (const std::string& msg)
{
  static std::ios_base::Init init;   // Can be called from static constructor.

  if (logs ().size () == 0)
    std::cerr << msg << "\n";

  for (unsigned int i = 0; i < logs ().size (); i++)
    {
      logs ()[i]->debug (msg);
      logs ()[i]->flush ();
    }
}

void 
Assertion::failure (const char* file, int line, const char* fun,
		    const char* test)
{
  std::ostringstream tmp;
  tmp << file << ":" << line << ": assertion '" << test << "' failed";
  if (fun)
    tmp << " in " << fun;
  
  error (tmp.str ());

  if (logs ().size () == 0)
    // Windows dislike throw in static constructors.
    exit (4);
  else
    throw 5;
}

void 
Assertion::bug (const char* file, int line, const char* fun,
		const std::string& msg)
{
  if (full (file, line))
    return;
  std::ostringstream tmp;
  tmp << file << ":" << line << ": Daisy bug: '" << msg << "'";
  if (fun)
    tmp << " in " << fun;

  error (tmp.str ());
}

void 
Assertion::warning (const char* file, int line, const char* fun,
		    const std::string& msg)
{
  if (full (file, line))
    return;
  std::ostringstream tmp;
  tmp << file << ":" << line << ": ";
  if (fun)
    tmp << "(" << fun << ") ";
  tmp << "warning: " << msg;

  warning (tmp.str ());
}

void 
Assertion::panic (const char* file, int line, const char* fun,
		  const std::string& msg)
{
  bug (file, line, fun, msg);
  if (logs ().size () == 0)
    // Windows dislike throw in static constructors.
    exit (6);
  else
    throw 7;
}

void 
Assertion::notreached (const char* file, int line, const char* fun)
{ panic (file, line, fun, "This line should never have been reached"); }

void 
Assertion::non_negative (const char* file, int line, const char* fun,
			 const std::vector<double>& v)
{
  for (unsigned int i = 0; i < v.size (); i++)
    if (v[i] < 0 || !std::isfinite (v[i]))
      {
	std::ostringstream tmp;
	tmp << "v[" << i << "] >= 0";
	Assertion::failure (file, line, fun, tmp.str ().c_str ());
      }
}

void 
Assertion::approximate (const char* file, int line, const char* fun,
                        double a, double b)
{
  if (!::approximate (a, b))
    {
      std::ostringstream tmp;
      tmp << a << " != " << b;
      Assertion::warning (file, line, fun, tmp.str ().c_str ());
    }
    
}

void 
Assertion::balance (const char* file, int line, const char* fun,
                    double oldval, double newval, double growth) 
{
  if (!::approximate (newval, oldval + growth)
      && !::approximate (newval - oldval, growth))
    {
      std::ostringstream tmp;
      tmp << "Internal balance check:\n"
          << "old value = " << oldval << "\n"
          << "new value = " << newval << " (should be " 
          << oldval + growth << ")\n"
          << "growth = " << growth << " (should be " 
          << newval - oldval << ")";
      Assertion::failure (file, line, fun, tmp.str ().c_str ());
    }
}


Assertion::Register::Register (Treelog& log)
  : treelog (log)
{
  for (unsigned int i = 0; i < logs ().size (); i++)
    daisy_assert (&log != logs ()[i]);

  logs ().push_back (&log);
}

Assertion::Register::~Register ()
{
  if (counter)
    {
      for (auto i: *counter)
	{
	  const std::string entry = i.first;
	  const int count = i.second;
	  std::ostringstream tmp;
	  tmp << entry << ": " << count << " total messages from this line";
	  warning (tmp.str ());
	}
      delete counter;
      counter = NULL;
    }
  daisy_assert (find (logs ().begin (), 
                      logs ().end (),
                      &treelog)
                != logs ().end ());
  logs ().erase (find (logs ().begin (), logs ().end (), &treelog));
  daisy_safe_assert (find (logs ().begin (), logs ().end (), &treelog)
                     == logs ().end ());
}

// assertion.C ends here.
