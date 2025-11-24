// Copyright (c) Prograda Pty. Ltd.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#include <catch2/catch.hpp>
#include <SkyboltReflect/Reflection.h>

using namespace skybolt::refl;

struct PropertyMetadataTestClass
{
	int a;
	int b;
	int c;

	void setB(int v) { b = v; }
	int getB() const { return b; }
};

SKYBOLT_REFLECT(PropertyMetadataTestClass) {
	registry.type<PropertyMetadataTestClass>("PropertyMetadataTestClass")
		.property("a", &PropertyMetadataTestClass::a, { { "key1", std::string("value1") } })
		.property("b", &PropertyMetadataTestClass::getB, &PropertyMetadataTestClass::setB, { { "key1", std::string("value1") } })
		.propertyFn<int, int>("c",
			[](const PropertyMetadataTestClass& obj) { return obj.c; },
			[](PropertyMetadataTestClass& obj, int value) { obj.c = value; },
			{ { "key1", std::string("value1") } });
}

TEST_CASE("Get property metadata")
{
	TypeRegistry registry;

	auto type = registry.getTypeByName("PropertyMetadataTestClass");
	REQUIRE(type);

	SECTION("Member property metadata")
	{
		auto property = type->getProperty("a");
		REQUIRE(property);
		CHECK(std::any_cast<std::string>(property->getMetadata("key1")) == "value1");
	}

	SECTION("Method property metadata")
	{
		auto property = type->getProperty("b");
		REQUIRE(property);
		CHECK(std::any_cast<std::string>(property->getMetadata("key1")) == "value1");
	}

	SECTION("Function property metadata")
	{
		auto property = type->getProperty("c");
		REQUIRE(property);
		CHECK(std::any_cast<std::string>(property->getMetadata("key1")) == "value1");
	}
}
