
template < enum T> class CFlags
{

};

template <typename T> static inline T operator|(T a, T b)
    {return static_cast<T>(static_cast<int>(a) | static_cast<int>(b));}
template <typename T>    static inline T operator&(T a, T b)
    {return static_cast<T>(static_cast<int>(a) & static_cast<int>(b));}
template <typename T>    static inline T operator&=(T a, T b)
    { a = a & b; return a; }
template <typename T>    static inline T operator~(T a)
    {return static_cast<T>(~static_cast<int>(a));}
template <typename T>    static inline T operator|=(T a, T b)
    { a = a | b; return a; }

template <typename T>    static inline T operator^(T a, T b)
    {return static_cast<T>(static_cast<int>(a) ^ static_cast<int>(b));}
template <typename T>    static inline T operator^=(T a, T b)
    { a = a ^ b; return a; }