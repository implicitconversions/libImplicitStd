#pragma once

#include <type_traits>

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

template <typename T> __nodebug
constexpr intptr_t intptr_cast(T ptr)
{
	return (intptr_t)(typename _priv_voidcast<T>::Type)ptr;
}

template <typename T> __nodebug
constexpr uintptr_t uintptr_cast(T ptr)
{
	return (uintptr_t)(typename _priv_voidcast<T>::Type)ptr;
}
