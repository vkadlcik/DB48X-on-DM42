// ****************************************************************************
//  datetime.cc                                                   DB48X project
// ****************************************************************************
//
//   File Description:
//
//    Date and time related functions and commands
//
//
//
//
//
//
//
//
// ****************************************************************************
//   (C) 2024 Christophe de Dinechin <christophe@dinechin.org>
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

#include "datetime.h"

#include "renderer.h"
#include "unit.h"
#include "arithmetic.h"



// ============================================================================
//
//   Date and time utilities
//
// ============================================================================

bool to_time(object_p tobj, tm_t &tm)
// ----------------------------------------------------------------------------
//   Convert a HH.MMSS time value to a time
// ----------------------------------------------------------------------------
{
    if (!tobj)
        return false;

    algebraic_g time = nullptr;
    uint scale = 100;
    if (unit_p u = tobj->as<unit>())
        if (object_p uexpr = u->uexpr())
            if (symbol_p sym = uexpr->as_quoted<symbol>())
                if (sym->matches("hms"))
                    time = u->value();
    if (time)
        scale = 60;
    else
        time = tobj->as_real();
    if (!time)
    {
        rt.type_error();
        return false;
    }

    algebraic_g factor = integer::make(scale);
    uint hour = time->as_uint32(false);
    time = (time * factor) % factor;
    uint min = time->as_uint32(false);
    time = (time * factor) % factor;
    uint sec = time->as_uint32(false);
    factor = integer::make(100);
    time = (time * factor) % factor;
    uint csec = time->as_uint32(false);
    if (hour >= 24 || min >= 60 || sec >= 60)
    {
        rt.invalid_time_error();
        return false;
    }
    tm.hour = hour;
    tm.min = min;
    tm.sec = sec;
    tm.csec = csec;

    return true;
}


uint to_date(object_p dtobj, dt_t &dt, tm_t &tm)
// ----------------------------------------------------------------------------
//   Convert a YYYYMMDD.HHMMSS value to a date and optional time
// ----------------------------------------------------------------------------
{
    if (!dtobj)
        return 0;

    algebraic_g date = nullptr;
    if (unit_p u = dtobj->as<unit>())
        if (object_p uexpr = u->uexpr())
            if (symbol_p sym = uexpr->as_quoted<symbol>())
                if (sym->matches("date"))
                    date = u->value();
    if (!date)
        date = dtobj->as_real();
    if (!date)
    {
        rt.type_error();
        return 0;
    }

    algebraic_g factor = integer::make(100);
    algebraic_g time = integer::make(1);
    time = date % time;

    uint d = date->as_uint32(false) % 100;
    date = date / factor;
    uint m = date->as_uint32(false) % 100;
    date = date / factor;
    uint y = date->as_uint32(false);

    const uint days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    bool bisext = m == 2 && y % 4 == 0 && (y % 100 != 0 || y % 400 == 0);
    if (m < 1 || m > 12 || d < 1 || d > days[m-1] + bisext)
    {
        rt.invalid_date_error();
        return 0;
    }

    dt.year  = y;
    dt.month = m;
    dt.day   = d;

    if (time && !time->is_zero())
    {
        time = time * factor;
        uint hour = time->as_uint32(false);
        time = (time * factor) % factor;
        uint min = time->as_uint32(false);
        time = (time * factor) % factor;
        uint sec = time->as_uint32(false);
        time = (time * factor) % factor;
        uint csec = time->as_uint32(false);
        if (hour >= 24 || min >= 60 || sec >= 60)
        {
            rt.invalid_time_error();
            return 0;
        }
        tm.hour = hour;
        tm.min = min;
        tm.sec = sec;
        tm.csec = csec;
        return 2;
    }

    return 1;
}


ularge julian_day_number(const dt_t &dt)
// ----------------------------------------------------------------------------
//   Compute julian day number for a dt_t structure
// ----------------------------------------------------------------------------
{
    return julian_day_number(dt.day, dt.month, dt.year);
}


ularge julian_day_number(algebraic_p dtobj)
// ----------------------------------------------------------------------------
//   Compute the Julian day number associate with a date value
// ----------------------------------------------------------------------------
{
    dt_t dt;
    tm_t tm;
    if (to_date(dtobj, dt, tm))
        return julian_day_number(dt);
    return 0;
}


ularge julian_day_number(int d, int m, int y)
// ----------------------------------------------------------------------------
//   Compute the Julian day number given day, month and year
// ----------------------------------------------------------------------------
{
    uint rm = (m-14)/12;
    ularge jdn = ((1461 * (y + 4800 + rm)) / 4
                  + (367 * (m - 2 - 12 * rm)) / 12
                  - (3 * ((y + 4900 + rm) / 100)) / 4
                  + d
                  - 32075);
    return jdn;
}


COMMAND_BODY(DateTime)
// ----------------------------------------------------------------------------
//   Return current date and time
// ----------------------------------------------------------------------------
{
    dt_t dt;
    tm_t tm;
    rtc_wakeup_delay();
    rtc_read(&tm, &dt);

    ularge tval = tm.hour * 10000 + tm.min * 100 + tm.sec;
    ularge dval = dt.year * 10000 + (dt.month + 1) * 100 + dt.day;
    dval = dval * 1000000ULL + tval;
    if (decimal_g date   = decimal::make(dval, -6))
        if (unit_g result = unit::make(+date, +symbol::make("date")))
            if (rt.push(+result))
                return OK;
    return ERROR;
}


COMMAND_BODY(Date)
// ----------------------------------------------------------------------------
//   Return current date
// ----------------------------------------------------------------------------
{
    dt_t dt;
    tm_t tm;
    rtc_wakeup_delay();
    rtc_read(&tm, &dt);

    ularge dval = dt.year * 10000 + (dt.month + 1) * 100 + dt.day;
    if (integer_g date   = integer::make(dval))
        if (unit_g result = unit::make(+date, +symbol::make("date")))
            if (rt.push(+result))
                return OK;
    return ERROR;
}


static bool setDate(object_p dobj)
// ----------------------------------------------------------------------------
//   Set the system date based on input
// ----------------------------------------------------------------------------
{
    dt_t dt;
    tm_t tm;
    rtc_wakeup_delay();
    rtc_read(&tm, &dt);
    if (!to_date(dobj, dt, tm))
        return false;
    rtc_write(&tm, &dt);
    return true;
}


COMMAND_BODY(SetDate)
// ----------------------------------------------------------------------------
//   Set the current date
// ----------------------------------------------------------------------------
{
    if (rt.args(1))
        if (object_p d = rt.top())
            if (setDate(d))
                if (rt.drop())
                    return OK;

    return ERROR;
}


COMMAND_BODY(Time)
// ----------------------------------------------------------------------------
//   Return the current time
// ----------------------------------------------------------------------------
{
    dt_t dt;
    tm_t tm;
    rtc_wakeup_delay();
    rtc_read(&tm, &dt);

    ularge tval = tm.hour * 10000 + tm.min * 100 + tm.sec;
    if (integer_g itime = integer::make(tval))
      if (integer_g ratio = integer::make(10000))
          if (fraction_g time = fraction::make(itime, ratio))
              if (algebraic_g sexag = from_hms_dms(+time, ""))
                  if (unit_g result = unit::make(+sexag, +symbol::make("hms")))
                      if (rt.push(+result))
                          return OK;
    return ERROR;
}


COMMAND_BODY(ChronoTime)
// ----------------------------------------------------------------------------
//   Return the current time with a precision of 1/100th of a second
// ----------------------------------------------------------------------------
{
    dt_t dt;
    tm_t tm;
    rtc_wakeup_delay();
    rtc_read(&tm, &dt);

    ularge tval = tm.hour * 1000000 + tm.min * 10000 + tm.sec * 100 + tm.csec;
    if (integer_g itime = integer::make(tval))
      if (integer_g ratio = integer::make(1000000))
          if (fraction_g time = fraction::make(itime, ratio))
              if (algebraic_g sexag = from_hms_dms(+time, ""))
                  if (unit_g result = unit::make(+sexag, +symbol::make("hms")))
                      if (rt.push(+result))
                          return OK;
    return ERROR;
}





static bool setTime(object_p tobj)
// ----------------------------------------------------------------------------
//   Set the system date based on input
// ----------------------------------------------------------------------------
{
    dt_t dt;
    tm_t tm;
    rtc_wakeup_delay();
    rtc_read(&tm, &dt);
    if (!to_time(tobj, tm))
        return false;
    rtc_write(&tm, &dt);
    return true;
}


COMMAND_BODY(SetTime)
// ----------------------------------------------------------------------------
//   Set the current time
// ----------------------------------------------------------------------------
{
    if (rt.args(1))
        if (object_p t = rt.top())
            if (setTime(t))
                if (rt.drop())
                    return OK;
    return ERROR;
}



void render_time(renderer &r, algebraic_g &value,
                 cstring hrs, cstring min, cstring sec,
                 uint base, bool ampm)
// ----------------------------------------------------------------------------
//   Render a time (or an angle) as hours/minutes/seconds
// ----------------------------------------------------------------------------
{
    if (!value)
        return;
    bool as_time = *hrs == ':';
    uint h = value->as_uint32(false);
    r.printf("%u", h);
    r.put(hrs);

    algebraic_g one = integer::make(1);
    algebraic_g factor = integer::make(base);
    value = (value * factor) % factor;
    uint m = value ? value->as_uint32(false) : 0;
    r.printf("%02u", m);
    r.put(min);

    value = (value * factor) % factor;
    uint s = value ? value->as_uint32() : 0;
    r.printf("%02u", s);
    r.put(sec);

    value = value % one;
    if (value && !value->is_zero())
    {
        if (as_time && algebraic::to_decimal(value))
        {
            settings::SaveLeadingZero slz(false);
            auto dm = Settings.DisplayMode();
            if (dm == object::ID_Sci || dm == object::ID_Eng)
                dm = object::ID_Fix;
            settings::SaveDisplayMode sdm(dm);
            value->render(r);
        }
        else if (algebraic::decimal_to_fraction(value))
        {
            value->render(r);
        }
    }
    if (ampm)
        r.put(h < 12 ? 'A' : 'P');
}


size_t render_dms(renderer &r, algebraic_g value,
                  cstring deg, cstring min, cstring sec)
// ----------------------------------------------------------------------------
//   Render a number as "degrees / minutes / seconds"
// ----------------------------------------------------------------------------
{
    bool neg = value->is_negative();
    if (neg)
    {
        r.put('-');
        value = -value;
    }
    render_time(r, value, deg, min, sec, 60, false);
    return r.size();
}


size_t render_date(renderer &r, algebraic_g date)
// ----------------------------------------------------------------------------
//   Render a number as "degrees / minutes / seconds"
// ----------------------------------------------------------------------------
{
    if (!date || !date->is_real())
        return 0;
    bool neg = date->is_negative();
    if (neg)
    {
        r.put('-');
        date = -date;
    }

    algebraic_g factor = integer::make(100);
    algebraic_g time = integer::make(1);
    time = date % time;
    uint day = date->as_uint32(false) % 100;
    date = date / factor;
    uint month = date->as_uint32(false) % 100;
    date = date / factor;
    uint year = date->as_uint32(false);

    char mname[4];
    if (Settings.ShowMonthName() && month >=1 && month <= 12)
        snprintf(mname, 4, "%s", get_month_shortcut(month-1));
    else
        snprintf(mname, 4, "%u", month);

    char ytext[6];
    if (Settings.TwoDigitYear())
        snprintf(ytext, 6, "%02u", year % 100);
    else
        snprintf(ytext, 6, "%u", year);

    if (Settings.ShowDayOfWeek())
    {
        ularge jdn = julian_day_number(day, month, year);
        uint dow = (jdn + 1) % 7;
        r.printf("%s ", get_wday_shortcut(dow));
    }

    char sep   = Settings.DateSeparator();
    uint index = 2 * Settings.YearFirst() + Settings.MonthBeforeDay();
    switch(index)
    {
    case 0: r.printf("%u%c%s%c%s", day,   sep, mname, sep, ytext); break;
    case 1: r.printf("%s%c%u%c%s", mname, sep, day,   sep, ytext); break;
    case 2: r.printf("%s%c%u%c%s", ytext, sep, day,   sep, mname); break;
    case 3: r.printf("%s%c%s%c%u", ytext, sep, mname, sep, day);   break;
    }

    if (time && !time->is_zero())
    {
        r.put(", ");
        time = time * factor;
        render_time(r, time, ":", ":", "", 100, Settings.Time12H());
    }

    return r.size();
}



// ============================================================================
//
//   HMS and DMS commands
//
// ============================================================================

algebraic_p to_hms_dms(algebraic_r x)
// ----------------------------------------------------------------------------
//   Convert an algebraic value to HMS or DMS value (i.e. no unit)
// ----------------------------------------------------------------------------
{
    if (unit_p u = x->as<unit>())
    {
        algebraic_g uexpr = u->uexpr();
        if (symbol_p sym = uexpr->as_quoted<symbol>())
        {
            if (sym->matches("dms") || sym->matches("hms"))
                return u->value();
        }
        rt.inconsistent_units_error();
        return nullptr;
    }
    if (!x->is_real())
    {
        rt.type_error();
        return nullptr;
    }
    return x;
}


object::result to_hms_dms(cstring name)
// ----------------------------------------------------------------------------
//   Convert the top of stack to HMS or DMS unit
// ----------------------------------------------------------------------------
{
    if (!rt.args(1))
        return object::ERROR;
    algebraic_g x = algebraic_p(rt.top());
    algebraic_g xc = to_hms_dms(x);
    if (!xc)
        return object::ERROR;

    if (!arithmetic::decimal_to_fraction(xc))
    {
        if (!rt.error())
            rt.value_error();
        return object::ERROR;
    }
    algebraic_g sym = +symbol::make(name);
    unit_g unit = unit::make(xc, sym);
    if (!rt.top(unit))
        return object::ERROR;
    return object::OK;
}


algebraic_p from_hms_dms(algebraic_g x, cstring name)
// ----------------------------------------------------------------------------
//   Convert a value from HMS input
// ----------------------------------------------------------------------------
{
    if (x->is_real())
    {
        // Compatibility mode (including behaviour for 1.60->2.00)
        if (!algebraic::decimal_to_fraction(x))
            return nullptr;
        algebraic_g hours = IntPart::run(x);
        algebraic_g fp = FracPart::run(x);
        algebraic_g hundred = integer::make(100);
        algebraic_g min = hundred * fp;
        algebraic_g sec = hundred * FracPart::run(min);
        min = IntPart::run(min);
        algebraic_g ratio = +fraction::make(integer::make(100),
                                            integer::make(6000));
        sec = sec * ratio;
        min = (min + sec) * ratio;
        hours = hours + min;
        return hours;
    }
    else if (unit_g u = x->as<unit>())
    {
        algebraic_g uexpr = u->uexpr();
        if (symbol_p sym = uexpr->as_quoted<symbol>())
        {
            if (sym->matches(name))
            {
                uexpr = u->value();
                return uexpr;
            }
        }
        rt.inconsistent_units_error();
    }
    else
    {
        rt.type_error();
    }
    return nullptr;
}


object::result from_hms_dms(cstring name)
// ----------------------------------------------------------------------------
//   Convert the top of stack from HMS or DMS unit
// ----------------------------------------------------------------------------
{
    if (!rt.args(1))
        return object::ERROR;
    algebraic_g x = algebraic_p(rt.top());
    x = from_hms_dms(x, name);
    if (x && rt.top(x))
        return object::OK;
    return object::ERROR;
}


COMMAND_BODY(ToHMS)
// ----------------------------------------------------------------------------
//   Convert value to HMS format
// ----------------------------------------------------------------------------
{
    return to_hms_dms("hms");
}


COMMAND_BODY(ToDMS)
// ----------------------------------------------------------------------------
//   Convert value to DMS format
// ----------------------------------------------------------------------------
{
    return to_hms_dms("dms");
}


COMMAND_BODY(FromHMS)
// ----------------------------------------------------------------------------
//   Convert value from HMS format
// ----------------------------------------------------------------------------
{
    return from_hms_dms("hms");
}


COMMAND_BODY(FromDMS)
// ----------------------------------------------------------------------------
//   Convert value from DMS format
// ----------------------------------------------------------------------------
{
    return from_hms_dms("dms");
}


static object::result hms_dms_add_sub(cstring name, bool sub)
// ----------------------------------------------------------------------------
//   Addition or subtraction of DMS/HMS values
// ----------------------------------------------------------------------------
{
    if (!rt.args(2))
        return object::ERROR;

    algebraic_g x = algebraic_p(rt.stack(0));
    algebraic_g y = algebraic_p(rt.stack(1));

    // Convert both arguments to DMS
    x = from_hms_dms(x, name);
    y = from_hms_dms(y, name);
    if (!x || !y)
        return object::ERROR;

    // Add or subtract
    x = sub ? y - x : y + x;

    // Build result
    algebraic_g sym = +symbol::make(name);
    unit_g unit = unit::make(x, sym);
    if (!rt.drop() || !rt.top(unit))
        return object::ERROR;
    return object::OK;
}


COMMAND_BODY(DMSAdd)
// ----------------------------------------------------------------------------
//   Addition of DMS values
// ----------------------------------------------------------------------------
{
    return hms_dms_add_sub("dms", false);
}


COMMAND_BODY(DMSSub)
// ----------------------------------------------------------------------------
//   Subtraction of DMS values
// ----------------------------------------------------------------------------
{
    return hms_dms_add_sub("dms", true);
}


COMMAND_BODY(HMSAdd)
// ----------------------------------------------------------------------------
//   Addition of HMS values
// ----------------------------------------------------------------------------
{
    return hms_dms_add_sub("hms", false);
}


COMMAND_BODY(HMSSub)
// ----------------------------------------------------------------------------
//   Subtraction of HMS values
// ----------------------------------------------------------------------------
{
    return hms_dms_add_sub("hms", true);
}


COMMAND_BODY(DateAdd)
// ----------------------------------------------------------------------------
//   Add a date to a number of days
// ----------------------------------------------------------------------------
{
    if (!rt.args(2))
        return ERROR;

    return ERROR;
}


COMMAND_BODY(DateSub)
// ----------------------------------------------------------------------------
//   Compute the number of days between two dates
// ----------------------------------------------------------------------------
{
    if (!rt.args(2))
        return ERROR;

    dt_t dt1, dt2;
    tm_t tm1, tm2;
    if (object_p d1 = rt.stack(1))
    {
        if (object_p d2 = rt.stack(0))
        {
            if (to_date(d1, dt1, tm1) && to_date(d2, dt2, tm2))
            {
                ularge diff = julian_day_number(dt1) - julian_day_number(dt2);

                if (integer_p d = integer::make(diff))
                    if (rt.drop() && rt.top(d))
                        return OK;
            }
        }
    }
    return ERROR;
}


COMMAND_BODY(JulianDayNumber)
// ----------------------------------------------------------------------------
//   Return the Julian day number for current date and time
// ----------------------------------------------------------------------------
{
    if (!rt.args(1))
        return ERROR;
    dt_t dt;
    tm_t tm{};
    if (object_p d = rt.top())
    {
        if (to_date(d, dt, tm))
        {
            ularge jdn = julian_day_number(dt);
            ularge jf = (3600 * tm.hour + 60 * tm.min + tm.sec) * 100 + tm.csec;
            ularge ratio = 8640000;
            algebraic_g jdna = integer::make(jdn);
            algebraic_g jdnf = +fraction::make(integer::make(jf),
                                               integer::make(ratio));
            jdna = jdna + jdnf;
            if (jdna && rt.top(jdna))
                return OK;
        }
    }
    return ERROR;
}
