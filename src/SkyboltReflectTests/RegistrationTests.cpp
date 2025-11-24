// Copyright (c) Prograda Pty. Ltd.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#include <catch2/catch.hpp>
#include <SkyboltReflect/Reflection.h>

using namespace skybolt::refl;

struct RegistrationTestClass
{
	int a;
};

SKYBOLT_REFLECT(RegistrationTestClass) {
	registry.type<RegistrationTestClass>("RegistrationTestClass")
		.property("a", &RegistrationTestClass::a);
}

TEST_CASE("Get registered class by type and name")
{
	TypeRegistry registry;
	registry.addType(std::make_shared<Type>("RegistrationTestClassName", typeid(RegistrationTestClass)));
	CHECK(registry.getType<RegistrationTestClass>() != nullptr);
	CHECK(registry.getTypeByName("RegistrationTestClassName") != nullptr);
}


struct OtherClass {};
struct TypeReferringToAnotherType
{
	OtherClass value;
};

SKYBOLT_REFLECT(RegistrationTestClasses) {
	registry.type<TypeReferringToAnotherType>("TypeReferringToAnotherType")
		.property("value", &TypeReferringToAnotherType::value);
	registry.type<OtherClass>("OtherClassCustomName");
}

TEST_CASE("Type that type can be registered after the type that refers to it")
{
	// This tests that delayed registration of type definitions works correctly.
	// All the explicitly reflected types must be registered before type properties which may
	// refer to other types.
	TypeRegistry registry;

	auto type = registry.getTypeByName("TypeReferringToAnotherType");
	REQUIRE(type);
	auto property = type->getProperty("value");
	REQUIRE(property);
	
	TypeReferringToAnotherType object;
	Instance value = property->getValue(makeRefInstance(registry, &object));
	CHECK(value.getType()->getName() == "OtherClassCustomName");
}