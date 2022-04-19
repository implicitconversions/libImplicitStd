#pragma once


template <typename T> struct _priv_voidcast { using Type = void; };

// Valid specializations for combinations of const and non-const.
// Anything not in this list results in compilation failure.

template <typename T> struct _priv_voidcast<T*>                        { using Type = void*;						};
template <typename T> struct _priv_voidcast<T const*>                  { using Type = void const*;					};
														        	   
template <typename T> struct _priv_voidcast<T**>                       { using Type = void**;						};
template <typename T> struct _priv_voidcast<T  const**>                { using Type = void const**;					};
template <typename T> struct _priv_voidcast<T* const*>                 { using Type = void* const*;					};
template <typename T> struct _priv_voidcast<T  const* const*>          { using Type = void* const*;					};
																	   
template <typename T> struct _priv_voidcast<T***>                      { using Type = void***;						};
template <typename T> struct _priv_voidcast<T   const***>              { using Type = void const***;				};
template <typename T> struct _priv_voidcast<T*  const**>               { using Type = void* const**;				};
template <typename T> struct _priv_voidcast<T** const*>                { using Type = void** const*;				};
template <typename T> struct _priv_voidcast<T   const*  const**>       { using Type = void const* const**;			};
template <typename T> struct _priv_voidcast<T*  const*  const*>        { using Type = void* const* const*;			};
template <typename T> struct _priv_voidcast<T   const** const*>        { using Type = void const** const*;			};
template <typename T> struct _priv_voidcast<T   const*  const* const*> { using Type = void const* const* const*;	};

// Cast a pointer to a void pointer, preserving strictness about constness and indirection
template <typename T> __nodebug
constexpr typename _priv_voidcast<T>::Type void_cast(T ptr)
{
	return (typename _priv_voidcast<T>::Type)ptr;
}
