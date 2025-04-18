// integer.C --- Integers in Daisy.
// 
// Copyright 2006 Per Abrahamsen and KVL.
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

#include "object_model/parameter_types/integer.h"
#include "object_model/parameter_types/boolean.h"
#include "object_model/submodeler.h"
#include "object_model/block_model.h"
#include "util/memutils.h"
#include "object_model/librarian.h"
#include "object_model/treelog.h"
#include "object_model/frame.h"
#include <sstream>

const char *const Integer::component = "integer";

symbol
Integer::library_id () const
{
  static const symbol id (component);
  return id;
}

const std::string& 
Integer::title () const
{ return name.name (); }

Integer::Integer (const BlockModel& al)
  : name (al.type_name ())
{ }

Integer::~Integer ()
{ }

struct IntegerConst : public Integer
{
  // Parameters.
  const int val;

  // Simulation.
  bool missing (const Scope&) const
  { return false; }
  int value (const Scope&) const
  { return val; }

  // Create.
  bool initialize (const Units&, const Scope&, Treelog&)
  { return true; }
  bool check (const Scope&, Treelog&) const
  { return true; }
  IntegerConst (const BlockModel& al)
    : Integer (al),
      val (al.integer ("value"))
  { }
};

static struct IntegerConstSyntax : public DeclareModel
{
  Model* make (const BlockModel& al) const
  { return new IntegerConst (al); }
  IntegerConstSyntax ()
    : DeclareModel (Integer::component, "const", 
	       "Always give the specified value.")
  { }
  void load_frame (Frame& frame) const
  {

    frame.declare_integer ("value", Attribute::Const,
		"Fixed value for this integer.");
    frame.order ("value");
  }
} IntegerConst_syntax;

struct IntegerCond : public Integer
{
  // Parameters.
  struct Clause
  {
    const std::unique_ptr<Boolean> condition;
    const int value;
    static void load_syntax (Frame& frame)
    {
      frame.declare_object ("condition", Boolean::component, "\
Condition to test for.");
      frame.declare_integer ("value", Attribute::Const, "\
Value to return.");
      frame.order ("condition", "value");
    }
    Clause (const Block& al)
      : condition (Librarian::build_item<Boolean> (al, "condition")),
        value (al.integer ("value"))
    { }
  };
  std::vector<const Clause*> clauses;

  // Simulation.
  bool missing (const Scope&) const
  { return false; }
  int value (const Scope& scope) const
  { 
    for (size_t i = 0; i < clauses.size (); i++)
      if (clauses[i]->condition->value (scope))
        return clauses[i]->value;
    throw "No matching conditions";
  }

  // Create.
  bool initialize (const Units& units, const Scope& scope, Treelog& msg)
  {
    bool ok = true;
    for (size_t i = 0; i < clauses.size (); i++)
      {
        std::ostringstream tmp;
        tmp << name << "[" << i << "]";
        Treelog::Open nest (msg, tmp.str ());
        if (!clauses[i]->condition->initialize (units, scope, msg))
          ok = false;
      }
    return ok;
  }
  bool check (const Scope& scope, Treelog& msg) const
  { 
    for (size_t i = 0; i < clauses.size (); i++)
      if (clauses[i]->condition->value (scope))
        return true;
    msg.error ("No clause matches");
    return false; 
  }
  IntegerCond (const BlockModel& al)
    : Integer (al),
      clauses (map_submodel_const<Clause> (al, "clauses"))
  { }
  ~IntegerCond ()
  { sequence_delete (clauses.begin (), clauses.end ()); }
};

static DeclareSubmodel 
integer_cond_clause_submodel (IntegerCond::Clause::load_syntax,
                              "IntegerCondClause", "\
If condition is true, return value.");

static struct IntegerCondSyntax : public DeclareModel
{
  Model* make (const BlockModel& al) const
  { return new IntegerCond (al); }
  IntegerCondSyntax ()
    : DeclareModel (Integer::component, "cond", "\
Return the value of the first clause whose condition is true.")
  { }
  void load_frame (Frame& frame) const
  {

    frame.declare_submodule_sequence ("clauses", Attribute::Const, "\
List of clauses to match for.",
                                   IntegerCond::Clause::load_syntax);
    frame.order ("clauses");
  }
} IntegerCond_syntax;

static struct IntegerInit : public DeclareComponent 
{
  IntegerInit ()
    : DeclareComponent (Integer::component, "\
Generic representation of integers.")
  { }
} Integer_init;

// integer.C ends here
