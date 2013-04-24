//    Copyright (C) 2012, 2013 ebftpd team
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef __SCRIPTING_PYTHON_CONVERT_HPP
#define __SCRIPTING_PYTHON_CONVERT_HPP

#include <boost/python.hpp>

namespace scripting { namespace python
{

template <typename T, typename TfromPy>
struct object_from_python
{
   object_from_python()
   {
   py::converter::registry::push_back(&TfromPy::convertible, &TfromPy::construct, py::type_id<T>());
   }
};

template <typename T, typename TtoPy, typename TfromPy>
struct register_python_conversion
{
   register_python_conversion()
   {
   py::to_python_converter<T, TtoPy>();
   object_from_python<T, TfromPy>();
   }
};

template <typename T>
struct python_optional : public boost::noncopyable
{
  struct optional_to_python
  {
  static PyObject * convert(const boost::optional<T>& value)
  {
    return (value ? py::to_python_value<T>()(*value) : py::detail::none());
  }
  };

  struct optional_from_python
  {
  static void * convertible(PyObject * source)
  {
    using namespace py::converter;
    
    if (source == Py_None) return source;

    const registration& converters(registered<T>::converters);

    if (implicit_rvalue_convertible_from_python(source, converters))
    {
    rvalue_from_python_stage1_data data = rvalue_from_python_stage1(source, converters);
    return rvalue_from_python_stage2(source, data, converters);
    }
    
    return nullptr;
  }

  static void construct(PyObject * source, py::converter::rvalue_from_python_stage1_data * data)
  {
    using namespace py::converter;

    void * const storage = ((rvalue_from_python_storage<T> *)data)->storage.bytes;

    if (data->convertible == source)	  // == None
    new (storage) boost::optional<T>(); // A Boost uninitialized value
    else
    new (storage) boost::optional<T>(*static_cast<T *>(data->convertible));
    data->convertible = storage;
  }
  };

  explicit python_optional()
  {
  register_python_conversion<boost::optional<T>, optional_to_python, optional_from_python>();
  }
};

template <typename C>
struct container_to_python_list
{
  static PyObject* convert(const C& cont)
  {
    boost::python::list l;
    for (auto& elem : cont) l.append(elem);
    return py::incref(l.ptr());
  }
  
  container_to_python_list()
  {
    py::to_python_converter<C, container_to_python_list<C>>();
  }
};

} /* python namespace */
} /* scripting namespace */

#endif
