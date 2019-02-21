/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef BW_FUNCTOR_HPP
#define BW_FUNCTOR_HPP


#include "smartpointer.hpp"


// 0 param functor, no return

class BWBaseFunctor0 : public ReferenceCount
{
public:
	virtual void operator()() = 0;
};

template< class C >
class BWFunctor0 : public BWBaseFunctor0
{
public:
	typedef void (C::*Method)();
	BWFunctor0( C* instance, Method method )
		: instance_( instance )
		, method_( method )
	{};
	void operator()()
	{
		(instance_->*method_)();
	}
private:
	C* instance_;
	Method method_;
};


// 1 param functor, no return

template< typename P1 >
class BWBaseFunctor1 : public ReferenceCount
{
public:
	virtual void operator()( P1 param1 ) = 0;
};

template< class C, typename P1 >
class BWFunctor1 : public BWBaseFunctor1< P1 >
{
public:
	typedef void (C::*Method)( P1 );
	BWFunctor1( C* instance, Method method )
		: instance_( instance )
		, method_( method )
	{};
	void operator()( P1 param1 )
	{
		(instance_->*method_)( param1 );
	}
private:
	C* instance_;
	Method method_;
};


// 2 param functor, no return

template< typename P1, typename P2 >
class BWBaseFunctor2 : public ReferenceCount
{
public:
	virtual void operator()( P1 param1, P2 param2 ) = 0;
};

template< class C, typename P1, typename P2 >
class BWFunctor2 : public BWBaseFunctor2< P1, P2 >
{
public:
	typedef void (C::*Method)( P1, P2 );
	BWFunctor2( C* instance, Method method )
		: instance_( instance )
		, method_( method )
	{};
	void operator()( P1 param1, P2 param2 )
	{
		(instance_->*method_)( param1, param2 );
	}
private:
	C* instance_;
	Method method_;
};


// 0 param functor

template< typename R >
class BWBaseFunctor0R : public ReferenceCount
{
public:
	virtual R operator()() = 0;
};

template< class C, typename R >
class BWFunctor0R : public BWBaseFunctor0R< R >
{
public:
	typedef R (C::*Method)();
	BWFunctor0R( C* instance, Method method )
		: instance_( instance )
		, method_( method )
	{};
	R operator()()
	{
		return (instance_->*method_)();
	}
private:
	C* instance_;
	Method method_;
};


// 1 param functor, no return

template< typename P1, typename R >
class BWBaseFunctor1R : public ReferenceCount
{
public:
	virtual R operator()( P1 param1 ) = 0;
};

template< class C, typename P1, typename R >
class BWFunctor1R : public BWBaseFunctor1R< P1, R >
{
public:
	typedef R (C::*Method)( P1 );
	BWFunctor1R( C* instance, Method method )
		: instance_( instance )
		, method_( method )
	{};
	R operator()( P1 param1 )
	{
		return (instance_->*method_)( param1 );
	}
private:
	C* instance_;
	Method method_;
};


// 2 param functor, no return

template< typename P1, typename P2, typename R >
class BWBaseFunctor2R : public ReferenceCount
{
public:
	virtual R operator()( P1 param1, P2 param2 ) = 0;
};

template< class C, typename P1, typename P2, typename R >
class BWFunctor2R : public BWBaseFunctor2R< P1, P2, R >
{
public:
	typedef R (C::*Method)( P1, P2 );
	BWFunctor2R( C* instance, Method method )
		: instance_( instance )
		, method_( method )
	{};
	R operator()( P1 param1, P2 param2 )
	{
		return (instance_->*method_)( param1, param2 );
	}
private:
	C* instance_;
	Method method_;
};


#endif // BW_FUNCTOR_HPP