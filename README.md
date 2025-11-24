# Skybolt Reflect

**A lightweight, header-only C++ runtime reflection system**

## Features
* Runtime type introspection
* Properties backed by class members, getter/setter methods, and getter/setter lambdas
* Class inheritance support
* Nested object support
* `std::vector` containers
* `std::optional` values
* Property value change callbacks
* Works across DLL boundaries without duplicate static definition issues
* Custom property metadata

## Projects using Skybolt Reflect

* [Skybolt Widgets](https://github.com/prograda/skybolt-widgets) provides useful [Qt](https://qt.io) widgets, including a property editor backed by Skybolt Reflect.
* [Skybolt Engine](https://github.com/prograda/skybolt) uses Skybolt Reflect to reflect entity component properties.

---

### Installation
The library may be installed in one of two ways:
#### 1. Header Only
Define the `SKYBOLT_REFLECT_IMPL` macro in exactly one cpp file in your project, and include `src/SkyboltReflect/SkyboltReflect.h`. E.g.
```
#define SKYBOLT_REFLECT_IMPL
#include "SkyboltReflect.h"
```

#### 2. Compiled Library
Compile both `src/SkyboltReflect/SkyboltReflect.h` and `src/SkyboltReflect/SkyboltReflect.cpp` into a library and link to it. Do not define `SKYBOLT_REFLECT_IMPL` in your code.


### Usage
#### Basic Example

```cpp
#define SKYBOLT_REFLECT_IMPL // Must be defined in exactly one cpp file
#include "SkyboltReflect/Reflection.h"

using namespace skybolt::refl;

struct Vec2
{
    float x = 0, y = 0;
};

SKYBOLT_REFLECT(Vec2)
{
    registry.type<Vec2>("Vec2")
        .property("x", &Vec2::x)
        .property("y", &Vec2::y);
}

TEST_CASE("Get and set class member properties")
{
    TypeRegistry registry;

    auto type = registry.getTypeByName("Vec2");
    auto property = type->getProperty("x");

    Vec2 v{1.f, 2,f};
    auto instance = makeRefInstance(registry, &v);

    // Get value
    float value = property->getValue(instance).cast<float>();
    CHECK(value == 1.f);

    // Set value
    property->setValue(instance, makeValueInstance(registry, 2.f));
    CHECK(value == 2.f);
}
```

#### Type Registration
Reflected types may be registered in either a .cpp or a header file using `SKYBOLT_REFLECT(MyModuleName)`. `MyModuleName` can be any name you like, so long as each `SKYBOLT_REFLECT` definition has a unique name.

If you use the `SKYBOLT_REFLECT` macro within a .cpp file, you must place `SKYBOLT_REFLECT_EXTERN(MyModuleName)` in the corresponding header. This creates a symbolic dependency that ensures the module is correctly linked into the final executable. This is a mechanism to bypass a standard C++ linker optimization: if an object file contains only staticlly initialized objects and no functions called by the main application, the linker may discard the file as "dead code." The `SKYBOLT_REFLECT_EXTERN` macro forces the linker to include the module, ensuring your types are registered at startup.

#### Feature Examples
* [Property getter/setters](src/SkyboltReflectTests/PropertyTests.cpp)
* [Property categories](src/SkyboltReflectTests/PropertyCategoryTests.cpp)
* [Property metadata](src/SkyboltReflectTests/PropertyMetadataTests.cpp)
* [Containers](src/SkyboltReflectTests/ContainerTests.cpp)
* [Class hierarchies](src/SkyboltReflectTests/PolymorphicReflectionTests.cpp)
* [Property value change callbacks](src/SkyboltReflectTests/PropertyCallbackTests.cpp)

#### Shared Library Usage
When you build an application using shared libraries on Windows (i.e. DLL files), each shared library normally gets its own private "island" of memory. This means that by default, types registered in shared libraries are not visible to other parts of your application.

To fix this and ensure all parts of your application share a single `TypeRegistry`, call the following in your library initialization code:
```
addStaticallyRegisteredTypes(myTypeRegistry)
```
This adds local statically registered types into your central `TypeRegistry` used by the rest of your application.

---

### Key Concepts

| Concept                 | Description |
|-------------------------|-------------|
| `TypeRegistry`          | Registry of all reflected types |
| `Type`                  | Information about a reflected C++ type |
| `Property`              | Base class for properties of types |
| `Instance`              | A type-safe wrapper around raw property values |
| `TypeBuilder`           | Builder API for defining types and properties |

---

### Macros
- `SKYBOLT_REFLECT_IMPL` - defines implementation for header-only usage
- `SKYBOLT_REFLECT(moduleName)` – defines a reflection registration block
- `SKYBOLT_REFLECT_EXTERN(moduleName)` – exposes a .cpp reflection definition block in a header to ensure correct linkage

---

### License
MIT License

---

## Contact
To submit a bug report, please [raise an issue on the GitHub repository](https://github.com/Prograda/skybolt-reflect/issues).