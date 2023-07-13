// ****************************************************************************
//  complex.cc                                                    DB48X project
// ****************************************************************************
//
//   File Description:
//
//      Complex numbers
//
//      There are two representations for complex numbers:
//      - rectangular representation is one of X;Y, X+ⅈY, X-ⅈY, X+Yⅈ or X-Yⅈ
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
#include "parser.h"
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



PARSE_BODY(complex)
// ----------------------------------------------------------------------------
//   Parse the various forms of complex number
// ----------------------------------------------------------------------------
//   We accept the following formats:
//   a. (1;3)           Classic RPL
//   b. (1 3)           Classic RPL
//   c. 1ⅈ3             ⅈ as a separator
//   d. 1+ⅈ3            ⅈ as a prefix
//   e. 1-ⅈ3
//   f. 1+3ⅈ            ⅈ as a postfix
//   g. 1-3ⅈ
//   h. 1∡30            ∡ as a separator
//
//   Cases a-g generate a rectangular form, case i generates a polar form
//   Cases c-h can be surrounded by parentheses as well
//
//   In case (a), we do not accept (1,3) which classic RPL would accept,
//   because in DB48X 1,000.000 is a valid real number with thousands separator
{
    gcutf8      src    = p.source;
    size_t      max    = p.length;
    id          type   = ID_object;

    // Find the end of the possible complex number and check parentheses
    utf8        first  = src;
    utf8        last   = first;
    utf8        ybeg   = nullptr;
    size_t      xlen   = 0;
    size_t      ylen   = 0;
    bool        paren  = false;
    bool        signok = false;
    bool        ineq   = false;
    char        sign   = 0;
    while (size_t(last - first) < max)
    {
        unicode cp = utf8_codepoint(last);

        // Check if we have an opening parenthese
        if (last == first && cp == '(')
        {
            paren = true;
            first++;
        }

        // Check if found a '+' or '-' (cases d-g)
        else if (signok && (cp == '+' || cp == '-'))
        {
            if (sign)
            {
                // Cannot have two signs
                rt.syntax_error().source(last);
                return WARN;
            }
            sign = cp;
            ybeg = last + 1;
            if (type != ID_polar)
                xlen = last - first;
        }

        // Check if we found the ⅈ sign
        else if (cp == I_MARK)
        {
            // Can't have two complex signs
            if (type != ID_object)
            {
                rt.syntax_error().source(last);
                return WARN;
            }
            type = ID_rectangular;

            // Case of ⅈ as a separator (c)
            if (!sign)
            {
                ybeg = last + utf8_size(cp);
                xlen = last - first;
            }
            // Case of prefix ⅈ (d or e)
            else if (last == ybeg)
            {
                ybeg = last + utf8_size(cp);
            }
            // Case of postfix ⅈ (f or g)
            else
            {
                ylen = last - ybeg;
            }
        }

        // Check if we found the ∡ sign
        else if (cp == ANGLE_MARK)
        {
            // Can't have two complex signs, or have that with a sign
            if (type != ID_object || sign)
            {
                rt.syntax_error().source(last);
                return WARN;
            }
            type = ID_polar;

            // Case of ∡ as a separator (h)
            ybeg = last + utf8_size(cp);
            xlen = last - first;
        }

        // Check if we found a space or ';' inside parentheses
        else if (paren && (cp == ' ' || cp == ';'))
        {
            // Can't have two complex signs
            if (type != ID_object)
            {
                rt.syntax_error().source(last);
                return WARN;
            }
            type = ID_rectangular;
            ybeg = last + 1;
            xlen = last - first;
        }

        // Check if we found characters that we don't expect in a complex
        else if (cp == '"' || cp == '{' || cp == '[' || cp == L'«')
        {
            return SKIP;
        }

        // Check if we have equations in our complex
        else if (cp == '\'')
        {
            ineq = !ineq;
        }

        // Check if we have two parentheses
        else if (paren && !ineq && cp == '(')
        {
            rt.syntax_error().source(last);
            return WARN;
        }

        // Check if we found the end of the complex number
        else if (cp == ' ' || cp == '\n' || cp == '\t' ||
                 cp == ')' || cp == '}')
        {
            break;
        }

        // We can have a sign except after exponent markers
        signok = cp != 'e' && cp != 'E' && cp != L'⁳';

        // Loop on next characters
        last += utf8_size(cp);
    }

    // If we did not find the necessary structure, just skip
    if (type == ID_object || !xlen || !ybeg)
        return SKIP;

    // Check if we need to compute the length of y
    if (!ylen)
        ylen = last - ybeg;

    // Compute size that we parsed
    size_t parsed = last - first + paren;

    // Parse the first object
    gcutf8 ysrc = ybeg;
    size_t xsz = xlen;
    algebraic_g x = algebraic_p(object::parse(first, xlen));
    if (!x)
        return ERROR;
    if (xlen != xsz)
    {
        rt.syntax_error().source(utf8(src) + xlen);
        return ERROR;
    }

    // Parse the second object
    size_t ysz = ylen;
    algebraic_g y = algebraic_p(object::parse(ysrc, ylen));
    if (!y)
        return ERROR;
    if (ylen != ysz)
    {
        rt.syntax_error().source(utf8(ysrc) + ylen);
        return ERROR;
    }
    if (sign == '-')
    {
        y = neg::evaluate(y);
        if (!y)
            return ERROR;
    }

    // Build the resulting complex
    complex_g result = rt.make<complex>(type, x, y);
    p.out = complex_p(result);
    p.end = parsed;

    return OK;
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
    // This is really handled in the parser for 'rectangular'
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


COMMAND_BODY(ImaginaryUnit)
// ----------------------------------------------------------------------------
//   Push a unit complex number on the stack
// ----------------------------------------------------------------------------
{
    algebraic_g zero = algebraic_p(rt.make<integer>(0));
    algebraic_g one = algebraic_p(rt.make<integer>(1));
    if (!zero || !one)
        return ERROR;
    rectangular_g i = rt.make<rectangular>(zero, one);
    if (!i)
        return ERROR;
    if (!rt.push(rectangular_p(i)))

        return ERROR;
    return OK;
}
