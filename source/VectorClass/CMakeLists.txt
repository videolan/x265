if (MSVC_VERSION GREATER 1500) # VC10 or higher
    add_definitions(/arch:SSE2)
endif()

add_library(VectorClass
    instrset.h
    instrset_detect.cpp
    vectorclass.h
    vectorf128.h
    vectorf256.h
    vectorf256e.h
    vectori128.h
    vectori256.h
    vectori256e.h
    special/complexvec.h
    special/decimal.h
    special/quaternion.h
    special/vector3d.h
    special/vectormath.h)

# dispatch_example.cpp
