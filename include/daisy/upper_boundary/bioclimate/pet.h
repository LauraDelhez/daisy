// pet.h -- Potential evopotranspiration
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


#ifndef PET_H
#define PET_H

#include "object_model/model_framed.h"

class Log;
class Geometry;
class Soil;
class SoilHeat;
class SoilWater;
class Weather;
class Vegetation;
class Surface;
class Time;
class Treelog;
class BlockModel;

class Pet : public ModelFramed
{
  // Content.
public:
  static const char *const component;
  symbol library_id () const;

  // Utilities.
public:
  static double reference_to_potential_dry (const Vegetation&, const Surface&, 
                                            double ref);
  static double reference_to_potential_wet (const Vegetation&, const Surface&, 
                                            double ref);

  // Simulation.
public:
  virtual void tick (const Weather&, 
		     const double Rn, const double Rn_ref, const double G,
		     const Vegetation&, const Surface&, const Geometry&,
                     const Soil&, const SoilHeat&, 
		     const SoilWater&, Treelog&) = 0;
  virtual double wet () const = 0; // [mm/h]
  virtual double dry () const = 0; // [mm/h]
  virtual void output (Log&) const;

  // Create and Destroy.
public:
  virtual bool check (const Weather&, Treelog&) const;
  virtual void initialize (const Weather&);

protected:
  Pet (const BlockModel&);
public:
  ~Pet ();
};

#endif // PET_H
