// block_nested.h -- Support for blocks nested in other blocks.
// 
// Copyright 2005, 2009 Per Abrahamsen and KVL.
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


#ifndef BLOCK_NESTED_H
#define BLOCK_NESTED_H

#include "object_model/block.h"
#include "object_model/treelog.h"

class BlockNested : public Block
{
  const Block& context;
  Treelog::Open msg_nest;
public:
  const Metalib& metalib () const;
  Treelog& msg () const;
  void set_error () const;
  const Frame& find_frame (const symbol key) const;
  Attribute::type lookup (symbol) const;
  void entries (std::set<symbol>&) const;

  // Context
protected:  
  BlockNested (const Block&, symbol scope_tag);
public:
  ~BlockNested ();
};

#endif // BLOCK_NESTED_H
