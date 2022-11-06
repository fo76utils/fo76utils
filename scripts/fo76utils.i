%module fo76utils

%include "carrays.i"
%array_functions(std::uint32_t, uint32Array);

%include "std_string.i"

%feature ("flatnested");

%{
#include "common.hpp"
#include "filebuf.hpp"
#include "downsamp.hpp"
#include "ba2file.hpp"
#include "esmfile.hpp"
#include "render.hpp"
enum
{
  usingRGB10A2 = USE_PIXELFMT_RGB10A2
};
%}

%include "filebuf.hpp"
%include "downsamp.hpp"
%include "ba2file.hpp"
%include "esmfile.hpp"
%include "render.hpp"

enum
{
  usingRGB10A2 = USE_PIXELFMT_RGB10A2
};

