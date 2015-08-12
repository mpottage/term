#include <string>
namespace utf8 {

inline int offset_next(char ch) //ch is the start of a code point.
{
    auto c = static_cast<unsigned char>(ch);
    //Set p125 of Unicode 7.0 standard, table 3-7.
    if(c<=0x7F)
        return 1;
    else if(c>=0xC2 and c<=0xDF)
        return 2;
    else if(c>=0xE0 and c<=0xEF)
        return 3;
    else if(c>=0xF0 and c<=0xF4)
        return 4;
    else
        throw std::runtime_error{"Badly formed UTF-8 sequence."};
}

//Returns number of code points in a UTF-8 string.
inline size_t size(const std::string& s)
{
    size_t size=0;
    for(int i=0; i<s.size(); ++size)
        i += offset_next(s[i]);
    return size;
}

//Indexing a UTF-8 string.
inline std::string at(const std::string& s, int index)
{
    int j=0;
    int code_point=0;
    for(; j<s.size() and code_point!=index; ++code_point)
        j += offset_next(s[j]);
    if(code_point!=index or j>s.size())
        throw std::out_of_range{"Non-existent code point in UTF-8 string."};
    auto c = static_cast<unsigned char>(s[j]);
    if(c>=0xF0)
        return s.substr(j,4);
    else if(c>=0xE0)
        return s.substr(j,3);
    else if(c>=0xC2)
        return s.substr(j,2);
    else
        return s.substr(j,1);
}

}
