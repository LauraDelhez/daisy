// scope_sources.C --- A scope based on a time series.
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


#include "util/scope_sources.h"
#include "gnuplot/source.h"
#include "object_model/treelog.h"
#include "util/assertion.h"
#include <sstream>

void 
ScopeSources::entries (std::set<symbol>& all) const
{ 
  for (size_t i = 0; i < source.size (); i++)
    all.insert (source[i]->title ());
}

Attribute::type 
ScopeSources::lookup (const symbol tag) const
{
  for (size_t i = 0; i < source.size (); i++)
    if (source[i]->title () == tag)
      return Attribute::Number;

  return Attribute::Error;
}

bool 
ScopeSources::check (const symbol tag) const
{
  for (size_t i = 0; i < source.size (); i++)
    if (source[i]->title () == tag 
        && (index[i] < 0
            || (index[i] < source[i]->value ().size ()
                && source[i]->time ().at (index[i]) == now)))
      return true;
    
  return false;
}

double 
ScopeSources::number (const symbol tag) const
{
  for (size_t i = 0; i < source.size (); i++)
    if (source[i]->title () == tag)
      {
        daisy_assert (index[i] < source[i]->value ().size ());
        daisy_assert (source[i]->time ().at (index[i]) == now);
        return source[i]->value ().at (index[i]);
      }
  daisy_notreached ();
}

symbol 
ScopeSources::dimension (const symbol tag) const
{
  for (size_t i = 0; i < source.size (); i++)
    if (source[i]->title () == tag)
      return symbol (source[i]->dimension ());
  daisy_notreached ();
}

symbol
ScopeSources::description (const symbol) const
{ return symbol ("Descriptions not implemented yet"); }

bool 
ScopeSources::load (Treelog& msg)
{
  daisy_assert (index.size () == source.size ());

  // Propagate.
  bool ok = true;
  for (size_t i = 0; i < source.size (); i++)
    {
      std::ostringstream tmp;
      tmp << "[" << i << "] " << source[i]->title ();
      Treelog::Open nest (msg, tmp.str ());
      if (!source[i]->load (msg))
        ok = false;
    }
  return ok;
}

symbol
ScopeSources::with ()
{
  if (source.size () > 0)
    return source[0]->with ();
    
  // No sources => no dates => no data.
  return "lines";
}

void 
ScopeSources::first ()
{ 
  fill (index.begin (), index.end (), 0); 
  now = Time (1, 1, 1, 0);
  next ();
}

bool 
ScopeSources::done ()
{
  daisy_assert (index.size () == source.size ());
  for (size_t i = 0; i < index.size (); i++)
    if (index[i] < source[i]->value ().size ())
      return false;

  return true;
}
      
void 
ScopeSources::next ()
{
  daisy_assert (index.size () == source.size ());
  for (size_t i = 0; i < index.size (); i++)
    if (index[i] < source[i]->time ().size ()
        && source[i]->time ().at (index[i]) <= now)
      index[i]++;
    
  now = Time (9999, 12, 31, 23);
    
  for (size_t i = 0; i < index.size (); i++)
    if (index[i] < source[i]->time ().size ()
        && source[i]->time ().at (index[i]) <= now)
      now = source[i]->time ().at (index[i]);
}

ScopeSources::ScopeSources (const std::vector<Source*>& s)
  : source (s),
    index (s.size (), -1),
    now (1, 1, 1, 0)
{ }

ScopeSources::~ScopeSources ()
{ }

// scope_sources.C ends here
