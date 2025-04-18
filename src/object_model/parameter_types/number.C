// number.C --- Numbers in Daisy.
// 
// Copyright 2004 Per Abrahamsen and KVL.
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

#include "object_model/parameter_types/number.h"
#include "object_model/block_model.h"
#include "object_model/librarian.h"
#include "object_model/units.h"
#include "util/assertion.h"
#include "object_model/treelog.h"
#include <sstream>

const char *const Number::component = "number";

symbol
Number::library_id () const
{
  static const symbol id (component);
  return id;
}

symbol
Number::title () const
{ return objid; }

bool 
Number::known (symbol dim)
{ return dim != Attribute::Unknown (); }

bool 
Number::tick_value (const Units& units,
                    double& value, symbol want, const Scope& scope, 
		    Treelog& msg)
{ 
  this->tick (units, scope, msg);
  if (this->missing (scope))
    {
      msg.warning ("Expression '" + objid + "' '" 
                   + title () + "' is not found in scope");
      return false;
    }

  value = this->value (scope);
  const symbol has = this->dimension (scope);
      
  if (!units.can_convert (has, want, value))
    {
      std::ostringstream tmp;
      tmp << "Cannot convert " << value << " [" << has
	  << "] to [" << want << "]";
      msg.warning (tmp.str ());
    }
  else
    value = units.convert (has, want, value);
  
  return true;
}

const Unit& 
Number::unit () const
{ daisy_notreached (); }

bool 
Number::check_dim (const Units& units, 
                   const Scope& scope, const symbol want, Treelog& msg) const
{
  if (!this->check (units, scope, msg))
    return false;

  const symbol has = this->dimension (scope);
  if (units.can_convert (has, want))
    return true;

  msg.error ("Cannot convert from [" + has + "] to [" + want + "]");
  return false;
}

Number::Number (const BlockModel& al)
  : objid (al.type_name ())
{ }

Number::~Number ()
{ }

static struct NumberInit : public DeclareComponent 
{
  NumberInit ()
    : DeclareComponent (Number::component, "\
Generic representation of numbers.")
  { }
} Number_init;

// number.C ends here.
