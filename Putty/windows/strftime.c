/***
*strftime.c - String Format Time
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*
*******************************************************************************/

#include <windows.h>

#include <io.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
/*
static int debug_printf(const char *format, ...)
{
    int len = 0;
#ifdef NETBOX_DEBUG
    va_list args;
    va_start(args, format);
    len = sprintf(format, args);
    char buf(len + sizeof(char), 0);
    snprintfs(buf, sizeof(buf, format, args);
    va_end(args);
    OutputDebugString(buf);
#endif
    return len;
}
*/

#ifdef NETBOX_DEBUG
#define DEBUG_PRINTF(format, ...) debug_printf(format, __VA_ARGS__);
#else
#define DEBUG_PRINTF(format, ...)
#endif

size_t strftime2( char *s, size_t maxsize, const char *fmt, const struct tm *t )
{
    char buf[ 255 ];

    char *p = buf;
    char pad;
    size_t  i, len;
    char FormatBuff[128];
    OutputDebugString("NetBox: 1");

    for ( len = 1; len < maxsize && *fmt; ++fmt, p = buf )
    {

        if ( *fmt != '%' )
        {
            ++len;
            *s++ = *fmt;
        }

        else
        {
            strcpy( buf, "00" );

the_switch:
            OutputDebugString("NetBox: 2");
            switch( *++fmt )
            {

            /* abbreviated day */
            // case 'a':
                // if (t->tm_wday == 0)
                    // p = (char *) _getLocaleTimeInfo(LOCALE_SABBREVDAYNAME7);
                // else
                    // p = (char *) _getLocaleTimeInfo(LOCALE_SABBREVDAYNAME1 + t->tm_wday - 1);
                // break;

            /* full day */
            // case 'A':
                // if (t->tm_wday == 0)
                    // p = (char *) _getLocaleTimeInfo(LOCALE_SDAYNAME7);
                // else
                    // p = (char *) _getLocaleTimeInfo(LOCALE_SDAYNAME1 + t->tm_wday - 1);
                // break;

            /* abbreviated month */
            // case 'b':
            // case 'h':   /* POSIX  */
                // p = (char *) _getLocaleTimeInfo(LOCALE_SABBREVMONTHNAME1 + t->tm_mon);
                // break;

            // /* full month */
            // case 'B':
                // p = (char *) _getLocaleTimeInfo(LOCALE_SMONTHNAME1 + t->tm_mon);
                // break;

            /* date and time */
            case 'c':
                /* set p to current s position */
                p = s;

                /* recurse */
                strftime2( p, maxsize, "%x %X", t );
                break;

            /* month date */
            case 'd':
                // __utoa( t->tm_mday, buf + (t->tm_mday < 10));
                _itoa_s( t->tm_mday, buf + (t->tm_mday < 10), maxsize, 10 );
                break;

            /*
            case 'g':
                strcpy(p, "");
                _getLocaleEra(buf, sizeof(buf), t);
                break;
            */

            /* hour (24) */
            case 'H':
                // __utoa( t->tm_hour, buf + (t->tm_hour < 10));
                _itoa_s( t->tm_hour, buf + (t->tm_hour < 10), maxsize, 10 );
                break;

            /* hour (12) */
            case 'I':
                i = t->tm_hour % 12;

                if (i == 0)
                    i = 12;

                // __utoa( i, buf + (i < 10) );
                _itoa_s( i, buf + (i < 10), maxsize, 10 );
                break;

            /* year day */
            case 'j':
                i = t->tm_yday + 1;

                // __utoa( i, buf + (i < 10) + (i < 100) );
                _itoa_s( i, buf + (i < 10) + (i < 100), maxsize, 10 );
                break;

            /* month */
            case 'm':
                i = t->tm_mon + 1;

                // __utoa( i, buf + (i < 10) );
                _itoa_s( i, buf + (i < 10), maxsize, 10 );
                break;

            /* minute */
            case 'M':
                // __utoa( t->tm_min, buf + (t->tm_min < 10) );
                _itoa_s( t->tm_min, buf + (t->tm_min < 10), maxsize, 10 );
                break;

            /* am/pm */
            /*
            case 'p':
                if (t->tm_hour / 12 > 0)
                    p = (char *) _getLocaleTimeInfo(LOCALE_S2359);
                else
                    p = (char *) _getLocaleTimeInfo(LOCALE_S1159);

                break;
            */
            /* seconds */
            case 'S':
                // __utoa( t->tm_sec, buf + (t->tm_sec < 10) );
                _itoa_s( t->tm_sec, buf + (t->tm_sec < 10), maxsize, 10 );
                break;

            /* week of year (Sunday) */
            case 'U':
                // Adjust weekday to be positive (first week) Sunday
                // based
                i = (t->tm_wday + 7) % 7;

                // Adjust week to be positive (first week)
                i = (t->tm_yday - i + 7) / 7;

                // __utoa( i, buf + (i < 10) );
                _itoa_s( i, buf + (i < 10), maxsize, 10 );
                break;

            /* day of week */
            case 'w':
                // __utoa( t->tm_wday, buf );
                _itoa_s( t->tm_wday, buf, maxsize, 10 );

                break;

            /* week of year (Monday) */
            case 'W':
                // Adjust weekday to be positive (first week) Monday
                // based
                i = (t->tm_wday + 6) % 7;

                // Adjust week to be positive (first week)
                i = (t->tm_yday - i + 7) / 7;

                // __utoa( i, buf + (i < 10) );
                _itoa_s( i, buf + (i < 10), maxsize, 10 );
                break;

            /* date */
            // case 'x':
            // case 'D':       /* POSIX */
                // /* set p to current s position */
                // p = s;

                // /* recurse */
                // strftime2( p, maxsize, (char *) _getLocaleTimeInfo(LOCALE_SSHORTDATE), t );

                // break;
            /* time */
        case 'r':
            /* set p to current s position */
            p = s;

        /* recurse */
            strftime2( p, maxsize, "%I:%M:%S %p", t);

            break;

        case 'R':
            /*set p to current s position */
            p = s;

            /* recurse */
            strftime2(p, maxsize, "%H:%M", t);

            break;

        /* POSIX */
        case 'T':

            /* set p to current s position */
            p = s;

            /* recurse */
            strftime2(p, maxsize, "%H:%M:%S", t);

            break;

            // case 'X':
                // /* set p to current s position */
                // p = s;

                // /* recurse */
                // strftime2( p, maxsize, (char *) _getLocaleTimeInfo(LOCALE_STIMEFORMAT), t );

                // break;

            /* year (no century) */
            case 'y':
                i = t->tm_year % 100;

                // __utoa( i, buf + (i < 10) );
                _itoa_s( i, buf + (i < 10), maxsize, 10 );
                break;

            /* year (with century) */
            case 'Y':
                OutputDebugString("NetBox: 2_1");
                // __utoa( 1900 + t->tm_year, buf );
                _itoa_s( 1900 + t->tm_year, buf, maxsize, 10 );
                OutputDebugString("NetBox: 2_2");
                break;

            /* century */
            case 'C':       /* POSIX */
                // __utoa( t->tm_year/100 + 19, buf);
                _itoa_s( t->tm_year/100 + 19, buf, maxsize, 10 );
                break;

            case '+' :
                pad = *++fmt;
                buf[0] = pad;
                buf[1] = pad;
                goto the_switch;


            /* time zone */
            case 'z':
            case 'Z':
               p = _tzname[ t->tm_isdst ];
               break;

            /* literally "%" */
            case '%':
                p = "%";
                break;

            case '#' :
                switch( *++fmt )
                {

                // case  'c' :
                    // strcpy(FormatBuff, (char *) _getLocaleTimeInfo(LOCALE_SLONGDATE));
                    // strcat ( FormatBuff, " %X");
                    // strftime2( p, maxsize, FormatBuff, t );
                    // break;

                // case 'x' :
                    // strftime2( p, maxsize, (char *) _getLocaleTimeInfo(LOCALE_SLONGDATE), t );
                    // break;

                default :
                    break;

                }
                break;
            }
            OutputDebugString("NetBox: 3");

            i = min( strlen( p ), maxsize - len );
            strncpy( s, p, i );
            len += i;
            s += i;

        } /* if-else */

        OutputDebugString("NetBox: 4");
    } /* for */

    *s = '\0';
    OutputDebugString("NetBox: 5");

    if( *fmt )
        return( 0 );
    else
        return( len - 1 );
}
