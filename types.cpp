/*
********************************************************************
(c) MSU Video Group 2003-2010, http://compression.ru/video/
This source code is property of MSU Graphics and Media Lab

This code can be distributed and used only with WRITTEN PERMISSION.
********************************************************************
*/


#include "types.h"

MV::MV()
{
	splitted = false;
	sub[0] = sub[1] = sub[2] = sub[3] = NULL;
	x = y = 0;
	error = MAXLONG;
	dir = sd_up;
}

MV MV::operator +=(MV vector)
{
	x += vector.x;
	y += vector.y;
	return *this;
}
MV MV::operator -=(MV vector)
{
	x -= vector.x;
	y -= vector.y;
	return *this;
}
MV MV::operator *=( int mul )
{
	x *= mul;
	y *= mul;
	return *this;
}
MV MV::operator /=( int mul )
{
	x /= mul;
	y /= mul;
	return *this;
}
MV MV::operator +(MV vector)
{
	MV res;
	res.x = x + vector.x;
	res.y = y + vector.y;
	return res;
}


MV MV::operator -(MV vector)
{
	MV res;
	res.x = x - vector.x;
	res.y = y - vector.y;
	return res;
}