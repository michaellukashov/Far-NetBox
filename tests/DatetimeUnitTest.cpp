#define NOMINMAX
#include <Sysutils.hpp>
#include <iostream>
#include <cassert>

int main()
{
    // Test FormatDateTime: year formatting
    TDateTime dt = EncodeDate(2025, 4, 23);
    UnicodeString result = FormatDateTime(L"yyyy", dt);
    assert(result == L"2025");

    // Two-digit year
    result = FormatDateTime(L"yy", dt);
    assert(result == L"25");

    // Month
    result = FormatDateTime(L"mm", dt);
    assert(result == L"04");

    // Day
    result = FormatDateTime(L"dd", dt);
    assert(result == L"23");

    // Combined date
    result = FormatDateTime(L"yyyy-mm-dd", dt);
    assert(result == L"2025-04-23");

    // Time formatting
    TDateTime dt2 = EncodeTime(12, 34, 56, 789);
    result = FormatDateTime(L"hh:nn:ss", dt2);
    assert(result == L"12:34:56");

    // Milliseconds
    result = FormatDateTime(L"zzz", dt2);
    assert(result == L"789");

    // Combined date and time
    TDateTime dt3 = EncodeDate(2025, 4, 23) + EncodeTime(14, 30, 0, 0);
    result = FormatDateTime(L"yyyy-mm-dd hh:nn:ss", dt3);
    assert(result == L"2025-04-23 14:30:00");

    // Test ISO8601ToDate: date only
    TDateTime dt_iso1 = ISO8601ToDate(L"2025-04-23");
    assert(dt_iso1.GetValue() == EncodeDate(2025, 4, 23).GetValue());

    // Date and time
    TDateTime dt_iso2 = ISO8601ToDate(L"2025-04-23T14:30:00");
    double expected = EncodeDate(2025, 4, 23).GetValue() + EncodeTime(14, 30, 0, 0).GetValue();
    assert(dt_iso2.GetValue() == expected);

    // Invalid format should throw
    bool thrown = false;
    try {
        ISO8601ToDate(L"invalid");
    } catch (const Exception&) {
        thrown = true;
    }
    assert(thrown);

    std::wcout << L"All datetime tests passed." << std::endl;
    return 0;
}
