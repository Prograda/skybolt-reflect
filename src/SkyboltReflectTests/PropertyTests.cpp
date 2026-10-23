// Copyright (c) Prograda Pty. Ltd.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#include <catch2/catch_test_macros.hpp>
#include <SkyboltReflect/Reflection.h>

using namespace skybolt::refl;

struct ClassWithMembers
{
	int a;
	int b;
};

struct ClassWithMethods
{
	int a;
	int b;

	int getA() const { return a; }

	void setB(int v) { b = v; }
	int getB() const { return b; }
};

struct OtherClassWithMembers
{
	int a;
	int b;
};

SKYBOLT_REFLECT(PropertyTestClasses) {
	registry.type<ClassWithMembers>("ClassWithMembers")
		// Read-only member property
		.propertyReadOnly("a", &ClassWithMembers::a)
		// Member property
		.property("b", &ClassWithMembers::b);
	registry.type<ClassWithMethods>("ClassWithMethods")
		// Getter method property
		.propertyReadOnly("a", &ClassWithMethods::getA)
		// Getter/setter method property
		.property("b", &ClassWithMethods::getB, &ClassWithMethods::setB);
	registry.type<OtherClassWithMembers>("OtherClassWithMembers")
		// Getter function property
		.propertyReadOnlyFn<int>("a",
			[](const OtherClassWithMembers& obj) { return obj.a; })
			// Getter/setter function property
		.propertyFn<int, int>("b",
			[](const OtherClassWithMembers& obj) { return obj.b; },
			[](OtherClassWithMembers& obj, int value) { obj.b = value; });
}

TEST_CASE("Properties backed by class members")
{
	TypeRegistry registry;

	auto type = registry.getTypeByName("ClassWithMembers");
	REQUIRE(type);

	SECTION("Get read-only property")
	{
		auto property = type->getProperty("a");
		REQUIRE(property);

		ClassWithMembers obj{.a = 123};
		auto instance = makeRefInstance(registry, &obj);
		CHECK(property->getValue(instance).cast<int>() == 123);
	}

	SECTION("Set and get property")
	{
		auto property = type->getProperty("b");
		REQUIRE(property);

		ClassWithMembers obj{};
		auto instance = makeRefInstance(registry, &obj);
		property->setValue(instance, makeValueInstance(registry, 123));
		CHECK(obj.b == 123);

		obj.b = 456;
		CHECK(property->getValue(instance).cast<int>() == 456);
	}
}

TEST_CASE("Properties backed by getter and setter methods")
{
	TypeRegistry registry;

	auto type = registry.getTypeByName("ClassWithMethods");
	REQUIRE(type);

	SECTION("Get read-only property")
	{
		auto property = type->getProperty("a");
		REQUIRE(property);

		ClassWithMethods obj{ .a = 123 };
		auto instance = makeRefInstance(registry, &obj);
		CHECK(property->getValue(instance).cast<int>() == 123);
	}

	SECTION("Set and get property")
	{
		auto property = type->getProperty("b");
		REQUIRE(property);

		ClassWithMethods obj{};
		auto instance = makeRefInstance(registry, &obj);
		property->setValue(instance, makeValueInstance(registry, 123));
		CHECK(obj.b == 123);

		obj.b = 456;
		CHECK(property->getValue(instance).cast<int>() == 456);
	}
}

TEST_CASE("Properties backed by getter and setter functions")
{
	TypeRegistry registry;

	auto type = registry.getTypeByName("OtherClassWithMembers");
	REQUIRE(type);

	SECTION("Get read-only property")
	{
		auto property = type->getProperty("a");
		REQUIRE(property);

		OtherClassWithMembers obj{ .a = 123 };
		auto instance = makeRefInstance(registry, &obj);
		CHECK(property->getValue(instance).cast<int>() == 123);
	}

	SECTION("Set and get property")
	{
		auto property = type->getProperty("b");
		REQUIRE(property);

		OtherClassWithMembers obj{};
		auto instance = makeRefInstance(registry, &obj);
		property->setValue(instance, makeValueInstance(registry, 123));
		CHECK(obj.b == 123);

		obj.b = 456;
		CHECK(property->getValue(instance).cast<int>() == 456);
	}
}
