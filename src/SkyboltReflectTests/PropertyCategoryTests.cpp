// Copyright (c) Prograda Pty. Ltd.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#include <catch2/catch.hpp>
#include <SkyboltReflect/Reflection.h>

using namespace skybolt::refl;


struct PropertyCategoryTestClass
{
	int a;
	int b;
};

SKYBOLT_REFLECT(PropertyCategoryTestClass) {
	registry.type<PropertyCategoryTestClass>("PropertyCategoryTestClass")
		.category("A")
		.property("a", &PropertyCategoryTestClass::a)
		.category("B")
		.property("b", &PropertyCategoryTestClass::b);
}

TEST_CASE("Test that property is registered in category")
{
	TypeRegistry registry;

	auto type = registry.getTypeByName("PropertyCategoryTestClass");
	
	auto property = type->getProperty("a");
	REQUIRE(property);
	CHECK(property->getCategory() == "A");

	property = type->getProperty("b");
	REQUIRE(property);
	CHECK(property->getCategory() == "B");
}