#ifndef OBJECT_H
#define OBJECT_H
// ****************************************************************************
//  object.h                                                      DB48X project
// ****************************************************************************
//
//   File Description:
//
//     The basic RPL object
//
//     An RPL object is a bag of bytes densely encoded using LEB128
//
//     It is important that the base object be empty, sizeof(object) == 1
//
//
//
//
// ****************************************************************************
//   (C) 2022 Christophe de Dinechin <christophe@dinechin.org>
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
// Object encoding:
//
//    RPL objects are encoded using sequence of LEB128 values
//    An LEB128 value is a variable-length encoding with 7 bits per byte,
//    the last byte having its high bit clear. This, values 0-127 are coded
//    as 0-127, values up to 16384 are coded on two bytes, and so on.
//
//    All objects begin with an "identifier" (the type is called id in the code)
//    which uniquely defines the type of the object. Identifiers are defined in
//    the source file called ids.tbl.
//
//    For commands, the object type is all there is to the object. Therefore,
//    most RPL commands consume only one byte in memory. For other objects,
//    there is a "payload" following that identifier. The format of the paylaod
//    is described in the source file for the corresponding object type, but a
//    general guiding principle is that the payload must make it easy to skip
//    the object in memory, notably during garbage collection.
//
// Object handler:
//
//    The type of the object is an index in an object-handler table, so they act
//    either as commands (performing an action when evaluated) or as data types
//    (putting themselves on the runtime stack when evaluated).
//
//    All handlers must respond to a fixed number of "opcodes", which are
//    reserved identifiers in ids.tbl. These opcodes also correspond do
//    user-accessible commands that apply to objects. They include:
//
//    - EVAL:   Evaluates the object
//    - SIZE:   Compute the size of the object
//    - PARSE:  Try to parse an object of the type (see note)
//    - RENDER: Render an object as text
//    - HELP:   Return the name of the help topic associated to the object
//
//    Note: PARSE is the only opcode that does not take an object as user_interface
//
//    The handler is not exactly equivalent to the user command.
//    It may present an internal interface that is more convenient for C code.
//    This approach makes it possible to pass other IDs to an object, for
//    example the "add" operator can delegate the addition of complex numbers
//    to the complex handler by calling the complex handler with 'add'.
//
// Rationale:
//
//    The reason for this design is that the DM42 is very memory-starved
//    (~70K available to DMCP programs), so the focus is on a format for objects
//    that is extremely compact.
//
//    Notably, for size encoding, with only 70K available, the chances of a size
//    exceeding 2 bytes (16384) are exceedingly rare.
//
//    We can also use the lowest opcodes for the most frequently used features,
//    ensuring that 128 of them can be encoded on one byte only. Similarly, all
//    constants less than 128 can be encoded in two bytes only (one for the
//    opcode, one for the value), and all constants less than 16384 are encoded
//    on three bytes.
//
//    Similarly, the design of RPL calls for a garbage collector.
//    The format of objects defined above ensures that all objects are moveable.
//    The garbage collector can therefore be "compacting", moving all live
//    objects at the beginning of memory. This in turns means that each garbage
//    collection cycle gives us a large amount of contiguous memory, but more
//    importantly, that the allocation of new objects is extremely simple, and
//    therefore quite fast.
//
//    The downside is that we can't really use the C++ built-in dyanmic dispatch
//    mechanism (virtual functions), as having a pointer to a vtable would
//    increase the size of the object too much.


#include "leb128.h"
#include "precedence.h"
#include "recorder.h"
#include "types.h"

RECORDER_DECLARE(object);
RECORDER_DECLARE(parse);
RECORDER_DECLARE(parse_attempts);
RECORDER_DECLARE(render);
RECORDER_DECLARE(eval);
RECORDER_DECLARE(run);
RECORDER_DECLARE(object_errors);

struct algebraic;
struct menu_info;
struct object;
struct parser;
struct program;
struct renderer;
struct grapher;
struct runtime;
struct symbol;
struct text;
struct grob;
struct user_interface;

typedef const algebraic *algebraic_p;
typedef const object    *object_p;
typedef const program   *program_p;
typedef const symbol    *symbol_p;
typedef const text      *text_p;
typedef const grob      *grob_p;

struct object
// ----------------------------------------------------------------------------
//  The basic RPL object
// ----------------------------------------------------------------------------
{
    enum id : unsigned
    // ------------------------------------------------------------------------
    //  Object ID
    // ------------------------------------------------------------------------
    {
#define ID(i)   ID_##i,
#include "ids.tbl"
        NUM_IDS
    };

    object(id i)
    // ------------------------------------------------------------------------
    //  Write the id of the object
    // ------------------------------------------------------------------------
    {
        byte *ptr = (byte *) this;
        leb128(ptr, i);
    }
    ~object() {}


    // ========================================================================
    //
    //   Object protocol
    //
    // ========================================================================

    enum result
    // -------------------------------------------------------------------------
    //   Return values for parsing
    // -------------------------------------------------------------------------
    {
        OK,                     // Command ran successfully
        SKIP,                   // Command not for this handler, try next
        ERROR,                  // Error processing the command
        WARN,                   // Possible error (if no object succeeds)
        COMMENTED               // Code is commented out
    };


    typedef size_t      (*size_fn)(object_p o);
    typedef result      (*parse_fn)(parser &p);
    typedef utf8        (*help_fn)(object_p o);
    typedef result      (*evaluate_fn)(object_p o);
    typedef result      (*execute_fn)(object_p o);
    typedef size_t      (*render_fn)(object_p o, renderer &r);
    typedef grob_p      (*graph_fn)(object_p o, grapher &g);
    typedef result      (*insert_fn)(object_p o, user_interface &i);
    typedef bool        (*menu_fn)(object_p o, menu_info &m);
    typedef unicode     (*menu_marker_fn)(object_p o);

    struct dispatch
    // ------------------------------------------------------------------------
    //   Operations that can be run on an object
    // ------------------------------------------------------------------------
    {
        cstring         name;            // Basic (compatibility) name
        cstring         fancy;           // Fancy name
        size_fn         size;            // Compute object size in bytes
        parse_fn        parse;           // Parse an object
        help_fn         help;            // Return help topic
        evaluate_fn     evaluate;        // Evaluate the object
        execute_fn      execute;         // Execute the object
        render_fn       render;          // Render the object as text
        graph_fn        graph;           // Render the object as a grob
        insert_fn       insert;          // Insert object in editor
        menu_fn         menu;            // Build menu entries
        menu_marker_fn  menu_marker;     // Show marker
        uint            arity;           // Number of input arguments
        uint            precedence;      // Precedence in equations
        bool            is_type      :1; // Is a data type
        bool            is_integer   :1; // Is an integer type
        bool            is_based     :1; // Is a based integer type
        bool            is_bignum    :1; // Is a bignum type
        bool            is_fraction  :1; // Is a fraction type
        bool            is_real      :1; // Is a real type (excludes based ints)
        bool            is_decimal   :1; // Is a decimal type
        bool            is_complex   :1; // Is a complex (but not real) type
        bool            is_command   :1; // Is an RPL command
        bool            is_symbolic  :1; // Is a symbol or an equation
        bool            is_algebraic :1; // Algebraic functions (in equations)
        bool            is_immediate :1; // Commands that execute immediately
    };


    // ========================================================================
    //
    //   Memory management
    //
    // ========================================================================

    static size_t required_memory(id i)
    // ------------------------------------------------------------------------
    //  Compute the amount of memory required for an object
    // ------------------------------------------------------------------------
    {
        return leb128size(i);
    }


    id type() const
    // ------------------------------------------------------------------------
    //   Return the type of the object
    // ------------------------------------------------------------------------
    {
        byte *ptr = (byte *) this;
        id ty = (id) leb128<uint16_t>(ptr);
        if (ty > NUM_IDS)
        {
            object_error(ty, this);
            ty = ID_object;
        }
        return ty;
    }


    const dispatch &ops() const
    // ------------------------------------------------------------------------
    //   Return the handlers for the current object
    // ------------------------------------------------------------------------
    {
        return handler[type()];
    }


    size_t size() const
    // ------------------------------------------------------------------------
    //  Compute the size of the object by calling the handler with SIZE
    // ------------------------------------------------------------------------
    {
        return ops().size(this);
    }


    object_p skip() const
    // ------------------------------------------------------------------------
    //  Return the pointer to the next object in memory by skipping its size
    // ------------------------------------------------------------------------
    {
        return this + size();
    }


    template <typename Obj>
    static byte_p payload(const Obj *p)
    // ------------------------------------------------------------------------
    //  Return the object's payload, i.e. first byte after ID
    // ------------------------------------------------------------------------
    //  When we can, use the static type to know how many bytes to skip
    {
        return byte_p(p) + (Obj::static_id < 0x80 ? 1 : 2);
    }

    byte_p payload() const
    // ------------------------------------------------------------------------
    //  Return the object's payload, i.e. first byte after ID
    // ------------------------------------------------------------------------
    {
        return byte_p(leb128skip(this));
    }


    static void object_error(id type, const object *ptr);
    // ------------------------------------------------------------------------
    //   Report an error e.g. with with an object type
    // ------------------------------------------------------------------------



    // ========================================================================
    //
    //    High-level functions on objects
    //
    // ========================================================================

    result evaluate() const
    // ------------------------------------------------------------------------
    //  Evaluate an object by calling the handler
    // ------------------------------------------------------------------------
    {
        record(eval, "Evaluating %+s %p", name(), this);
        return ops().evaluate(this);
    }


    result execute() const
    // ------------------------------------------------------------------------
    //   Execute the object, i.e. run programs and equations
    // ------------------------------------------------------------------------
    {
        record(eval, "Executing %+s %p", name(), this);
        return ops().execute(this);
    }


    size_t render(renderer &r) const
    // ------------------------------------------------------------------------
    //   Render the object into an existing renderer
    // ------------------------------------------------------------------------
    {
        record(render, "Rendering %+s %p into %p", name(), this, &r);
        return ops().render(this, r);
    }


    size_t render(char *output, size_t length) const;
    // ------------------------------------------------------------------------
    //   Render the object into a static buffer
    // ------------------------------------------------------------------------


    cstring edit() const;
    // ------------------------------------------------------------------------
    //   Render the object into the scratchpad, then move into the editor
    // ------------------------------------------------------------------------


    grob_p graph(grapher &g) const
    // ------------------------------------------------------------------------
    //   Render the object into an existing grapher
    // ------------------------------------------------------------------------
    {
        record(render, "Graphing %+s %p into %p", name(), this, &g);
        return ops().graph(this, g);
    }


    text_p as_text(bool edit = true, bool eq = false) const;
    // ------------------------------------------------------------------------
    //   Return the object as text
    // ------------------------------------------------------------------------


    symbol_p as_symbol(bool editing) const
    // ------------------------------------------------------------------------
    //   Return the object as text
    // ------------------------------------------------------------------------
    {
        return symbol_p(as_text(editing, true));
    }


    grob_p as_grob() const;
    // ------------------------------------------------------------------------
    //   Return the object as a pixel graphic object
    // ------------------------------------------------------------------------


    uint32_t as_uint32(uint32_t def = 0, bool err = true) const;
    int32_t  as_int32 (int32_t  def = 0, bool err = true)  const;
    // ------------------------------------------------------------------------
    //   Return the object as an integer, possibly erroring out for bad type
    // ------------------------------------------------------------------------


    object_p at(size_t index, bool err = true) const;
    // ------------------------------------------------------------------------
    //   Extract a subobject at given index, works for list, array and text
    // ------------------------------------------------------------------------


    result insert(user_interface &i) const
    // ------------------------------------------------------------------------
    //   Insert in the editor at cursor position, with possible offset
    // ------------------------------------------------------------------------
    {
        return ops().insert(this, i);
    }


    static object_p parse(utf8     source,
                          size_t  &size,
                          int      precedence = 0);
    // ------------------------------------------------------------------------
    //  Try parsing the object as a top-level temporary
    // ------------------------------------------------------------------------
    //  If precedence != 0, parse as an equation object with that precedence


    utf8 help() const
    // ------------------------------------------------------------------------
    //   Return the help topic for the given object
    // ------------------------------------------------------------------------
    {
        return ops().help(this);
    }


    static cstring name(result r)
    // ------------------------------------------------------------------------
    //    Convenience function for the name of results
    // ------------------------------------------------------------------------
    {
        switch (r)
        {
        case OK:        return "OK";
        case SKIP:      return "SKIP";
        case ERROR:     return "ERROR";
        case WARN:      return "WARN";
        default:        return "<Unknown>";
        }
    }


    static utf8 name(id i)
    // ------------------------------------------------------------------------
    //   Return the name for a given ID
    // ------------------------------------------------------------------------
    {
        return utf8(i < NUM_IDS ? handler[i].name : "<invalid ID>");
    }


    static utf8 fancy(id i)
    // ------------------------------------------------------------------------
    //   Return the fancy name for a given ID
    // ------------------------------------------------------------------------
    {
        return utf8(i < NUM_IDS ? handler[i].fancy : "<Invalid ID>");
    }


    utf8 name() const
    // ------------------------------------------------------------------------
    //   Return the name for the current object
    // ------------------------------------------------------------------------
    {
        return name(type());
    }


    utf8 fancy() const
    // ------------------------------------------------------------------------
    //   Return the fancy name for the current object
    // ------------------------------------------------------------------------
    {
        return fancy(type());
    }


    unicode marker() const
    // ------------------------------------------------------------------------
    //   Marker in menus
    // ------------------------------------------------------------------------
    {
        return ops().menu_marker(this);
    }



    // ========================================================================
    //
    //    Attributes of objects
    //
    // ========================================================================

    static bool is_type(id ty)
    // -------------------------------------------------------------------------
    //   Check if a type is for an RPL data type
    // -------------------------------------------------------------------------
    {
        return handler[ty].is_type;
    }


    bool is_type() const
    // -------------------------------------------------------------------------
    //   Check if an object is an integer
    // -------------------------------------------------------------------------
    {
        return is_type(type());
    }


    static bool is_integer(id ty)
    // -------------------------------------------------------------------------
    //   Check if a type is an integer
    // -------------------------------------------------------------------------
    {
        return handler[ty].is_integer;
    }


    bool is_integer() const
    // -------------------------------------------------------------------------
    //   Check if an object is an integer
    // -------------------------------------------------------------------------
    {
        return is_integer(type());
    }


    static bool is_based(id ty)
    // -------------------------------------------------------------------------
    //   Check if a type is a based integer
    // -------------------------------------------------------------------------
    {
        return handler[ty].is_based;
    }


    bool is_based() const
    // -------------------------------------------------------------------------
    //   Check if an object is a based integer
    // -------------------------------------------------------------------------
    {
        return is_based(type());
    }


    static bool is_bignum(id ty)
    // -------------------------------------------------------------------------
    //   Check if a type is a big integer
    // -------------------------------------------------------------------------
    {
        return handler[ty].is_bignum;
    }


    bool is_bignum() const
    // -------------------------------------------------------------------------
    //   Check if an object is a big integer
    // -------------------------------------------------------------------------
    {
        return is_bignum(type());
    }


    bool is_big() const;
    // ------------------------------------------------------------------------
    //   Check if any component is a bignum
    // ------------------------------------------------------------------------


    static bool is_fraction(id ty)
    // -------------------------------------------------------------------------
    //   Check if a type is a fraction
    // -------------------------------------------------------------------------
    {
        return handler[ty].is_fraction;
    }


    bool is_fraction() const
    // -------------------------------------------------------------------------
    //   Check if an object is an integer
    // -------------------------------------------------------------------------
    {
        return is_fraction(type());
    }


    static bool is_fractionable(id ty)
    // -------------------------------------------------------------------------
    //   Check if a type is a fraction or a non-based integer
    // -------------------------------------------------------------------------
    {
        return handler[ty].is_fraction ||
            (handler[ty].is_integer && handler[ty].is_real);
    }


    bool is_fractionable() const
    // -------------------------------------------------------------------------
    //   Check if an object is a fraction or an integer
    // -------------------------------------------------------------------------
    {
        return is_fractionable(type());
    }


    static bool is_decimal(id ty)
    // -------------------------------------------------------------------------
    //   Check if a type is a decimal
    // -------------------------------------------------------------------------
    {
        return handler[ty].is_decimal;
    }


    bool is_decimal() const
    // -------------------------------------------------------------------------
    //   Check if an object is a decimal
    // -------------------------------------------------------------------------
    {
        return is_decimal(type());
    }


    static  bool is_real(id ty)
    // -------------------------------------------------------------------------
    //   Check if a type is a real number
    // -------------------------------------------------------------------------
    {
        return handler[ty].is_real;
    }


    bool is_real() const
    // -------------------------------------------------------------------------
    //   Check if an object is a real number
    // -------------------------------------------------------------------------
    {
        return is_real(type());
    }


    static  bool is_complex(id ty)
    // -------------------------------------------------------------------------
    //   Check if a type is a complex number
    // -------------------------------------------------------------------------
    {
        return handler[ty].is_complex;
    }


    bool is_complex() const
    // -------------------------------------------------------------------------
    //   Check if an object is a complex number
    // -------------------------------------------------------------------------
    {
        return is_complex(type());
    }


    static bool is_unit(id ty)
    // -------------------------------------------------------------------------
    //   Check if a type is a unit object
    // -------------------------------------------------------------------------
    {
        return ty == ID_unit;
    }


    bool is_unit() const
    // -------------------------------------------------------------------------
    //   Check if an object is a unit object
    // -------------------------------------------------------------------------
    {
        return is_unit(type());
    }


    static bool is_command(id ty)
    // ------------------------------------------------------------------------
    //    Check if a type denotes a command
    // ------------------------------------------------------------------------
    {
        return handler[ty].is_command;
    }


    bool is_command() const
    // ------------------------------------------------------------------------
    //   Check if an object is a command
    // ------------------------------------------------------------------------
    {
        return is_command(type());
    }


    static bool is_immediate(id ty)
    // ------------------------------------------------------------------------
    //    Check if a type denotes an immediate command (e.g. menus)
    // ------------------------------------------------------------------------
    {
        return handler[ty].is_immediate;
    }


    bool is_immediate() const
    // ------------------------------------------------------------------------
    //   Check if an object is an immediate command (e.g. menus)
    // ------------------------------------------------------------------------
    {
        return is_immediate(type());
    }


    static bool is_algebraic_number(id ty)
    // ------------------------------------------------------------------------
    //   Check if something is a number (real or complex)
    // ------------------------------------------------------------------------
   {
       return is_real(ty) || is_complex(ty) || is_unit(ty);
    }


    bool is_algebraic_number() const
    // ------------------------------------------------------------------------
    //   Check if something is a number (real or complex)
    // ------------------------------------------------------------------------
    {
        return is_algebraic_number(type());
    }


    static bool is_symbolic(id ty)
    // ------------------------------------------------------------------------
    //    Check if a type denotes a symbolic argument (symbol, equation, number)
    // ------------------------------------------------------------------------
    {
        return handler[ty].is_symbolic || is_algebraic_number(ty);
    }


    bool is_symbolic() const
    // ------------------------------------------------------------------------
    //   Check if an object is a symbolic argument
    // ------------------------------------------------------------------------
    {
        return is_symbolic(type());
    }


    static bool is_strictly_symbolic(id ty)
    // ------------------------------------------------------------------------
    //    Check if a type denotes a symbol or equation
    // ------------------------------------------------------------------------
    {
        return handler[ty].is_symbolic;
    }


    bool is_strictly_symbolic() const
    // ------------------------------------------------------------------------
    //   Check if an object is a symbol or equation
    // ------------------------------------------------------------------------
    {
        return is_strictly_symbolic(type());
    }


    static bool is_algebraic_function(id ty)
    // ------------------------------------------------------------------------
    //    Check if a type denotes an algebraic function
    // ------------------------------------------------------------------------
    {
        return handler[ty].is_algebraic;
    }


    bool is_algebraic_function() const
    // ------------------------------------------------------------------------
    //   Check if an object is an algebraic function
    // ------------------------------------------------------------------------
    {
        return is_algebraic_function(type());
    }


    static bool is_algebraic(id ty)
    // ------------------------------------------------------------------------
    //    Check if a type denotes an algebraic value or function
    // ------------------------------------------------------------------------
    {
        return is_algebraic_function(ty) || is_symbolic(ty);
    }


    bool is_algebraic() const
    // ------------------------------------------------------------------------
    //   Check if an object is an algebraic function
    // ------------------------------------------------------------------------
    {
        return is_algebraic(type());
    }


    algebraic_p as_algebraic() const
    // ------------------------------------------------------------------------
    //   Return an object as an algebraic if possible, or nullptr
    // ------------------------------------------------------------------------
    {
        if (is_algebraic())
            return algebraic_p(this);
        return nullptr;
    }


    static bool is_plot(id ty)
    // ------------------------------------------------------------------------
    //   Check if a type name denotes a plot type
    // ------------------------------------------------------------------------
    {
        return ty >= ID_Function && ty <= ID_Parametric;
    }


    bool is_plot() const
    // ------------------------------------------------------------------------
    //   Check if an object is a plot type
    // ------------------------------------------------------------------------
    {
        return is_plot(type());
    }


    uint arity() const
    // ------------------------------------------------------------------------
    //   Return the arity for arithmetic operators
    // ------------------------------------------------------------------------
    {
        return ops().arity;
    }


    uint precedence() const
    // ------------------------------------------------------------------------
    //   Return the arity for arithmetic operators
    // ------------------------------------------------------------------------
    {
        return ops().precedence;
    }


    template<typename Obj> const Obj *as() const
    // ------------------------------------------------------------------------
    //   Type-safe cast (note: only for exact type match)
    // ------------------------------------------------------------------------
    {
        if (type() == Obj::static_id)
            return (const Obj *) this;
        return nullptr;
    }


    template<typename Obj, typename Derived> const Obj *as() const
    // ------------------------------------------------------------------------
    //   Type-safe cast (note: only for exact type match)
    // ------------------------------------------------------------------------
    {
        id t = type();
        if (t >= Obj::static_id && t <= Derived::static_id)
            return (const Obj *) this;
        return nullptr;
    }


    object_p as_quoted(id ty = ID_symbol) const;
    template<typename T>
    const T *as_quoted() const { return (const T *) as_quoted(T::static_id); }
    // ------------------------------------------------------------------------
    //    Return object as a valid quoted name (e.g. 'ABC')
    // ------------------------------------------------------------------------


    int as_truth(bool error = true) const;
    // ------------------------------------------------------------------------
    //   Return 0 or 1 if this is a logical value, -1 and type error otherwise
    // ------------------------------------------------------------------------


    bool is_zero(bool error = true) const;
    // ------------------------------------------------------------------------
    //   Return true if this is zero, false otherwise. Can error if bad type
    // ------------------------------------------------------------------------


    bool is_one(bool error = true) const;
    // ------------------------------------------------------------------------
    //   Return true if this is one, false otherwise. Can error if bad type
    // ------------------------------------------------------------------------


    bool is_negative(bool error = true) const;
    // ------------------------------------------------------------------------
    //   Return true if this is negative, false otherwise, can error if bad
    // ------------------------------------------------------------------------



    int compare_to(object_p other) const;
    // ------------------------------------------------------------------------
    //   Compare two objects and return a signed comparison
    // ------------------------------------------------------------------------


    bool is_same_as(object_p other) const
    // ------------------------------------------------------------------------
    //   Compare two objects
    // ------------------------------------------------------------------------
    {
        return compare_to(other) == 0;
    }


    object_p child(uint index = 0) const;
    // ------------------------------------------------------------------------
    //   Return a child for a complex, list or array
    // ------------------------------------------------------------------------


    algebraic_p algebraic_child(uint index = 0) const;
    // ------------------------------------------------------------------------
    //   Return an algebraic child for a complex, list or array
    // ------------------------------------------------------------------------



    // ========================================================================
    //
    //    Default implementations for object interface
    //
    // ========================================================================

#define OBJECT_DECL(D)  static const id static_id = ID_##D;
#define PARSE_DECL(D)   static result   do_parse(parser &p UNUSED)
#define HELP_DECL(D)    static utf8     do_help(const D *o UNUSED)
#define EVAL_DECL(D)    static result   do_evaluate(const D *o UNUSED)
#define EXEC_DECL(D)    static result   do_execute(const D *o UNUSED)
#define SIZE_DECL(D)    static size_t   do_size(const D *o UNUSED)
#define RENDER_DECL(D)  static size_t   do_render(const D *o UNUSED,renderer &r UNUSED)
#define GRAPH_DECL(D)   static grob_p   do_graph(const D *o UNUSED,grapher &g UNUSED)
#define INSERT_DECL(D)  static result   do_insert(const D *o UNUSED)
#define MENU_DECL(D)    static bool     do_menu(const D *o UNUSED, menu_info &mi UNUSED)
#define MARKER_DECL(D)  static unicode  do_menu_marker(const D *o UNUSED)
#define ARITY_DECL(A)   enum { ARITY = A }
#define PREC_DECL(P)    enum { PRECEDENCE = precedence::P }

    OBJECT_DECL(object);
    PARSE_DECL(object);
    HELP_DECL(object);
    EVAL_DECL(object);
    EXEC_DECL(object);
    SIZE_DECL(object);
    RENDER_DECL(object);
    GRAPH_DECL(object);
    INSERT_DECL(object);
    MENU_DECL(object);
    MARKER_DECL(object);
    ARITY_DECL(0);
    PREC_DECL(NONE);

    template <typename T, typename U>
    static intptr_t ptrdiff(T *t, U *u)
    {
        return (byte *) t - (byte *) u;
    }


protected:
    static const dispatch   handler[NUM_IDS];

#if SIMULATOR
public:
    cstring debug() const;
#endif
};

#define PARSE_BODY(D)   object::result D::do_parse(parser &p UNUSED)
#define HELP_BODY(D)    utf8           D::do_help(const D *o UNUSED)
#define EVAL_BODY(D)    object::result D::do_evaluate(const D *o UNUSED)
#define EXEC_BODY(D)    object::result D::do_execute(const D *o UNUSED)
#define SIZE_BODY(D)    size_t         D::do_size(const D *o UNUSED)
#define RENDER_BODY(D)  size_t         D::do_render(const D *o UNUSED, renderer &r UNUSED)
#define GRAPH_BODY(D)   grob_p         D::do_graph(const D *o UNUSED, grapher &g UNUSED)
#define INSERT_BODY(D)  object::result D::do_insert(const D *o UNUSED)
#define MENU_BODY(D)    bool           D::do_menu(const D *o UNUSED, menu_info &mi UNUSED)
#define MARKER_BODY(D)  unicode        D::do_menu_marker(const D *o UNUSED)

template <typename RPL>
object::result run()
// ----------------------------------------------------------------------------
//  Run a given RPL opcode directly
// ----------------------------------------------------------------------------
{
    const RPL *obj = (const RPL *) RPL::static_object(RPL::static_id);
    return RPL::do_evaluate(obj);
}

#endif // OBJECT_H
