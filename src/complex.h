#ifndef COMPLEX_H
#define COMPLEX_H
// ****************************************************************************
//  complex.h                                                   DB48X project
// ****************************************************************************
//
//   File Description:
//
//      Complex numbers
//
//      There are two representations for complex numbers:
//      - rectangular representation is one of X;Y, X+𝒊Y, X-𝒊Y, X+Y𝒊 or X-Y𝒊
//      - polar representation is X∡Y
//
//      Some settings control how complex numbers are rendered
//
//
//
// ****************************************************************************
//   (C) 2023 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the terms outlined in LICENSE.txt
// ****************************************************************************
//   This file is part of DB48X.
//
//   DB48X is free software: you can redistribute it and/or modify
//   it under the terms outlined in the LICENSE.txt file
//
//   DB48X is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// ****************************************************************************
//
// Payload format:
//
//   The payload is a simple sequence with the two parts of the complex


#include "algebraic.h"
#include "runtime.h"
#include "settings.h"


struct complex;
struct rectangular;
struct polar;

typedef const complex         *complex_p;
typedef const rectangular     *rectangular_p;
typedef const polar           *polar_p;

typedef gcp<const complex>     complex_g;
typedef gcp<const rectangular> rectangular_g;
typedef gcp<const polar>       polar_g;


struct complex : algebraic
// ----------------------------------------------------------------------------
//    Base class shared by both rectangular and polar implementations
// ----------------------------------------------------------------------------
{
    complex(algebraic_g x, algebraic_g y, id type): algebraic(type)
    {
        byte *p = (byte *) payload(this);
        size_t xs = x->size();
        size_t ys = y->size();
        memcpy(p, byte_p(x), xs);
        p += xs;
        memcpy(p, byte_p(y), ys);
    }

    static size_t required_memory(id i, algebraic_g x, algebraic_g y)
    {
        return leb128size(i) + x->size() + y->size();
    }

    algebraic_g x() const
    {
        algebraic_p p = algebraic_p(payload(this));
        return p;
    }
    algebraic_g y() const
    {
        algebraic_p p = algebraic_p(payload(this));
        algebraic_p n = algebraic_p(byte_p(p) + p->size());
        return n;
    }

    algebraic_g re()  const;
    algebraic_g im()  const;
    algebraic_g mod() const;
    algebraic_g arg() const;
    algebraic_g conjugate() const;

    enum { I_MARK = L'ⅈ', ANGLE_MARK = L'∡' };

public:
    SIZE_DECL(complex);
    PARSE_DECL(complex);
};


complex_g operator-(complex_g x);
complex_g operator+(complex_g x, complex_g y);
complex_g operator-(complex_g x, complex_g y);
complex_g operator*(complex_g x, complex_g y);
complex_g operator/(complex_g x, complex_g y);


struct rectangular : complex
// ----------------------------------------------------------------------------
//   Rectangular representation for complex numbers
// ----------------------------------------------------------------------------
{
    rectangular(algebraic_g re, algebraic_g im, id type = ID_rectangular)
        : complex(re, im, type) {}

    algebraic_g re()  const     { return x(); }
    algebraic_g im()  const     { return y(); }
    algebraic_g mod() const;
    algebraic_g arg() const;
    bool        is_zero() const;

public:
    OBJECT_DECL(rectangular);
    // PARSE_DECL(rectangular); is really in complex
    RENDER_DECL(rectangular);
};


struct polar : complex
// ----------------------------------------------------------------------------
//   Polar representation for complex numbers
// ----------------------------------------------------------------------------
{
    polar(algebraic_g re, algebraic_g im, id type = ID_polar)
        : complex(re, im, type) {}

    algebraic_g re()  const;
    algebraic_g im()  const;
    algebraic_g mod() const     { return x(); }
    algebraic_g arg() const     { return y(); }
    bool        is_zero() const;

public:
    OBJECT_DECL(polar);
    PARSE_DECL(polar);          // Just skips, actual work in 'rectangular'
    RENDER_DECL(polar);
};

COMMAND_DECLARE(ImaginaryUnit);


#endif // COMPLEX_H
