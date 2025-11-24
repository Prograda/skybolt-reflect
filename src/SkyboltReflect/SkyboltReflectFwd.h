// Copyright (c) Prograda Pty. Ltd.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#pragma once

#include <memory>

namespace skybolt::refl {

class Instance;
class Property;
class Type;
class TypeRegistry;

using TypePtr = std::shared_ptr<Type>;
using PropertyPtr = std::shared_ptr<Property>;

} // namespace skybolt::refl