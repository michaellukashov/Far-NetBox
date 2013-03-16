/**
 by Danushka Abeysuriya aka. 'silvermace' 
 Contact: silvermace@gmail.com
 Last Edit 18 May 2010
*/

#ifndef RDESTL_STRINGSTREAM_H
#define RDESTL_STRINGSTREAM_H

#include "rdestl_common.h"
#include "rde_string.h"
#include "vector.h"

namespace rde 
{
    /*
     this is a super cut down replacement for std::stringstream
     current limitations:
     - only converts from string -> primitve type
     - only supports signed int, signed long, float and rde::string
     - no support for unsigned word types
     - untested unicode support, no support for custom allocator
     */
    template< typename E, typename TAlloc = rde::allocator >
    struct basic_stringstream
    {
        typedef E                                   value_type;
        typedef rde::vector<value_type, TAlloc>     buffer_type;
        typedef typename buffer_type::size_type		size_type;
		typedef basic_string<value_type, TAlloc>	string_type;
        
        explicit basic_stringstream(const value_type* inp) { init(inp); }
        explicit basic_stringstream(const string_type& inp) { init(inp.c_str()); }
        basic_stringstream() {}
 
        bool good() const { return buffer.size() ? cursor != buffer.end() : false; }
        bool eof() const { return !good(); }
        operator bool() const { return good(); }
        
        void reset(const value_type* inp) {
            init(inp);
        }
        
        //------------------------------------------------------
        //Output operators
        basic_stringstream& operator>>(int& x) {
            if( next() ) 
                x = atoi((const char*)current.c_str());
            return *this;
        }
        basic_stringstream& operator>>(long& x) {
            if( next() ) 
                x = atol((const char*)current.c_str());
            return *this;
        }
        basic_stringstream& operator>>(float& x) {
            if( next() ) 
                x = atof((const char*)current.c_str());
            return *this;
        }
        basic_stringstream& operator>>(rde::string& x) {
            if( next() ) 
                x = current;
            return *this;
        }
        //------------------------------------------------------
        
    private:
        //Setup our data buffer and cursor
        void init(const value_type* inp) 
        {
            if( !inp || !strlen(inp) ) {
                cursor = buffer.end();
                return;
            }
            const size_type len = strlen(inp);
            buffer.resize(len);
            memmove(buffer.begin(), inp, len);
            cursor = buffer.begin();
            current.clear();
            ltrim();
        }
        
        bool is_whitespace(const value_type& ch) const {
            return (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n');
        }
        
        //Trim whitespace from the left->right
        void ltrim() {
            while(is_whitespace(*cursor)) 
                ++cursor;
        }
        
        //Read in the next token into current
        bool next() 
        {
            if(!buffer.size())
                return false;
            current.clear();
            for(; cursor!=buffer.end(); ++cursor) {
                if(!is_whitespace(*cursor)) {
                    current.append(*cursor);   
                } else {
                    ltrim();
                    break;
                }
            }
            return current.length();
        }
        
        //Data members
        string_type current;
        buffer_type buffer;
        typename buffer_type::const_iterator cursor;
    };    
    
    typedef basic_stringstream<char> stringstream;
}


#endif
