// parser.C
// 
// Copyright 1996-2001 Per Abrahamsen and Søren Hansen
// Copyright 2000-2001 KVL.
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

#include "object_model/parser.h"
#include "object_model/block_model.h"
#include "object_model/librarian.h"

const char *const Parser::component = "parser";

symbol
Parser::library_id () const
{
  static const symbol id (component);
  return id;
}

Parser::Parser (const symbol id)
  : name (id)
{ }

Parser::Parser (const BlockModel& al)
  : name (al.type_name ())
{ }

Parser::~Parser ()
{ }

static struct ParserInit : public DeclareComponent 
{
  ParserInit ()
    : DeclareComponent (Parser::component, "\
To start the simulation, many parameters must be specified and state\n\
variables must be given an initial value.  It is the responsibility of\n\
the 'parser' component to read these data from an external source\n\
(typically a setup file), and convert them into the internal format.")
  { }
} Parser_init;

// parser.C ends here.
