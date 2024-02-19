#pragma once

#include <type_traits>
#include "void_cast.h"

template <typename TL> struct _priv_ptrcast { using Type = TL; };

// Valid specializations for combinations of const and non-const.
// Anything not in this list results in compilation failure.

template <typename TL> struct _priv_ptrcast<TL *                       >  { using Type = TL*;						};
template <typename TL> struct _priv_ptrcast<TL  const*                 >  { using Type = TL const*;					};					        	   
template <typename TL> struct _priv_ptrcast<TL **                      >  { using Type = TL**;						};
template <typename TL> struct _priv_ptrcast<TL   const**               >  { using Type = TL const**;				};
template <typename TL> struct _priv_ptrcast<TL * const*                >  { using Type = TL* const*;				};
template <typename TL> struct _priv_ptrcast<TL   const* const*         >  { using Type = TL* const*;				};
template <typename TL> struct _priv_ptrcast<TL ***                     >  { using Type = TL***;						};
template <typename TL> struct _priv_ptrcast<TL    const***             >  { using Type = TL const***;				};
template <typename TL> struct _priv_ptrcast<TL *  const**              >  { using Type = TL* const**;				};
template <typename TL> struct _priv_ptrcast<TL ** const*               >  { using Type = TL** const*;				};
template <typename TL> struct _priv_ptrcast<TL    const*  const**      >  { using Type = TL const* const**;			};
template <typename TL> struct _priv_ptrcast<TL *  const*  const*       >  { using Type = TL* const* const*;			};
template <typename TL> struct _priv_ptrcast<TL    const** const*       >  { using Type = TL const** const*;			};
template <typename TL> struct _priv_ptrcast<TL    const*  const* const*>  { using Type = TL const* const* const*;	};

// Cast a pointer to any other pointer type, preserving strictness about constness and indirection
// ONLY FOR USE WHEN CASTING BETWEEN TRIVIAL TYPES: such as uint8_t* to uint32_t* 
// For all other situations, static_cast<> should be preferred. The purpose here is to provide a better
// alternative to reinterpret_cast<> when such morphing of types is required, such that constness and
// indirection are preserved.
template <typename LV, typename RV> yesinline
constexpr typename _priv_ptrcast<LV>::Type ptr_cast(RV ptr)
{
	static_assert(!std::is_convertible_v<LV,RV>, "Arguments are convertible. Use static_cast<> instead.");
	static_assert(std::is_same<typename _priv_voidcast<LV>::Type, typename _priv_voidcast<RV>::Type>::value, 
		"invalid ptr_cast: changing constness or indirection is not allowed."
	);
	return (typename _priv_ptrcast<LV>::Type)ptr;
}
