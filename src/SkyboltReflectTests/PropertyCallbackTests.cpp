// Copyright (c) Prograda Pty. Ltd.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#include <catch2/catch_test_macros.hpp>
#include <SkyboltReflect/Reflection.h>

using namespace skybolt::refl;

struct PropertyCallbackTestClass
{
	int a;
	int b;
	int c;

	int getB() const { return b; }
	void setB(int value) { b = value; }

	std::vector<std::string> changedPropertyNames;

	void onPropertyChanged(const std::string& name)
	{
		changedPropertyNames.push_back(name);
	}
};

SKYBOLT_REFLECT(PropertyCallbackTestClass) {
	registry.type<PropertyCallbackTestClass>("PropertyCallbackTestClass")
		.property("a", &PropertyCallbackTestClass::a, {}, &PropertyCallbackTestClass::onPropertyChanged)
		.property("b", &PropertyCallbackTestClass::getB, &PropertyCallbackTestClass::setB, {}, &PropertyCallbackTestClass::onPropertyChanged)
		.propertyFn<int, int>("c",
			[](const PropertyCallbackTestClass& c) { return c.c; },
			[](PropertyCallbackTestClass& c, int value) { c.c = value; },
			{}, &PropertyCallbackTestClass::onPropertyChanged);
}


TEST_CASE("Test property value change callback is called when property value changes")
{
	TypeRegistry registry;

    auto type = registry.getTypeByName("PropertyCallbackTestClass");
    auto property = type->getProperty("a");

    PropertyCallbackTestClass object{};
    auto instance = makeRefInstance(registry, &object);

    // Set value
    property->setValue(instance, makeValueInstance(registry, 2));

	// Check that the callback was called
    REQUIRE(object.changedPropertyNames.size() == 1);
	CHECK(object.changedPropertyNames.front() == "a");
}