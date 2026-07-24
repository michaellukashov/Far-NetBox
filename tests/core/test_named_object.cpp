#define NOMINMAX
#include <iostream>
#include <cassert>

#include <NamedObjs.h>

// Regression test for empty session name causing "Index is out of range" crash
// during import of malformed .netbox XML files.
// https://github.com/michaellukashov/Far-NetBox/issues/???

int main()
{
    // --- Empty name must not crash ---
    // This path is exercised when a .netbox file contains a <Session>
    // element with a missing or empty name attribute.
    {
        TNamedObject obj(OBJECT_CLASS_TSessionData, L"");
        assert(!obj.GetHidden());
        assert(obj.GetName().IsEmpty());
        std::cout << "PASS: empty name handled safely" << std::endl;
    }

    // --- Normal hidden prefix still works ---
    {
        TNamedObject obj(OBJECT_CLASS_TSessionData, L"_!_HiddenSession");
        assert(obj.GetHidden());
        assert(obj.GetName() == L"_!_HiddenSession");
        std::cout << "PASS: hidden prefix detected" << std::endl;
    }

    // --- Non-hidden name ---
    {
        TNamedObject obj(OBJECT_CLASS_TSessionData, L"NormalSession");
        assert(!obj.GetHidden());
        assert(obj.GetName() == L"NormalSession");
        std::cout << "PASS: non-hidden name preserved" << std::endl;
    }

    // --- Single-character name (edge case near prefix length) ---
    {
        TNamedObject obj(OBJECT_CLASS_TSessionData, L"A");
        assert(!obj.GetHidden());
        assert(obj.GetName() == L"A");
        std::cout << "PASS: short name handled" << std::endl;
    }

    // --- Name shorter than hidden prefix but starting with underscore ---
    {
        TNamedObject obj(OBJECT_CLASS_TSessionData, L"_!");
        assert(!obj.GetHidden());
        assert(obj.GetName() == L"_!");
        std::cout << "PASS: partial prefix not misdetected" << std::endl;
    }

    std::cout << "All tests passed." << std::endl;
    return 0;
}
