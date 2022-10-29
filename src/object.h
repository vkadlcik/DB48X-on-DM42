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
//  RPL objects are encoded using sequence of LEB128 values
//  An LEB128 value is a variable-length encoding with 7 bits per byte,
//  the last byte having its high bit clear. This, values 0-127 are coded
//  as 0-127, values up to 16384 are coded on two bytes, and so on.
//
//  The reason for this encoding is that the DM42 is very memory-starved
//  (~70K available to DMCP programs), so chances of a size exceeding 2 bytes
//  are exceedingly rare. We can also use the lowest opcodes for the most
//  frequently used features, ensuring that 128 of them can be encoded on one
//  byte only. Similarly, all constants less than 128 can be encoded in two
//  bytes only (one for the opcode, one for the value), and all constants less
//  than 16384 are encoded on three bytes.
//
//  The opcode is an index in an object-handler table, so they act either as
//  commands or as types


#include "types.h"
#include "leb128.h"
#include "recorder.h"

struct runtime;
struct parser;
struct renderer;
struct object;

RECORDER_DECLARE(object);
RECORDER_DECLARE(parse);
RECORDER_DECLARE(parse_attempts);
RECORDER_DECLARE(render);
RECORDER_DECLARE(eval);
RECORDER_DECLARE(run);
RECORDER_DECLARE(object_errors);

typedef const object *object_p;


struct object
// ----------------------------------------------------------------------------
//  The basic RPL object
// ----------------------------------------------------------------------------
{
    enum id
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
    //   Object command protocol
    //
    // ========================================================================

    enum opcode
    // ------------------------------------------------------------------------
    //  The commands that all handlers must deal with
    // ------------------------------------------------------------------------
    {
#define RPL_OPCODE(n)           n,
#include "rpl-opcodes.tbl"
        NUM_OPCODES
    };

    enum result
    // ------------------------------------------------------------------------
    //   Common return values for handlers
    // ------------------------------------------------------------------------
    {
        OK    = 0,              // Command ran successfully
        SKIP  = -1,             // Command not for this handler, try next
        ERROR = -2,             // Error processing the command
        WARN  = -3,             // Possible error (if no object succeeds)
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
        id ty = (id) leb128(ptr);
        if (ty > NUM_IDS)
        {
            uintptr_t debug[2];
            byte *d = (byte *) debug;
            byte *s = (byte *) this;
            for (uint i = 0; i < sizeof(debug); i++)
                d[i] = s[i];
            record(object_errors,
                   "Invalid type %d for %p  Data %16llX %16llX",
                   ty, this, debug[0], debug[1]);
        }
        return ty;
    }

    size_t size(runtime &rt = RT) const
    // ------------------------------------------------------------------------
    //  Compute the size of the object by calling the handler with SIZE
    // ------------------------------------------------------------------------
    {
        return (size_t) run(SIZE, rt);
    }

    object_p skip(runtime &rt = RT) const
    // ------------------------------------------------------------------------
    //  Return the pointer to the next object in memory by skipping its size
    // ------------------------------------------------------------------------
    {
        return this + size(rt);
    }

    byte * payload() const
    // ------------------------------------------------------------------------
    //  Return the object's payload, i.e. first byte after ID
    // ------------------------------------------------------------------------
    {
        byte *ptr = (byte *) this;
        leb128(ptr);            // Skip ID
        return ptr;
    }



    // ========================================================================
    //
    //    High-level functions on objects
    //
    // ========================================================================

    void evaluate(runtime &rt = RT) const
    // ------------------------------------------------------------------------
    //  Evaluate an object by calling the handler
    // ------------------------------------------------------------------------
    {
        record(eval, "Evaluating %+s %p", name(), this);
        run(EVAL, rt);
    }

    size_t render(char *output, size_t length, runtime &rt = RT) const;
    // ------------------------------------------------------------------------
    //   Render the object into a buffer
    // ------------------------------------------------------------------------

    cstring render(bool editing = false, runtime &rt = RT) const;
    // ------------------------------------------------------------------------
    //   Render the object into the scratchpad
    // ------------------------------------------------------------------------

    cstring edit(runtime &rt = RT) const;
    // ------------------------------------------------------------------------
    //   Render the object into the scratchpad, then move into the editor
    // ------------------------------------------------------------------------


    static object_p parse(utf8 source, size_t &size, runtime &rt = RT);
    // ------------------------------------------------------------------------
    //  Try parsing the object as a top-level temporary
    // ------------------------------------------------------------------------

    static object_p parse(utf8 source, runtime &rt = RT)
    // ------------------------------------------------------------------------
    //  Try parsing an object without specifying input size
    // ------------------------------------------------------------------------
    {
        size_t size = (size_t) -1;
        return parse(source, size, rt);
    }

    utf8 help(runtime &rt = RT) const
    // ------------------------------------------------------------------------
    //   Return the help topic for the given object
    // ------------------------------------------------------------------------
    {
        return (utf8) run(HELP, rt);
    }

    static cstring name(opcode op)
    // ------------------------------------------------------------------------
    //   Return the name for a given ID
    // ------------------------------------------------------------------------
    {
        return op < NUM_OPCODES ? opcode_name[op] : "<invalid opcode>";
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
        }
        return "<Unknown>";
    }


    static utf8 name(id i)
    // ------------------------------------------------------------------------
    //   Return the name for a given ID
    // ------------------------------------------------------------------------
    {
        return utf8(i < NUM_IDS ? id_name[i] : "<invalid ID>");
    }


    static utf8 fancy(id i)
    // ------------------------------------------------------------------------
    //   Return the fancy name for a given ID
    // ------------------------------------------------------------------------
    {
        return utf8(i < NUM_IDS ? fancy_name[i] : "<Invalid ID>");
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


    // Off-line so that we don't need to import runtime.h
    static void error(utf8 err, utf8 source = nullptr, runtime &rt = RT);
    static void error(cstring err, utf8 source, runtime &rt = RT)
    {
        return error(utf8(err), source, rt);
    }



    // ========================================================================
    //
    //    Attributes of objects
    //
    // ========================================================================

    static bool is_integer(object::id ty)
    // -------------------------------------------------------------------------
    //   Check if a type is an integer
    // -------------------------------------------------------------------------
    {
        return ty >= FIRST_INTEGER_TYPE && ty <= LAST_INTEGER_TYPE;
    }


    bool is_integer(object *obj) const
    // -------------------------------------------------------------------------
    //   Check if an object is an integer
    // -------------------------------------------------------------------------
    {
        return is_integer(obj->type());
    }


    static bool is_decimal(object::id ty)
    // -------------------------------------------------------------------------
    //   Check if a type is a decimal
    // -------------------------------------------------------------------------
    {
        return ty >= FIRST_DECIMAL_TYPE && ty <= LAST_DECIMAL_TYPE;
    }


    bool is_decimal(object *obj) const
    // -------------------------------------------------------------------------
    //   Check if an object is a decimal
    // -------------------------------------------------------------------------
    {
        return is_decimal(obj->type());
    }


    static  bool is_real(object::id ty)
    // -------------------------------------------------------------------------
    //   Check if a type is a real number
    // -------------------------------------------------------------------------
    {
        return ty >= FIRST_REAL_TYPE && ty <= LAST_REAL_TYPE;
    }


    bool is_real(object *obj) const
    // -------------------------------------------------------------------------
    //   Check if an object is a real number
    // -------------------------------------------------------------------------
    {
        return is_real(obj->type());
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



    // ========================================================================
    //
    //    Low-level function dispatch
    //
    // ========================================================================

    static intptr_t run(id       type,
                        opcode   op  = EVAL,
                        runtime &rt  = RT,
                        void    *arg = nullptr)
    // ------------------------------------------------------------------------
    //  Run a command without an object
    // ------------------------------------------------------------------------
    {
        if (type >= NUM_IDS)
        {
            record(object_errors, "Static run op %+s with id %u, max %u",
                   name(op), type, NUM_IDS);
            return -1;
        }
        record(run, "Static run %+s cmd %+s", name(type), name(op));
        return handler[type](rt, op, arg, nullptr, nullptr);
    }

    intptr_t run(opcode op, runtime &rt = RT, void *arg = nullptr) const
    // ------------------------------------------------------------------------
    //  Run an arbitrary command on the object
    // ------------------------------------------------------------------------
    {
        byte *ptr = (byte *) this;
        id type = (id) leb128(ptr); // Don't use type() to update payload
        if (type >= NUM_IDS)
        {
            record(object_errors,
                   "Dynamic run op %+s at %p with id %u, max %u",
                   name(op), this, type, NUM_IDS);
            return -1;
        }
        record(run, "Dynamic run %+s op %+s", name(type), name(op));
        return handler[type](rt, op, arg, this, (object_p ) ptr);
    }

    template <typename Obj>
    static intptr_t run(opcode     op,
                        void      *arg     = nullptr,
                        const Obj *obj     = nullptr,
                        object_p   payload = nullptr,
                        runtime   &rt      = Obj::RT)
    // -------------------------------------------------------------------------
    //   Directly call the object handler for a type (no indirection)
    // -------------------------------------------------------------------------
    {
        record(run, "Direct %+s op %+s", name(Obj::static_type()), name(op));
        return Obj::object_handler(rt, op, arg, obj, payload);
    }

    template <typename Obj>
    static intptr_t run()
    // -------------------------------------------------------------------------
    //   Directly call the object evaluate (no indirection)
    // -------------------------------------------------------------------------
    {
        record(run, "Evaluate %+s", name(Obj::static_type()));
        return Obj::evaluate();
    }

protected:
#define OBJECT_PARSER(type)                                             \
    static result object_parser(parser UNUSED &p, runtime & UNUSED rt = RT)

#define OBJECT_PARSER_BODY(type)                            \
    object::result type::object_parser(parser UNUSED &p, runtime &UNUSED rt)

#define OBJECT_PARSER_ARG()     (*((parser *) arg))

#define OBJECT_RENDERER(type)                           \
    intptr_t object_renderer(renderer &r, runtime &UNUSED rt = RT) const

#define OBJECT_RENDERER_BODY(type)                              \
    intptr_t type::object_renderer(renderer &r, runtime &UNUSED rt) const

#define OBJECT_RENDERER_ARG()   (*((renderer *) arg))

    // The actual work is done here
#define OBJECT_HANDLER_NO_ID(type)                                    \
    static intptr_t object_handler(runtime    &UNUSED rt,             \
                                   opcode      UNUSED op,             \
                                   void       *UNUSED arg,            \
                                   const type *UNUSED obj,            \
                                   object_p    UNUSED payload)

#define OBJECT_HANDLER(type)                            \
    static id static_type() { return ID_##type; }       \
    OBJECT_HANDLER_NO_ID(type)

#define OBJECT_HANDLER_BODY(type)                                       \
    intptr_t type::object_handler(runtime      &UNUSED rt,              \
                                  opcode        UNUSED op,              \
                                  void         *UNUSED arg,             \
                                  const type   *UNUSED obj,             \
                                  object_p      UNUSED payload)

  // The default object handlers
  OBJECT_PARSER(object);
  OBJECT_RENDERER(object);
  OBJECT_HANDLER_NO_ID(object);


#define DELEGATE(base)                                          \
    base::object_handler(rt, op, arg, (base *) obj, payload)

    template <typename T, typename U>
    static intptr_t ptrdiff(T *t, U *u)
    {
        return (byte *) t - (byte *) u;
    }


protected:
    typedef intptr_t (*handler_fn)(runtime &rt,
                                   opcode op, void *arg,
                                   object_p obj, object_p payload);
    static const handler_fn handler[NUM_IDS];
    static const cstring    id_name[NUM_IDS];
    static const cstring    fancy_name[NUM_IDS];
    static const cstring    opcode_name[NUM_OPCODES];
    static runtime         &RT;
};

#endif // OBJECT_H
