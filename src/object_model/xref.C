// xref.C -- Find cross references in datastructures.
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

#include "object_model/xref.h"
#include "object_model/traverse.h"
#include "object_model/metalib.h"
#include "object_model/library.h"
#include "object_model/librarian.h"
#include "util/assertion.h"
#include "object_model/frame_model.h"
#include <deque>

class TraverseXRef : public Traverse
{
  // We store it here.
  XRef& xref;

  // State.
  symbol current_component;
  symbol current_model;
  enum { is_model, is_parameterization, is_submodel, is_invalid = -1 } type;
  symbol current_submodel;
  std::vector<symbol> path;

  // Use.
private:
  void use_submodel (const symbol submodel);
  void use_component (const Library& library);
  void use_model (const Library& library, symbol model);

  // Create & Destroy.
public:
  TraverseXRef (const Metalib&, XRef&);
  ~TraverseXRef ();

private:
  // Implementation.
  bool enter_library (Library& library, symbol component);
  void leave_library ();
  bool enter_model (Frame&, 
		    symbol component, symbol model);
  void leave_model (symbol component, symbol name);
  bool enter_submodel (Frame& frame, const Frame& default_frame,
  		       const symbol name, const symbol registered);
  void leave_submodel ();
  bool enter_submodel_default (const Frame&,
			       const symbol name,
                               const symbol registered);
  void leave_submodel_default ();
  bool enter_submodel_sequence (const Frame& frame, 
                                const Frame& default_frame,
                                const symbol name, unsigned index, 
                                const symbol registered);
  void leave_submodel_sequence ();
  bool enter_submodel_sequence_default (const Frame& default_frame,
  					const symbol name, 
                                        const symbol registered);
  void leave_submodel_sequence_default ();
  bool enter_object (const Library&, 
		     const Frame& frame, const Frame& default_frame,
  		     const symbol name);
  void leave_object ();
  bool enter_object_sequence (const Library&, const Frame& frame,
  			      const Frame& default_frame,
  			      const symbol name, 
  			      unsigned index);
  void leave_object_sequence ();
  bool enter_parameter (const Frame& frame, const Frame& default_frame,
			const symbol name, const symbol parameter);
  void leave_parameter ();
};

void 
TraverseXRef::use_submodel (const symbol submodel)
{
  daisy_assert (Librarian::submodel_registered (submodel));
  XRef::Users& moi = xref.submodels[submodel];
  
  switch (type)
    {
    case is_parameterization:
      break;
    case is_submodel:
      moi.submodels.insert (XRef::SubmodelUser (current_submodel, path));
      break;
    case is_model:
      moi.models.insert (XRef::ModelUser (current_component, current_model, 
                                          path)); 
      break;
    case is_invalid:
    default:
      daisy_notreached ();
    }
  
}

void 
TraverseXRef::use_component (const Library& library)
{
  const symbol component = library.name ();
  XRef::Users& moi = xref.components[component];
  
  switch (type)
    {
    case is_parameterization:
      break;
    case is_submodel:
      moi.submodels.insert (XRef::SubmodelUser (current_submodel, path));
      break;
    case is_model:
      moi.models.insert (XRef::ModelUser (current_component, current_model, 
                                          path)); 
      break;
    case is_invalid:
    default:
      daisy_notreached ();
    }
  
}

void 
TraverseXRef::use_model (const Library& library, const symbol model)
{
  daisy_assert (library.check (model));
  const symbol component = library.name ();
  XRef::Users& moi = xref.models[XRef::ModelUsed (component, model)];
  
  switch (type)
    {
    case is_submodel:
      moi.submodels.insert (XRef::SubmodelUser (current_submodel, path));
      break;
    case is_parameterization:
    case is_model:
      moi.models.insert (XRef::ModelUser (current_component, current_model, 
                                          path)); 
      break;
    case is_invalid:
    default:
      daisy_notreached ();
    }
  
}

bool
TraverseXRef::enter_library (Library&, symbol component)
{
  daisy_assert (type == is_invalid);
  current_component = component;
  return true;
}

void
TraverseXRef::leave_library ()
{ 
  daisy_safe_assert (type == is_invalid);
  daisy_safe_assert (path.empty ());
}

bool
TraverseXRef::enter_model (Frame& frame,
			   const symbol component, const symbol name)
{
  daisy_assert (component == current_component);
  daisy_assert (type == is_invalid);
  current_model = name;
  if (frame.base_name () != Attribute::None ())
    type = is_parameterization;
  else
    type = is_model;
  return true;
}

void
TraverseXRef::leave_model (const symbol component, const symbol name)
{ 
  daisy_safe_assert (path.empty ());
  daisy_safe_assert (component == current_component);
  daisy_safe_assert (name == current_model);
  type = is_invalid;
}

bool
TraverseXRef::enter_submodel (Frame& frame, const Frame&,
			      const symbol name, const symbol registered)
{ return enter_submodel_default (frame, name, registered); }

void
TraverseXRef::leave_submodel ()
{ leave_submodel_default (); }

bool
TraverseXRef::enter_submodel_default (const Frame& default_frame,
				      const symbol name, 
                                      const symbol registered)
{ 
  if (type == is_invalid)
    {
      // We are traversing a top level submodels.
      daisy_assert (path.empty ());
      type = is_submodel;
      current_submodel = name;
      return true;
    }
  if (registered != Attribute::None ())
    {
      // Nested submodel.  Register and stop.
      use_submodel (registered);
      return false;
    }
  return true; 
}

void
TraverseXRef::leave_submodel_default ()
{ 
  if (path.empty ())
    {
      // Top level submodel.
      daisy_safe_assert (type == is_submodel);
      type = is_invalid;
    }
}

bool
TraverseXRef::enter_submodel_sequence (const Frame& frame,
				       const Frame&,
				       const symbol name, unsigned,
                                       const symbol registered)
{ return enter_submodel_default (frame, name, registered); }

void
TraverseXRef::leave_submodel_sequence ()
{ leave_submodel_default (); }

bool
TraverseXRef::enter_submodel_sequence_default (const Frame& frame,
					       const symbol name,
                                               const symbol registered)
{ return enter_submodel_default (frame, name, registered); }

void
TraverseXRef::leave_submodel_sequence_default ()
{ leave_submodel_default (); }

bool
TraverseXRef::enter_object (const Library& library, 
                            const Frame& frame, const Frame&,
			    const symbol)
{
  use_model (library, frame.type_name ());
  return false; 
}

void
TraverseXRef::leave_object ()
{ daisy_notreached (); }

bool
TraverseXRef::enter_object_sequence (const Library& library, 
                                     const Frame& frame,
                                     const Frame& default_frame,
				     const symbol name, unsigned)
{ return enter_object (library, frame, default_frame, name); }

void
TraverseXRef::leave_object_sequence ()
{ leave_object (); }

bool
TraverseXRef::enter_parameter (const Frame& frame, const Frame& default_frame,
			       const symbol, const symbol name)
{ 
  if (type == is_parameterization)
    {
      // Ignore inherited values.
      if (frame.subset (metalib, default_frame, name))
        return false;
    }
  else if (type == is_model && frame.base_name () != Attribute::None ())
    {
      // Ignore base parameters.
      const Library& library = metalib.library (current_component);
      const symbol base_model = frame.base_name ();
      const Frame& base_frame = library.model (base_model);
      if (base_frame.lookup (name) != Attribute::Error)
        {
          if (frame.subset (metalib, base_frame, name))
            return false;
        }
    }
  path.push_back (name);

  if (frame.lookup (name) == Attribute::Model)
    // We always use the component, even if it has no value, or a
    // value that is an empty sequence.
    use_component (metalib.library (frame.component (name)));

  return true; 
}

void 
TraverseXRef::leave_parameter ()
{ path.pop_back (); }

TraverseXRef::TraverseXRef (const Metalib& mlib, XRef& xr)
  : Traverse (mlib),
    xref (xr),
    current_component ("Daisy"),
    current_model ("invalid"),
    type (is_invalid)
{
  traverse_all_libraries ();
  traverse_all_submodels ();
}

TraverseXRef::~TraverseXRef ()
{ 
  daisy_safe_assert (type == is_invalid); 
  daisy_safe_assert (path.empty ());
}

bool
XRef::ModelUsed::operator< (const ModelUsed& other) const
{ return component < other.component
    || (component == other.component && model < other.model); }


XRef::ModelUsed::ModelUsed (const symbol comp, const symbol mod)
  : component (comp),
    model (mod)
{ }

bool
XRef::ModelUser::operator< (const ModelUser& other) const
{ return component < other.component
    || (component == other.component && model < other.model); }


XRef::ModelUser::ModelUser (const symbol comp, const symbol mod,
			    const std::vector<symbol>& p)
  : component (comp),
    model (mod),
    path (p)
{ }

bool
XRef::SubmodelUser::operator< (const SubmodelUser& other) const
{ return submodel < other.submodel; }

XRef::SubmodelUser::SubmodelUser (const symbol sub,
                                  const std::vector<symbol>& p)
  : submodel (sub),
    path (p)
{ }

XRef::SubmodelUser::SubmodelUser ()
{ }

XRef::Users::Users ()
{ }

XRef::XRef (const Metalib& mlib)
{ 
  TraverseXRef traverse (mlib, *this);
}

XRef::~XRef ()
{ }

