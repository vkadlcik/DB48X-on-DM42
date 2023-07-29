#ifndef EQUATION_H
#define EQUATION_H
// ****************************************************************************
//  equation.h                                                    DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Implementation of algebraic equations
//
//     Equations are simply programs that are rendered and parsed specially
//
//
//
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


#include "program.h"
#include "symbol.h"

#include <type_traits>

GCP(equation);

struct equation : program
// ----------------------------------------------------------------------------
//   An equation is a program with ' and ' as delimiters
// ----------------------------------------------------------------------------
//   We also need special parsing and rendering of algebraic objects
{
    equation(gcbytes bytes, size_t len, id type = ID_equation)
        : program(bytes, len, type) {}
    static size_t required_memory(id i, gcbytes UNUSED bytes, size_t len)
    {
        return program::required_memory(i, bytes, len);
    }

    // Building an equation from an object
    equation(algebraic_r arg, id type = ID_equation);
    static size_t required_memory(id i, algebraic_r arg);

    // Building equations from one or two arguments
    equation(id op, algebraic_r arg, id type = ID_equation);
    static size_t required_memory(id i, id op, algebraic_r arg);
    equation(id op, algebraic_r x, algebraic_r y, id type = ID_equation);
    static size_t required_memory(id i, id op, algebraic_r x, algebraic_r y);

    object_p quoted(id type) const;
    static size_t size_in_equation(object_p obj);

    static equation_p make(algebraic_r x,
                           id type = ID_equation)
    {
        if (!x.Safe())
            return nullptr;
        return rt.make<equation>(type, x);
    }

    static equation_p make(id op, algebraic_r x,
                           id type = ID_equation)
    {
        if (!x.Safe())
            return nullptr;
        return rt.make<equation>(type, op, x);
    }

    static equation_p make(id op, algebraic_r x, algebraic_r y,
                           id type = ID_equation)
    {
        if (!x.Safe() || !y.Safe())
            return nullptr;
        return rt.make<equation>(type, op, x, y);
    }

    equation_p rewrite(equation_r from, equation_r to) const;
    equation_p rewrite(equation_p from, equation_p to) const
    {
        return rewrite(equation_g(from), equation_g(to));
    }

    static equation_p rewrite(equation_r eq, equation_r from, equation_r to)
    {
        return eq->rewrite(from, to);
    }

    template<typename from_eq, typename to_eq>
    equation_p rewrite(from_eq from, to_eq to) const
    {
        return rewrite(from.as_equation(), to.as_equation());
    }

    template <typename from_eq, typename to_eq, typename ...args>
    equation_p rewrite(from_eq from, to_eq to, args... rest) const
    {
        if (equation_p eq = rewrite(from, to))
            return eq->rewrite(rest...);
        return nullptr;
    }

    equation_p expand() const;
    equation_p collect() const;
    equation_p simplify() const;

protected:
    static symbol_g render(uint depth, int &precedence, bool edit);
    static symbol_g parentheses(symbol_g what);
    static symbol_g space(symbol_g what);

public:
    OBJECT_DECL(equation);
    PARSE_DECL(equation);
    RENDER_DECL(equation);
};



// ============================================================================
//
//    C++ equation building (to create rules in C++ code)
//
// ============================================================================

template <byte ...args>
struct eq
// ----------------------------------------------------------------------------
//   A static equation builder for C++ code
// ----------------------------------------------------------------------------
{
    eq() {}
    static constexpr byte data[sizeof...(args)] = { args... };
    static constexpr byte object_data[sizeof...(args) + 2] =
    {
        object::ID_equation,
        byte(sizeof...(args)),  // Must be less than 128...
        args...
    };
    constexpr object_p as_object() const
    {
        return object_p(object_data);
    }
    constexpr equation_p as_equation() const
    {
        return equation_p(object_data);
    }

    // Negation operation
    eq<args..., object::ID_neg>
    operator-()         { return eq<args..., object::ID_neg>(); }

#define EQ_FUNCTION(name)                                       \
    eq<args..., object::ID_##name>                              \
    name()         { return eq<args..., object::ID_##name>(); }

    EQ_FUNCTION(sqrt);
    EQ_FUNCTION(cbrt);

    EQ_FUNCTION(sin);
    EQ_FUNCTION(cos);
    EQ_FUNCTION(tan);
    EQ_FUNCTION(asin);
    EQ_FUNCTION(acos);
    EQ_FUNCTION(atan);

    EQ_FUNCTION(sinh);
    EQ_FUNCTION(cosh);
    EQ_FUNCTION(tanh);
    EQ_FUNCTION(asinh);
    EQ_FUNCTION(acosh);
    EQ_FUNCTION(atanh);

    EQ_FUNCTION(log1p);
    EQ_FUNCTION(expm1);
    EQ_FUNCTION(log);
    EQ_FUNCTION(log10);
    EQ_FUNCTION(log2);
    EQ_FUNCTION(exp);
    EQ_FUNCTION(exp10);
    EQ_FUNCTION(exp2);
    EQ_FUNCTION(erf);
    EQ_FUNCTION(erfc);
    EQ_FUNCTION(tgamma);
    EQ_FUNCTION(lgamma);

    EQ_FUNCTION(abs);
    EQ_FUNCTION(sign);
    EQ_FUNCTION(inv);
    EQ_FUNCTION(neg);
    EQ_FUNCTION(sq);
    EQ_FUNCTION(cubed);
    EQ_FUNCTION(fact);

    EQ_FUNCTION(re);
    EQ_FUNCTION(im);
    EQ_FUNCTION(arg);
    EQ_FUNCTION(conj);

#undef EQ_FUNCTION

    // Arithmetic
    template<byte ...y>
    eq<args..., y..., object::ID_add>
    operator+(eq<y...>) { return eq<args..., y..., object::ID_add>(); }

    template<byte ...y>
    eq<args..., y..., object::ID_sub>
    operator-(eq<y...>) { return eq<args..., y..., object::ID_sub>(); }

    template<byte ...y>
    eq<args..., y..., object::ID_mul>
    operator*(eq<y...>) { return eq<args..., y..., object::ID_mul>(); }

    template<byte ...y>
    eq<args..., y..., object::ID_div>
    operator/(eq<y...>) { return eq<args..., y..., object::ID_div>(); }

    template<byte ...y>
    eq<args..., y..., object::ID_mod>
    operator%(eq<y...>) { return eq<args..., y..., object::ID_mod>(); }

    template<byte ...y>
    eq<args..., y..., object::ID_rem>
    rem(eq<y...>) { return eq<args..., y..., object::ID_rem>(); }

    template<byte ...y>
    eq<args..., y..., object::ID_pow>
    operator^(eq<y...>) { return eq<args..., y..., object::ID_pow>(); }

    template<byte ...y>
    eq<args..., y..., object::ID_pow>
    pow(eq<y...>) { return eq<args..., y..., object::ID_pow>(); }

    // Comparisons
    template<byte ...y>
    eq<args..., y..., object::ID_TestLT>
    operator<(eq<y...>) { return eq<args..., y..., object::ID_TestLT>(); }

    template<byte ...y>
    eq<args..., y..., object::ID_TestEQ>
    operator==(eq<y...>) { return eq<args..., y..., object::ID_TestEQ>(); }

    template<byte ...y>
    eq<args..., y..., object::ID_TestGT>
    operator>(eq<y...>) { return eq<args..., y..., object::ID_TestGT>(); }

    template<byte ...y>
    eq<args..., y..., object::ID_TestLE>
    operator<=(eq<y...>) { return eq<args..., y..., object::ID_TestLE>(); }

    template<byte ...y>
    eq<args..., y..., object::ID_TestNE>
    operator!=(eq<y...>) { return eq<args..., y..., object::ID_TestNE>(); }

    template<byte ...y>
    eq<args..., y..., object::ID_TestGE>
    operator>=(eq<y...>) { return eq<args..., y..., object::ID_TestGE>(); }

};

#define EQ_FUNCTION(name)                               \
    template<byte ...x>                                 \
    eq<x..., object::ID_##name>                         \
    name(eq<x...> xeq)         { return xeq.name(); }


EQ_FUNCTION(sqrt);
EQ_FUNCTION(cbrt);

EQ_FUNCTION(sin);
EQ_FUNCTION(cos);
EQ_FUNCTION(tan);
EQ_FUNCTION(asin);
EQ_FUNCTION(acos);
EQ_FUNCTION(atan);

EQ_FUNCTION(sinh);
EQ_FUNCTION(cosh);
EQ_FUNCTION(tanh);
EQ_FUNCTION(asinh);
EQ_FUNCTION(acosh);
EQ_FUNCTION(atanh);

EQ_FUNCTION(log1p);
EQ_FUNCTION(expm1);
EQ_FUNCTION(log);
EQ_FUNCTION(log10);
EQ_FUNCTION(log2);
EQ_FUNCTION(exp);
EQ_FUNCTION(exp10);
EQ_FUNCTION(exp2);
EQ_FUNCTION(erf);
EQ_FUNCTION(erfc);
EQ_FUNCTION(tgamma);
EQ_FUNCTION(lgamma);

EQ_FUNCTION(abs);
EQ_FUNCTION(sign);
EQ_FUNCTION(inv);
EQ_FUNCTION(neg);
EQ_FUNCTION(sq);
EQ_FUNCTION(cubed);
EQ_FUNCTION(fact);

EQ_FUNCTION(re);
EQ_FUNCTION(im);
EQ_FUNCTION(arg);
EQ_FUNCTION(conj);

#undef EQ_FUNCTION

// Pi constant
struct eq_pi : eq<object::ID_pi> {};

// Build a symbol out of a character
template <byte c>       struct eq_symbol  : eq<object::ID_symbol,  1, c> {};

// Build an integer constant
template <uint c, std::enable_if_t<(c >= 0 && c < 128), bool> = true>
struct eq_integer : eq<object::ID_integer, byte(c)> {};
template <uint c, std::enable_if_t<(c >= 0 && c < 128), bool> = true>
struct eq_neg_integer : eq<object::ID_neg_integer, byte(-c)> {};



// ============================================================================
//
//   User commands
//
// ============================================================================

COMMAND_DECLARE(Rewrite);

#endif // EQUATION_H
