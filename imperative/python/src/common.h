#pragma once

#include "./helper.h"

void init_common(pybind11::module m);

void set_default_device(const std::string& device);
std::string get_default_device();

extern pybind11::handle py_comp_node_type;
