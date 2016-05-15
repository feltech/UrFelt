/*
** Lua binding: UrFelt
** Generated automatically by tolua++-1.0.93 on Mon Apr  4 22:56:41 2016.
*/

//
// Copyright (c) 2008-2015 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include <toluapp/tolua++.h>
#include <Urho3D/Urho3D.h>
#include <Urho3D/LuaScript/ToluaUtils.h>

#if __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif

/* Exported function */
TOLUA_API int tolua_UrFelt_open (lua_State* tolua_S);

#include "UrFelt.hpp"
using namespace felt;
using namespace Urho3D;

#undef self

#define TOLUA_DISABLE_tolua_UrFelt_GetFelt00
static int tolua_UrFelt_GetFelt00(lua_State* tolua_S)
{

    return ToluaGetSubsystem<UrFelt>(tolua_S);
}

#define TOLUA_DISABLE_tolua_get_felt_ptr
#define tolua_get_felt_ptr tolua_UrFelt_GetFelt00

/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
 tolua_usertype(tolua_S,"UrFelt");
 tolua_usertype(tolua_S,"Urho3D::Ray");
 tolua_usertype(tolua_S,"Urho3D::Application");
}

/* method: zap of class  UrFelt */
#ifndef TOLUA_DISABLE_tolua_UrFelt_UrFelt_zap00
static int tolua_UrFelt_UrFelt_zap00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"UrFelt",0,&tolua_err) ||
 (tolua_isvaluenil(tolua_S,2,&tolua_err) || !tolua_isusertype(tolua_S,2,"const Urho3D::Ray",0,&tolua_err)) ||
 !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  UrFelt* self = (UrFelt*)  tolua_tousertype(tolua_S,1,0);
  const Urho3D::Ray* ray = ((const Urho3D::Ray*)  tolua_tousertype(tolua_S,2,0));
  const float amount = ((const float)  tolua_tonumber(tolua_S,3,0));
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'zap'", NULL);
#endif
 {
  self->zap(*ray,amount);
 tolua_pushnumber(tolua_S,(lua_Number)amount);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'zap'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: repoly of class  UrFelt */
#ifndef TOLUA_DISABLE_tolua_UrFelt_UrFelt_repoly00
static int tolua_UrFelt_UrFelt_repoly00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"UrFelt",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  UrFelt* self = (UrFelt*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'repoly'", NULL);
#endif
 {
  self->repoly();
 }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'repoly'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* get function: felt */
#ifndef TOLUA_DISABLE_tolua_get_felt_ptr
static int tolua_get_felt_ptr(lua_State* tolua_S)
{
  tolua_pushusertype(tolua_S,(void*)GetFelt(),"UrFelt");
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* function: GetFelt */
#ifndef TOLUA_DISABLE_tolua_UrFelt_GetFelt00
static int tolua_UrFelt_GetFelt00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isnoobj(tolua_S,1,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
 {
  UrFelt* tolua_ret = (UrFelt*)  GetFelt();
  tolua_pushusertype(tolua_S,(void*)tolua_ret,"UrFelt");
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetFelt'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* get function: felt */
#ifndef TOLUA_DISABLE_tolua_get_felt_ptr
static int tolua_get_felt_ptr(lua_State* tolua_S)
{
  tolua_pushusertype(tolua_S,(void*)GetFelt(),"UrFelt");
 return 1;
}
#endif //#ifndef TOLUA_DISABLE

/* Open function */
TOLUA_API int tolua_UrFelt_open (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,1);
 tolua_beginmodule(tolua_S,NULL);
 tolua_cclass(tolua_S,"UrFelt","UrFelt","Urho3D::Application",NULL);
 tolua_beginmodule(tolua_S,"UrFelt");
  tolua_function(tolua_S,"zap",tolua_UrFelt_UrFelt_zap00);
  tolua_function(tolua_S,"repoly",tolua_UrFelt_UrFelt_repoly00);
 tolua_endmodule(tolua_S);
 tolua_variable(tolua_S,"felt",tolua_get_felt_ptr,NULL);
 tolua_function(tolua_S,"GetFelt",tolua_UrFelt_GetFelt00);
 tolua_variable(tolua_S,"felt",tolua_get_felt_ptr,NULL);
 tolua_endmodule(tolua_S);
 return 1;
}


#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 501
 TOLUA_API int luaopen_UrFelt (lua_State* tolua_S) {
 return tolua_UrFelt_open(tolua_S);
};
#endif

#if __clang__
#pragma clang diagnostic pop
#endif