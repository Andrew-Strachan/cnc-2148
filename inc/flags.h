
class CFlags<T> : T
{
    static inline T operator|(T a, T b)
    {return static_cast<T>(static_cast<int>(a) | static_cast<int>(b));}
    static inline T operator&(T a, T b)
    {return static_cast<T>(static_cast<int>(a) & static_cast<int>(b));}
    static inline T operator&=(T a, T b)
    { a = a & b; return a; }
    static inline T operator~(T a)
    {return static_cast<T>(~static_cast<int>(a));}
    static inline T operator|=(T a, T b)
    { a = a | b; return a; }
    static inline T operator~=(T a)
    { a = ~a; return a; }
    static inline T operator^(T a, T b)
    {return static_cast<T>(static_cast<int>(a) ^ static_cast<int>(b));}
    static inline T operator^=(T a, T b)
    { a = a ^ b; return a; }
}