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
//    Note: PARSE is the only opcode that does not take an object as input
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

struct runtime;
struct parser;
struct renderer;
struct object;
struct symbol;
struct program;
struct input;
struct text;
struct menu_info;

RECORDER_DECLARE(object);
RECORDER_DECLARE(parse);
RECORDER_DECLARE(parse_attempts);
RECORDER_DECLARE(render);
RECORDER_DECLARE(eval);
RECORDER_DECLARE(run);
RECORDER_DECLARE(object_errors);

typedef const object  *object_p;
typedef const symbol  *symbol_p;
typedef const program *program_p;
typedef const text    *text_p;


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
    // ----------------------------------------------------------------------------
    //   Return values for parsing
    // ----------------------------------------------------------------------------
    {
        OK,                     // Command ran successfully
        SKIP,                   // Command not for this handler, try next
        ERROR,                  // Error processing the command
        WARN ,                  // Possible error (if no object succeeds)
    };


    typedef size_t      (*size_fn)(object_p o);
    typedef result      (*parse_fn)(parser &p);
    typedef utf8        (*help_fn)(object_p o);
    typedef result      (*evaluate_fn)(object_p o);
    typedef result      (*execute_fn)(object_p o);
    typedef size_t      (*render_fn)(object_p o, renderer &p);
    typedef result      (*insert_fn)(object_p o, input &i);
    typedef bool        (*menu_fn)(object_p o, menu_info &m);
    typedef unicode     (*menu_marker_fn)(object_p o);

    struct dispatch
    // ----------------------------------------------------------------------------
    //   Operations that can be run on an object
    // ----------------------------------------------------------------------------
    {
        cstring        name;            // Basic (compatibility) name
        cstring        fancy;           // Fancy name
        size_fn        size;            // Compute object size in bytes
        parse_fn       parse;           // Parse an object
        help_fn        help;            // Return help topic
        evaluate_fn    evaluate;        // Evaluate the object
        execute_fn     execute;         // Execute the object
        render_fn      render;          // Render the object as text
        insert_fn      insert;          // Insert object in editor
        menu_fn        menu;            // Build menu entries
        menu_marker_fn menu_marker;     // Show marker
        uint           arity;           // Number of input arguments
        uint           precedence;      // Precedence in equations
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


    result insert(input &i) const
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

    static bool is_integer(id ty)
    // -------------------------------------------------------------------------
    //   Check if a type is an integer
    // -------------------------------------------------------------------------
    {
        return ty >= FIRST_INTEGER_TYPE && ty <= LAST_INTEGER_TYPE;
    }


    bool is_integer() const
    // -------------------------------------------------------------------------
    //   Check if an object is an integer
    // -------------------------------------------------------------------------
    {
        return is_integer(type());
    }


    static bool is_bignum(id ty)
    // -------------------------------------------------------------------------
    //   Check if a type is a big integer
    // -------------------------------------------------------------------------
    {
        return ty >= FIRST_BIGNUM_TYPE && ty <= LAST_BIGNUM_TYPE;
    }


    bool is_bignum() const
    // -------------------------------------------------------------------------
    //   Check if an object is a big integer
    // -------------------------------------------------------------------------
    {
        return is_bignum(type());
    }


    static bool is_fraction(id ty)
    // -------------------------------------------------------------------------
    //   Check if a type is a fraction
    // -------------------------------------------------------------------------
    {
        return ty >= FIRST_FRACTION_TYPE && ty <= LAST_FRACTION_TYPE;
    }


    bool is_fraction() const
    // -------------------------------------------------------------------------
    //   Check if an object is an integer
    // -------------------------------------------------------------------------
    {
        return is_fraction(type());
    }


    static bool is_decimal(id ty)
    // -------------------------------------------------------------------------
    //   Check if a type is a decimal
    // -------------------------------------------------------------------------
    {
        return ty >= FIRST_DECIMAL_TYPE && ty <= LAST_DECIMAL_TYPE;
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
        return ty >= FIRST_REAL_TYPE && ty <= LAST_REAL_TYPE;
    }


    bool is_real() const
    // -------------------------------------------------------------------------
    //   Check if an object is a real number
    // -------------------------------------------------------------------------
    {
        return is_real(type());
    }


    static bool is_command(id ty)
    // ------------------------------------------------------------------------
    //    Check if a type denotes a command
    // ------------------------------------------------------------------------
    {
        return ty >= FIRST_COMMAND && ty <= LAST_COMMAND;
    }


    bool is_command() const
    // ------------------------------------------------------------------------
    //   Check if an object is a command
    // ------------------------------------------------------------------------
    {
        return is_command(type());
    }


    static bool is_symbolic(id ty)
    // ------------------------------------------------------------------------
    //    Check if a type denotes a symbolic argument (symbol, equation, number)
    // ------------------------------------------------------------------------
    {
        return ty >= FIRST_SYMBOLIC_TYPE && ty <= LAST_SYMBOLIC_TYPE;
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
        return ty == ID_symbol || ty == ID_equation;
    }


    bool is_strictly_symbolic() const
    // ------------------------------------------------------------------------
    //   Check if an object is a symbol or equation
    // ------------------------------------------------------------------------
    {
        return is_strictly_symbolic(type());
    }


    static bool is_algebraic(id ty)
    // ------------------------------------------------------------------------
    //    Check if a type denotes an algebraic function
    // ------------------------------------------------------------------------
    {
        return ty >= FIRST_ALGEBRAIC && ty <= LAST_ALGEBRAIC;
    }


    bool is_algebraic() const
    // ------------------------------------------------------------------------
    //   Check if an object is an algebraic function
    // ------------------------------------------------------------------------
    {
        return is_algebraic(type());
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
        if (type() == Obj::static_type())
            return (const Obj *) this;
        return nullptr;
    }


    template<typename Obj, typename Derived> const Obj *as() const
    // ------------------------------------------------------------------------
    //   Type-safe cast (note: only for exact type match)
    // ------------------------------------------------------------------------
    {
        id t = type();
        if (t >= Obj::static_type() && t <= Derived::static_type())
            return (const Obj *) this;
        return nullptr;
    }


    symbol_p as_name() const;
    // ------------------------------------------------------------------------
    //    Return object as a name
    // ------------------------------------------------------------------------


    int as_truth() const;
    // ------------------------------------------------------------------------
    //   Return 0 or 1 if this is a logical value, -1 and type error otherwise
    // ------------------------------------------------------------------------


    // ========================================================================
    //
    //    Default implementations for object interface
    //
    // ========================================================================

#define OBJECT_DECL(D)  static id       static_type() { return ID_##D; }
#define PARSE_DECL(D)   static result   do_parse(parser &p UNUSED)
#define HELP_DECL(D)    static utf8     do_help(const D *o UNUSED)
#define EVAL_DECL(D)    static result   do_evaluate(const D *o UNUSED)
#define EXEC_DECL(D)    static result   do_execute(const D *o UNUSED)
#define SIZE_DECL(D)    static size_t   do_size(const D *o UNUSED)
#define RENDER_DECL(D)  static size_t   do_render(const D *o UNUSED,renderer &r UNUSED)
#define INSERT_DECL(D)  static result   do_insert(const D *o UNUSED, input &i UNUSED)
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
#define INSERT_BODY(D)  object::result D::do_insert(const D *o UNUSED, input &i UNUSED)
#define MENU_BODY(D)    bool           D::do_menu(const D *o UNUSED, menu_info &mi UNUSED)
#define MARKER_BODY(D)  unicode        D::do_menu_marker(const D *o UNUSED)

template <typename RPL>
object::result run()
// ----------------------------------------------------------------------------
//  Run a given RPL opcode directly
// ----------------------------------------------------------------------------
{
    const RPL *obj = (const RPL *) RPL::static_object(RPL::static_type());
    return RPL::do_evaluate(obj);
}

#endif // OBJECT_H
