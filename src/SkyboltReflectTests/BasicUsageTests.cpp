// Copyright (c) Prograda Pty. Ltd.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <SkyboltReflect/Reflection.h>

using namespace skybolt::refl;

struct Vec2
{
    float x = 0, y = 0;
};

SKYBOLT_REFLECT(Vec2) {
    registry.type<Vec2>("Vec2")
        .property("x", &Vec2::x)
        .property("y", &Vec2::y);
    }

TEST_CASE("Get and set class member properties")
{
    TypeRegistry registry;

    auto type = registry.getTypeByName("Vec2");
	REQUIRE(type);
    auto property = type->getProperty("x");

    Vec2 v{1.f, 2.f};
    auto instance = makeRefInstance(registry, &v);

    // Get value
    float value = property->getValue(instance).cast<float>();
    CHECK(value == 1.f);

    // Set value
    property->setValue(instance, makeValueInstance(registry, 2.f));
    CHECK(v.x == 2.f);
}
