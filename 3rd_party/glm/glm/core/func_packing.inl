///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2011 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2010-03-17
// Updated : 2010-03-17
// Licence : This source is under MIT License
// File    : glm/core/func_packing.inl
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace glm{
namespace core{
namespace function{
namespace packing{

GLM_FUNC_QUALIFIER detail::uint32 packUnorm2x16(detail::tvec2<detail::float32> const & v)
{
	detail::uint16 A(detail::uint16(round(clamp(v.x, 0.0f, 1.0f) * 65535.0f)));
	detail::uint16 B(detail::uint16(round(clamp(v.y, 0.0f, 1.0f) * 65535.0f)));
	return detail::uint32((B << 16) | A);
}

GLM_FUNC_QUALIFIER detail::tvec2<detail::float32> unpackUnorm2x16(detail::uint32 const & p)
{
	detail::uint32 Mask16((1 << 16) - 1);
	detail::uint32 A((p >>  0) & Mask16);
	detail::uint32 B((p >> 16) & Mask16);
	return detail::tvec2<detail::float32>(
		A * 1.0f / 65535.0f, 
		B * 1.0f / 65535.0f);
}
	
GLM_FUNC_QUALIFIER detail::uint32 packSnorm2x16(detail::tvec2<detail::float32> const & v)
{
	union iu
	{
		detail::int16 i;
		detail::uint16 u;
	} A, B;
		
	detail::tvec2<detail::float32> Unpack = clamp(v ,-1.0f, 1.0f) * 32767.0f;
	A.i = detail::int16(round(Unpack.x));
	B.i = detail::int16(round(Unpack.y));
	detail::uint32 Pack = (detail::uint32(B.u) << 16) | (detail::uint32(A.u) << 0);
	return Pack;
}

GLM_FUNC_QUALIFIER detail::tvec2<detail::float32> unpackSnorm2x16(detail::uint32 const & p)
{
	union iu
	{
		detail::int16 i;
		detail::uint16 u;
	} A, B;
		
	detail::uint32 Mask16((1 << 16) - 1);
	A.u = detail::uint16((p >>  0) & Mask16);
	B.u = detail::uint16((p >> 16) & Mask16);
	detail::tvec2<detail::float32> Pack(A.i, B.i);
		
	return clamp(Pack * 1.0f / 32767.0f, -1.0f, 1.0f);
}

GLM_FUNC_QUALIFIER detail::uint32 packUnorm4x8(detail::tvec4<detail::float32> const & v)
{
	detail::uint8 A((detail::uint8)round(clamp(v.x, 0.0f, 1.0f) * 255.0f));
	detail::uint8 B((detail::uint8)round(clamp(v.y, 0.0f, 1.0f) * 255.0f));
	detail::uint8 C((detail::uint8)round(clamp(v.z, 0.0f, 1.0f) * 255.0f));
	detail::uint8 D((detail::uint8)round(clamp(v.w, 0.0f, 1.0f) * 255.0f));
	return detail::uint32((D << 24) | (C << 16) | (B << 8) | A);
}

GLM_FUNC_QUALIFIER detail::tvec4<detail::float32> unpackUnorm4x8(detail::uint32 const & p)
{	
	detail::uint32 Mask8((1 << 8) - 1);
	detail::uint32 A((p >>  0) & Mask8);
	detail::uint32 B((p >>  8) & Mask8);
	detail::uint32 C((p >> 16) & Mask8);
	detail::uint32 D((p >> 24) & Mask8);
	return detail::tvec4<detail::float32>(
		A * 1.0f / 255.0f, 
		B * 1.0f / 255.0f, 
		C * 1.0f / 255.0f, 
		D * 1.0f / 255.0f);
}
	
GLM_FUNC_QUALIFIER detail::uint32 packSnorm4x8(detail::tvec4<detail::float32> const & v)
{
	union iu
	{
		detail::int8 i;
		detail::uint8 u;
	} A, B, C, D;
	
	detail::tvec4<detail::float32> Unpack = clamp(v ,-1.0f, 1.0f) * 127.0f;
	A.i = detail::int8(round(Unpack.x));
	B.i = detail::int8(round(Unpack.y));
	C.i = detail::int8(round(Unpack.z));
	D.i = detail::int8(round(Unpack.w));
	detail::uint32 Pack = (detail::uint32(D.u) << 24) | (detail::uint32(C.u) << 16) | (detail::uint32(B.u) << 8) | (detail::uint32(A.u) << 0);
	return Pack;
}
	
GLM_FUNC_QUALIFIER detail::tvec4<detail::float32> unpackSnorm4x8(detail::uint32 const & p)
{	
	union iu
	{
		detail::int8 i;
		detail::uint8 u;
	} A, B, C, D;
	
	detail::uint32 Mask8((1 << 8) - 1);
	A.u = detail::uint8((p >>  0) & Mask8);
	B.u = detail::uint8((p >>  8) & Mask8);
	C.u = detail::uint8((p >> 16) & Mask8);
	D.u = detail::uint8((p >> 24) & Mask8);
	detail::tvec4<detail::float32> Pack(A.i, B.i, C.i, D.i);
	
	return clamp(Pack * 1.0f / 127.0f, -1.0f, 1.0f);
}

GLM_FUNC_QUALIFIER double packDouble2x32(detail::tvec2<detail::uint32> const & v)
{
	return *(double*)&v;
}

GLM_FUNC_QUALIFIER detail::tvec2<uint> unpackDouble2x32(double const & v)
{
	return *(detail::tvec2<uint>*)&v;
}

GLM_FUNC_QUALIFIER uint packHalf2x16(detail::tvec2<float> const & v)
{
	detail::tvec2<detail::hdata> Pack(detail::toFloat16(v.x), detail::toFloat16(v.y));
	return *(uint*)&Pack;
}

GLM_FUNC_QUALIFIER vec2 unpackHalf2x16(uint const & v)
{
	detail::tvec2<detail::hdata> Unpack = *(detail::tvec2<detail::hdata>*)&v;
	return vec2(detail::toFloat32(Unpack.x), detail::toFloat32(Unpack.y));
}

}//namespace packing
}//namespace function
}//namespace core
}//namespace glm
