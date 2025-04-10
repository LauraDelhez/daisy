// select_number.C --- Select a state variable.
// 
// Copyright 1996-2002 Per Abrahamsen and Søren Hansen
// Copyright 2000-2002 KVL.
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

#include "daisy/output/select_value.h"
#include "object_model/librarian.h"
#include "object_model/frame.h"

struct SelectNumber : public SelectValue
{
  // Output routines.
  void output_number (const double number)
  { add_result (number); }
  void output_integer (const int integer)
  { output_number (integer); }
  void output_array (const std::vector<double>& array)
  { 
    const size_t size = array.size ();
    for (size_t i = 0; i < size; i++)
      add_result (array[i]); 
  }

  // Create and Destroy.
  SelectNumber (const BlockModel& al)
    : SelectValue (al)
  { }
};

static struct SelectNumberSyntax : public DeclareModel
{
  Model* make (const BlockModel& al) const
  { return new SelectNumber (al); }

  SelectNumberSyntax ()
    : DeclareModel (Select::component, "number", "value", "\
Extract specified number.\n\
If used on an array, it will treat them as individual numbers as\n\
specified by the 'handle' parameter.")
  { }
  void load_frame (Frame&) const
  { }
} SelectNumber_syntax;

// select_number.C ends here.
