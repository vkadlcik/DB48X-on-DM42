// ****************************************************************************
//  complex.cc                                                    DB48X project
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

#include "complex.h"

#include "arithmetic.h"
#include "functions.h"
#include "renderer.h"
#include "runtime.h"


// ============================================================================
//
//   Generic complex operations
//
// ============================================================================
//
//  The generic operations optimize for the most efficient operation
//  if there is a difference between rectangular and polar
//

SIZE_BODY(complex)
// ----------------------------------------------------------------------------
//   Size of a complex number
// ----------------------------------------------------------------------------
{
    object_p p = object_p(payload(o));
    object_p e = p->skip()->skip();
    return byte_p(e) - byte_p(o);
}


algebraic_g complex::re() const
// ----------------------------------------------------------------------------
//   Return real part in a format-independent way
// ----------------------------------------------------------------------------
{
    if (type() == ID_rectangular)
        return rectangular_p(this)->re();
    return polar_p(this)->re();
}


algebraic_g complex::im() const
// ----------------------------------------------------------------------------
//   Return imaginary part in a format-independent way
// ----------------------------------------------------------------------------
{
    if (type() == ID_rectangular)
        return rectangular_p(this)->im();
    return polar_p(this)->im();
}


algebraic_g complex::mod() const
// ----------------------------------------------------------------------------
//   Return modulus in a format-independant way
// ----------------------------------------------------------------------------
{
    if (type() == ID_polar)
        return polar_p(this)->mod();
    return rectangular_p(this)->mod();
}


algebraic_g complex::arg() const
// ----------------------------------------------------------------------------
//   Return argument in a format-independant way
// ----------------------------------------------------------------------------
{
    if (type() == ID_polar)
        return polar_p(this)->arg();
    return rectangular_p(this)->arg();
}


algebraic_g complex::conjugate() const
// ----------------------------------------------------------------------------
//   Return complex conjugate in a format-independent way
// ----------------------------------------------------------------------------
{
    return rt.make<complex>(type(), x(), -y());
}


complex_g operator-(complex_g x)
// ----------------------------------------------------------------------------
//  Unary minus
// ----------------------------------------------------------------------------
{
    if (x->type() == object::ID_polar)
    {
        polar_p p = polar_p(complex_p(x));
        return rt.make<polar>(-p->mod(), p->arg());
    }
    rectangular_p r = rectangular_p(complex_p(x));
    return rt.make<rectangular>(-r->re(), -r->im());
}


complex_g operator+(complex_g x, complex_g y)
// ----------------------------------------------------------------------------
//   Complex addition - Don't even bother doing it in polar form
// ----------------------------------------------------------------------------
{
    return rt.make<rectangular>(x->re() + y->re(), x->im() + y->im());
}


complex_g operator-(complex_g x, complex_g y)
// ----------------------------------------------------------------------------
//   Complex subtraction - Always in rectangular form
// ----------------------------------------------------------------------------
{
    return rt.make<rectangular>(x->re() - y->re(), x->im() - y->im());
}


complex_g operator*(complex_g x, complex_g y)
// ----------------------------------------------------------------------------
//   If both are in rectangular form, rectangular, otherwise polar
// ----------------------------------------------------------------------------
{
    object::id xt = x->type();
    object::id yt = y->type();
    if (xt != object::ID_rectangular || yt != object::ID_rectangular)
        return rt.make<polar>(x->mod() * y->mod(), x->arg() + y->arg());

    rectangular_p xx = rectangular_p(complex_p(x));
    rectangular_p yy = rectangular_p(complex_p(y));
    algebraic_g xr = xx->re();
    algebraic_g xi = xx->im();
    algebraic_g yr = yy->re();
    algebraic_g yi = yy->im();
    return rt.make<rectangular>(xr*yr-xi*yi, xr*yi+xi*yr);
}


complex_g operator/(complex_g x, complex_g y)
// ----------------------------------------------------------------------------
//   Like for multiplication, it's slighly cheaper in polar form
// ----------------------------------------------------------------------------
{
    object::id xt = x->type();
    object::id yt = y->type();
    if (xt != object::ID_rectangular || yt != object::ID_rectangular)
        return rt.make<polar>(x->mod() / y->mod(), x->arg() - y->arg());

    rectangular_p xx = rectangular_p(complex_p(x));
    rectangular_p yy = rectangular_p(complex_p(y));
    algebraic_g xr = xx->re();
    algebraic_g xi = xx->im();
    algebraic_g yr = yy->re();
    algebraic_g yi = yy->im();
    algebraic_g d = sq::evaluate(yr);
    return rt.make<rectangular>((xr*yr+xi*yi)/d, (xi*yr-xr*yi)/d);
}



// ============================================================================
//
//   Specific code for rectangular form
//
// ============================================================================

algebraic_g rectangular::mod() const
// ----------------------------------------------------------------------------
//   Compute the modulus in rectangular form
// ----------------------------------------------------------------------------
{
    algebraic_g r = re();
    algebraic_g i = im();
    return hypot::evaluate(r, i);
}

algebraic_g rectangular::arg() const
// ----------------------------------------------------------------------------
//   Compute the argument in rectangular form
// ----------------------------------------------------------------------------
{
    algebraic_g r = re();
    algebraic_g i = im();
    return atan2::evaluate(r, i);
}


PARSE_BODY(rectangular)
// ----------------------------------------------------------------------------
//   Parse a complex number in rectangular form
// ----------------------------------------------------------------------------
{
    return SKIP;
}


RENDER_BODY(rectangular)
// ----------------------------------------------------------------------------
//   Render a complex number in rectangular form
// ----------------------------------------------------------------------------
{
    algebraic_g re = o->re();
    algebraic_g im = o->im();
    re->render(r);
    r.need_sign();
    im->render(r);
    r.put(unicode(I_MARK));
    return r.size();
}



// ============================================================================
//
//   Polar-specific code
//
// ============================================================================

algebraic_g polar::re() const
// ----------------------------------------------------------------------------
//   Compute the real part in polar form
// ----------------------------------------------------------------------------
{
    algebraic_g m = mod();
    algebraic_g a = arg();
    return m * cos::evaluate(a);
}

algebraic_g polar::im() const
// ----------------------------------------------------------------------------
//   Compute the imaginary part in polar form
// ----------------------------------------------------------------------------
{
    algebraic_g m = mod();
    algebraic_g a = arg();
    return m * sin::evaluate(a);
}


PARSE_BODY(polar)
// ----------------------------------------------------------------------------
//   Parse a complex number in polar form
// ----------------------------------------------------------------------------
{
    return SKIP;
}


RENDER_BODY(polar)
// ----------------------------------------------------------------------------
//   Render a complex number in rectangular form
// ----------------------------------------------------------------------------
{
    algebraic_g m = o->mod();
    algebraic_g a = o->arg();
    m->render(r);
    r.put(unicode(ANGLE_MARK));
    a->render(r);
    return r.size();
}
