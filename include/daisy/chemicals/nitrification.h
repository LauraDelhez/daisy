// nitrification.h
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


#ifndef NITRIFICATION_H
#define NITRIFICATION_H

#include "object_model/model.h"
#include "object_model/symbol.h"
#include <memory>

class Soil;
class SoilWater;
class SoilHeat;
class SoilNO3;
class SoilNH4;
class BlockModel;

class Nitrification : public Model
{
  // Content.
public:
  static const char *const component;
  symbol library_id () const;

  // Parameters.
protected:
  const double N2O_fraction;

  // Simulation.
public:
  virtual void tick (const double M, const double C,  const double Theta,
                     const double h, const double T,
                     double& NH4, double& N2O, double& NO3) const = 0;

  // Utilities.
  static double f_h (double h);
  static double f_T (double T);

  // Create and Destroy.
public:
  Nitrification (const BlockModel& al);
  Nitrification (const Frame& al);
  static std::unique_ptr<Nitrification> create_default ();
public:
  ~Nitrification ();
};

#endif // NITRIFICATION_H
