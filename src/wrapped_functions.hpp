#ifndef DCGP_WRAPPED_FUNCTIONS_H
#define DCGP_WRAPPED_FUNCTIONS_H

#include <string>
#include <vector>
#include <audi/gdual.hpp>

namespace dcgp {

// SFINAE dust
template <typename T>
using f_enabler = typename std::enable_if<std::is_same<T,double>::value || std::is_same<T,audi::gdual>::value, int>::type;


/*--------------------------------------------------------------------------
*                                  BINARY FUNCTIONS
*------------------------------------------------------------------------**/
template <typename T, f_enabler<T> = 0>
T my_sum(const T &x, const T &y)
{
        return x + y;
}

std::string print_my_sum(const std::string& s1, const std::string& s2)
{
    if (s1 == s2) 
    {
        return "(2*"+s1+")";
    }
    else if (s1 == "0")
    {
        return s2;
    }
    else if (s2 == "0")
    {
        return s1;
    }
    return ("(" + s1 + "+" + s2 + ")");
}

template <typename T, f_enabler<T> = 0>
T my_diff(const T &x, const T &y)
{
        return x - y;
}

std::string print_my_diff(const std::string& s1, const std::string& s2)
{
    if (s1 == s2) 
    {
        return "0";
    }
    else if (s1 == "0")
    {
        return "(-" + s2 + ")";
    }
    else if (s2 == "0")
    {
        return s1;
    }
    return ("(" + s1 + "-" + s2 + ")");
}

template <typename T, f_enabler<T> = 0>
T my_mul(const T &x, const T &y)
{
        return (x * y);
}

std::string print_my_mul(const std::string& s1, const std::string& s2)
{
    if (s1 == "0" || s2 == "0")
    {
        return "0";
    }
    else if (s1 == s2)
    {
        return s1 + "^2";
    }
    else if (s1 == "1")
    {
        return s2;
    }
    else if (s2 == "1")
    {
        return s1;
    }
    return ("(" + s1 + "*" + s2 + ")");
}

template <typename T, f_enabler<T> = 0>
T my_div(const T &x, const T &y)
{
        return x / y;
}

std::string print_my_div(const std::string& s1, const std::string& s2)
{
    if (s1 == "0" && s2 != "0")
    {
        return "0";
    }
    else if (s1 == s2)
    {
        return "1";
    }
    return ("(" + s1 + "/" + s2 + ")");
}



/*--------------------------------------------------------------------------
*                                  UNARY FUNCTIONS
*------------------------------------------------------------------------**/


} // dcgp namespace ends

#endif // DCGP_WRAPPED_FUNCTIONS_H