// treelog.h -- Log hierarchical information.
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


#ifndef TREELOG_H
#define TREELOG_H

#include "object_model/symbol.h"
#include <string>
#include <boost/noncopyable.hpp>

#ifdef __unix
#define EXPORT /* Nothing */
#elif defined (BUILD_DLL)
/* DLL export */
#define EXPORT __declspec(dllexport)
#else
/* EXE import */
#define EXPORT __declspec(dllimport)
#endif

class EXPORT Treelog : private boost::noncopyable
{
  // Nesting.
public:
  class Open
  {
  private:
    Treelog& log;
  public:
    Open (Treelog& l, const std::string& name);
    Open (Treelog& l, const symbol name);
    Open (Treelog& l, const char* name);
    Open (Treelog& l, symbol parameter, size_t index, symbol model);
    ~Open ();
  };
  virtual void open (const std::string&) = 0;
  virtual void close () = 0;
  
  // Use.
public:
  virtual void debug (const std::string&) = 0;
  virtual void entry (const std::string&) = 0;
  virtual void message (const std::string&);
  virtual void warning (const std::string&);
  virtual void error (const std::string&);
  virtual void bug (const std::string&);
  virtual void touch () = 0;
  virtual void flush () = 0;

  // Create and Destroy.
public:
  static Treelog& null ();
  Treelog ();
  virtual ~Treelog ();
};

#define TREELOG_MODEL(msg) \
  Treelog::Open nest (msg, this->library_id () + ": " + this->objid \
                      + " " + __FUNCTION__)

#define TREELOG_SUBMODEL(msg, submodel) \
  Treelog::Open nest (msg, submodel + std::string (": ") + __FUNCTION__)

#endif // TREELOG_H
