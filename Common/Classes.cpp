#pragma once

#include "stdafx.h"
#include "Classes.h"
#include "FarUtil.h"

//---------------------------------------------------------------------------

void TDateTime::DecodeDate(unsigned short &Y,
        unsigned short &M, unsigned short &D)
{
    ::DecodeDate(*this, Y, M, D);
}
void TDateTime::DecodeTime(unsigned short &H,
        unsigned short &N, unsigned short &S, unsigned short &MS)
{
    ::DecodeTime(*this, H, N, S, MS);
}

//---------------------------------------------------------------------------

void TStream::ReadBuffer(void *Buffer, unsigned long int Count)
{
}

unsigned long TStream::Read(void *Buffer, unsigned long int Count)
{
    return 0;
}

void TStream::WriteBuffer(void *Buffer, unsigned long int Count)
{
}

unsigned long TStream::Write(void *Buffer, unsigned long int Count)
{
    return 0;
}

