// Copyright (c) Prograda Pty. Ltd.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#include <catch2/catch.hpp>
#include <SkyboltReflect/Reflection.h>

using namespace skybolt::refl;

struct BaseA
{
	virtual ~BaseA() = default;

	int intA = 0;
};

struct BaseB
{
	virtual ~BaseB() = default;

	int intB = 0;
};

struct Derived : public BaseA, BaseB
{
	~Derived() override = default;

	int intC = 0;
};

struct MultiLevelDerived : public Derived
{
	~MultiLevelDerived() override = default;

	int intD = 0;
};

SKYBOLT_REFLECT(PolymorphicReflectionTestClasses) {
	registry.type<BaseA>("BaseA")
		.property("intA", &BaseA::intA);
	registry.type<BaseB>("BaseB")
		.property("intB", &BaseB::intB);
	registry.type<Derived>("Derived")
		.superType<BaseA>()
		.superType<BaseB>()
		.property("intC", &Derived::intC);
	registry.type<MultiLevelDerived>("MultiLevelDerived")
		.superType<Derived>()
		.property("intD", &MultiLevelDerived::intD);
}

TEST_CASE("Derived type has super type properties")
{
	TypeRegistry registry;

	auto type = registry.getTypeByName("Derived");
	REQUIRE(type);
	CHECK(type->isDerivedFrom<BaseA>());
	CHECK(type->isDerivedFrom<BaseB>());
	CHECK(type->getProperty("intA"));
	CHECK(type->getProperty("intB"));
	CHECK(type->getProperty("intC"));
}

TEST_CASE("Multi-level derived type has super type properties")
{
	TypeRegistry registry;

	auto type = registry.getTypeByName("MultiLevelDerived");
	REQUIRE(type);
	CHECK(type->isDerivedFrom<Derived>());
	CHECK(type->isDerivedFrom<BaseA>());
	CHECK(type->isDerivedFrom<BaseB>());
	CHECK(type->getProperty("intA"));
	CHECK(type->getProperty("intB"));
	CHECK(type->getProperty("intC"));
	CHECK(type->getProperty("intD"));
}

TEST_CASE("Access properties of type with multiple super classes")
{
	TypeRegistry registry;

	auto type = registry.getTypeByName("Derived");
	REQUIRE(type);
	auto derivedClassProperty = type->getProperty("intC");
	REQUIRE(derivedClassProperty);
	auto firstBaseClassProperty = type->getProperty("intA");
	REQUIRE(firstBaseClassProperty);
	auto secondBaseClassProperty = type->getProperty("intB");
	REQUIRE(secondBaseClassProperty);
	
	Derived derivedObj;

	SECTION("Set derived class value on instance created from derived class")
	{
		auto instance = makeRefInstance(registry, &derivedObj);
		derivedClassProperty->setValue(instance, makeValueInstance(registry, 1));
		CHECK(derivedObj.intC == 1);
	}

	SECTION("Set derived class value on instance created from second base class")
	{
		BaseB* baseObj = &derivedObj;
		auto instance = makeRefInstance(registry, baseObj);
		derivedClassProperty->setValue(instance, makeValueInstance(registry, 2));
		CHECK(derivedObj.intC == 2);
	}

	SECTION("Set derived class value on instance created from second base class")
	{
		BaseB* baseObj = &derivedObj;
		auto instance = makeRefInstance(registry, baseObj);
		derivedClassProperty->setValue(instance, makeValueInstance(registry, 3));
		CHECK(derivedObj.intC == 3);
	}

	SECTION("Set first base class value on instance created from second base class")
	{
		BaseB* baseObj = &derivedObj;
		auto instance = makeRefInstance(registry, baseObj);
		firstBaseClassProperty->setValue(instance, makeValueInstance(registry, 1));
		CHECK(derivedObj.intA == 1);
	}

	SECTION("Set second base class value on instance created from first base class")
	{
		BaseA* baseObj = &derivedObj;
		auto instance = makeRefInstance(registry, baseObj);
		secondBaseClassProperty->setValue(instance, makeValueInstance(registry, 2));
		CHECK(derivedObj.intB == 2);
	}

	SECTION("Set second base class value on instance created from derived class")
	{
		auto instance = makeRefInstance(registry, &derivedObj);
		secondBaseClassProperty->setValue(instance, makeValueInstance(registry, 3));
		CHECK(derivedObj.intB == 3);
	}
}


TEST_CASE("Access properties of multi-level derived type")
{
	TypeRegistry registry;

	auto type = registry.getTypeByName("MultiLevelDerived");
	REQUIRE(type);
	auto multiLevelDerivedClassProperty = type->getProperty("intD");
	REQUIRE(multiLevelDerivedClassProperty);
	auto derivedClassProperty = type->getProperty("intC");
	REQUIRE(derivedClassProperty);
	auto baseClassProperty = type->getProperty("intA");
	REQUIRE(baseClassProperty);
	
	MultiLevelDerived derivedObj;

	SECTION("Set proeprty values on instance created from multi-level derived class")
	{
		auto instance = makeRefInstance(registry, &derivedObj);
		
		multiLevelDerivedClassProperty->setValue(instance, makeValueInstance(registry, 1));
		CHECK(derivedObj.intD == 1);

		derivedClassProperty->setValue(instance, makeValueInstance(registry, 2));
		CHECK(derivedObj.intC == 2);

		baseClassProperty->setValue(instance, makeValueInstance(registry, 3));
		CHECK(derivedObj.intA == 3);
	}
}
