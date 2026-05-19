#include <vcl.h>
#include <Classes.hpp>
#include "FormatUtils.h"


namespace nb {

UnicodeString Format(const UnicodeString & fmt, fmt::ArgList args)
{
  fmt::WMemoryWriter w;
  w.write(fmt.data(), args);
  try
  {
    return UnicodeString(w.data(), nb::ToInt32(w.size()));
  }
  catch(const fmt::FormatError & Ex)
  {
    nb::used(Ex);
    DEBUG_PRINTF("Error: %s", UnicodeString(Ex.what()));
    DebugAssert(false);
  }
  return UnicodeString();
}

UnicodeString Sprintf(const UnicodeString & fmt, fmt::ArgList args)
{
  fmt::WMemoryWriter w;
  try
  {
    fmt::printf(w, fmt.data(), args);
    return UnicodeString(w.data(), nb::ToInt32(w.size()));
  }
  catch(const fmt::FormatError & Ex)
  {
    nb::used(Ex);
    DEBUG_PRINTF("Error: %s", UnicodeString(Ex.what()));
    DebugAssert(false);
  }
  return UnicodeString();
}

UnicodeString FmtLoadStr(int32_t id, fmt::ArgList args)
{
  Expects(GetGlobals() != nullptr);
  const UnicodeString Fmt = GetGlobals()->GetMsg(id);
  if (!Fmt.IsEmpty())
  {
    const UnicodeString Result = Sprintf(Fmt, args);
    return Result;
  }
  DEBUG_PRINTF("Unknown resource string id: %d\n", id);
  return UnicodeString();
}

UnicodeString EscapeFmtChars(const UnicodeString & Str)
{
  UnicodeString Result;
  for (int32_t I = 1; I <= Str.GetLength(); ++I)
  {
    if (Str[I] == '%')
    {
      Result += '%';
      Result += '%';
    }
    else
    {
      Result += Str[I];
    }
  }
  return Result;
}

UnicodeString DateTimeToStr(const TDateTime & DateTime)
{

  // TDateTime represents days since 12/30/1899
  // Convert to year, month, day, hour, minute, second
  // Extract date components
  int32_t Days = static_cast<int32_t>(DateTime);
  int32_t Year, Month, Day;
  // Convert days to year/month/day (based on Delphi's DateUtils)
  // 12/30/1899 is day 0
  // This is a simplified implementation based on known date arithmetic
  // Base date: 12/30/1899 (day 0)
  const int32_t BaseYear = 1899;
  const int32_t BaseMonth = 12;
  const int32_t BaseDay = 30;
  // Handle negative dates
  if (Days < 0)
  {
    // For negative dates, we need to subtract from base date
    // This is a simplified approach - we'll handle it by converting to Julian day number
    // and then back to Gregorian
    // We'll use a known algorithm for Gregorian date calculation
    // Convert to Julian day number
    // Julian day number for 12/30/1899 is 2415020
    int32_t JulianDay = 2415020 + Days;
    // Convert Julian day to Gregorian date
    // Algorithm from https://en.wikipedia.org/wiki/Julian_day#Converting_Julian_day_number_to_Gregorian_calendar_date
    int32_t a = JulianDay + 32044;
    int32_t b = (4 * a + 3) / 146097;
    int32_t c = a - (146097 * b) / 4;
    int32_t d = (4 * c + 3) / 1461;
    int32_t e = c - (1461 * d) / 4;
    int32_t m = (5 * e + 2) / 153;
    Day = e - (153 * m + 2) / 5 + 1;
    Month = m + 3 - 12 * (m / 10);
    Year = 100 * b + d - 4800 + m / 10;
  }
  else
  {
    // For non-negative dates, we can use a simpler approach
    // Start from base date 12/30/1899
    Year = BaseYear;
    Month = BaseMonth;
    Day = BaseDay;
    // Add days
    while (Days > 0)
    {
      int32_t DaysInMonth;
      if (Month == 2)
      {
        // Check for leap year
        bool IsLeap = (Year % 4 == 0 && Year % 100 != 0) || (Year % 400 == 0);
        DaysInMonth = IsLeap ? 29 : 28;
      }
      else if (Month == 4 || Month == 6 || Month == 9 || Month == 11)
      {
        DaysInMonth = 30;
      }
      else
      {
        DaysInMonth = 31;
      }
      if (Days >= DaysInMonth - Day + 1)
      {
        Days -= DaysInMonth - Day + 1;
        Day = 1;
        Month++;
        if (Month > 12)
        {
          Month = 1;
          Year++;
        }
      }
      else
      {
        Day += Days;
        Days = 0;
      }
    }
  }
  // Extract time components (fractional part of day)
  double FractionalDay = DateTime - static_cast<double>(Days);
  int32_t Hours = static_cast<int32_t>(FractionalDay * 24);
  double RemainingHours = FractionalDay * 24 - Hours;

  int32_t Minutes = static_cast<int32_t>(RemainingHours * 60);
  double RemainingMinutes = RemainingHours * 60 - Minutes;
  int32_t Seconds = static_cast<int32_t>(RemainingMinutes * 60);
  // Format as YYYY-MM-DD HH:MM:SS
  return nb::Format(L"{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}",
                     Year, Month, Day, Hours, Minutes, Seconds);

}

} // namespace nb
