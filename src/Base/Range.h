//-----------------------------------------------------------------------------------------------// 
// Range.h
//-----------------------------------------------------------------------------------------------// 
#ifndef MPX_BASE_RANGE_H
#define MPX_BASE_RANGE_H

#include <Include.h>

namespace mpx {

//-----------------------------------------------------------------------------------------------// 

template<typename T>
struct Range
{
	T begin;
	T end;
};

using RangeU64 = Range<uint64_t>;
using RangeI64 = Range<int64_t>;
using RangeU32 = Range<uint32_t>;
using RangeI32 = Range<int32_t>;
using RangeU = Range<uint>;
using RangeI = Range<int>;
using RangeF = Range<float>;
using RangeD = Range<double>;

//-----------------------------------------------------------------------------------------------// 

template<typename T>
struct Block
{
	T begin;
	T size;
};

using BlockU64 = Block<uint64_t>;
using BlockI64 = Block<int64_t>;
using BlockU32 = Block<uint32_t>;
using BlockI32 = Block<int32_t>;
using BlockU = Block<uint>;
using BlockI = Block<int>;
using BlockF = Block<float>;
using BlockD = Block<double>;

//-----------------------------------------------------------------------------------------------// 

} // mpx

//-----------------------------------------------------------------------------------------------// 

#endif
