// Copyright (c) Prograda Pty. Ltd.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#pragma once

#include <any>
#include <assert.h>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>
#include <typeindex>

namespace skybolt::refl {


class Instance;
class Property;
class Type;
class TypeRegistry;

using TypePtr = std::shared_ptr<Type>;
using PropertyPtr = std::shared_ptr<Property>;

struct ContainerValueAccessor
{
	ContainerValueAccessor(std::string valueTypeName) : valueTypeName(std::move(valueTypeName)) {}
	virtual ~ContainerValueAccessor() = default;

	virtual std::vector<Instance> getValues(TypeRegistry& registry, const Instance& instance) const = 0;

	virtual void setValues(Instance& instance, const std::vector<Instance>& values) const = 0;

	const std::string valueTypeName;
};

using ContainerValueAccessorPtr = std::shared_ptr<ContainerValueAccessor>;

class Type : public std::enable_shared_from_this<Type>
{
public:
	using ValueFactory = std::function<std::shared_ptr<void>()>;

	//! @param containerValueAccessor is an interface for accessing values if type is a container, otherwise null if type is a simple value.
	//! @param valueFactory is an optional factory to create a default instance value of the type; if null, functionality needing the ability to create new instances by reflection will be disabled.
	Type(const std::string& name, const std::type_index& typeIndex, ContainerValueAccessorPtr containerValueAccessor = nullptr, ValueFactory valueFactory = nullptr) :
		mName(name),
		mTypeIndex(typeIndex),
		mContainerValueAccessor(containerValueAccessor),
		mValueFactory(std::move(valueFactory))
	{
	}

	void addProperty(const PropertyPtr& property);
	PropertyPtr getProperty(const std::string& name);

	using PropertyMap = std::map<std::string, PropertyPtr>;
	PropertyMap getProperties() const;

	void addSuperType(const TypePtr&, std::ptrdiff_t offsetFromThisToSuper);
	std::optional<std::ptrdiff_t> getOffsetFromThisToSuper(const std::type_index& super) const;

	template <typename T>
	bool isDerivedFrom() const
	{
		// Look for direct super type
		if (auto i = mSuperTypes.find(typeid(T)); i != mSuperTypes.end())
		{
			return true;
		}
		
		// Look for indirect super types
		for (const auto& [typeIndex, typeAndOffset] : mSuperTypes)
		{
			const TypePtr& superType = typeAndOffset.first;
			if (auto result = superType->isDerivedFrom<T>(); result)
			{
				return true;
			}
		}
		return false;
	}

	//! Creates a default-constructed owning instance of the underlying type.
	//! @returns null if the type is not default constructible.
	std::unique_ptr<Instance> createDefaultInstance() const;

	const std::string& getName() const { return mName; }
	const std::type_index& getTypeIndex() const { return mTypeIndex; }

	const ContainerValueAccessorPtr getContainerValueAccessor() const { return mContainerValueAccessor; } //!< Can return null

private:
	std::string mName;
	std::type_index mTypeIndex;
	ContainerValueAccessorPtr mContainerValueAccessor; //!< Can be null
	ValueFactory mValueFactory; //!< null if instances are not constructible by the reflection system
	PropertyMap mProperties;
	std::map<std::type_index, std::pair<TypePtr, std::ptrdiff_t>> mSuperTypes;
};

template <typename T>
T* addPointerByteOffset(T* p, std::ptrdiff_t offset)
{
	return reinterpret_cast<T*>(reinterpret_cast<unsigned char*>(p) + offset);
}

//! Instance of a Type
class Instance
{
public:
	Instance(std::shared_ptr<void> objectPtr, TypePtr type) :
		mObjectPtr(std::move(objectPtr)),
		mType(std::move(type))
	{
		assert(mObjectPtr);
	}

	TypePtr getType() const { return mType; }

	//! @returns null if cast fails
	template <typename T>
	T* tryCast()
	{
		return const_cast<T*>(static_cast<const Instance*>(this)->tryCast<T>());
	}

	//! Never returns null
	template <typename T>
	const T* tryCast() const
	{
		std::type_index resultTypeIndex = typeid(T);
		if (resultTypeIndex == mType->getTypeIndex()) // if result type is same type
		{
			return static_cast<T*>(mObjectPtr.get());
		}
		else if (auto offset = mType->getOffsetFromThisToSuper(resultTypeIndex); offset) // if result type is a super type
		{
			return static_cast<T*>(addPointerByteOffset(mObjectPtr.get(), *offset));
		}

		return nullptr;
	}

	//! @throws if cast is invalid
	template <typename T>
	T& cast()
	{
		return const_cast<T&>(static_cast<const Instance*>(this)->cast<T>());
	}

	//! @throws if cast is invalid
	template <typename T>
	const T& cast() const
	{
		const T* result = tryCast<T>();
		if (result)
		{
			return *result;
		}

		throw std::runtime_error("Instance::cast() could not cast from " + mType->getName() + " to " + typeid(T).name());
	}

private:
	std::shared_ptr<void> mObjectPtr;
	TypePtr mType;
};

struct StdOptionalValueAccessor : public ContainerValueAccessor
{
	using ContainerValueAccessor::ContainerValueAccessor;
};

template <typename ValueT>
struct StdOptionalValueAccessorT : public StdOptionalValueAccessor
{
	using StdOptionalValueAccessor::StdOptionalValueAccessor;
	~StdOptionalValueAccessorT() override = default;

	std::vector<Instance> getValues(TypeRegistry& registry, const Instance& instance) const override
	{
		const auto& optionalValue = instance.cast<std::optional<ValueT>>();
		if (!optionalValue.has_value())
		{
			return {};
		}
		return { makeValueInstance(registry, optionalValue.value()) };	
	};

	void setValues(Instance& instance, const std::vector<Instance>& values) const override
	{
		auto& optionalValue = instance.cast<std::optional<ValueT>>();
		optionalValue = !values.empty() ? (values.front().cast<ValueT>()) : std::optional<ValueT>();
	};
};

struct StdVectorValueAccessor : public ContainerValueAccessor
{
	using ContainerValueAccessor::ContainerValueAccessor;
};

template <typename ValueT>
struct StdVectorValueAccessorT : public StdVectorValueAccessor
{
	using StdVectorValueAccessor::StdVectorValueAccessor;
	~StdVectorValueAccessorT() override = default;

	std::vector<Instance> getValues(TypeRegistry& registry, const Instance& instance) const override
	{
		const auto& values = instance.cast<std::vector<ValueT>>();

		std::vector<Instance> result;
		for (const ValueT& v : values)
		{
			result.push_back(makeValueInstance(registry, v));
		}
		return result;
	};

	void setValues(Instance& instance, const std::vector<Instance>& values) const override
	{
		auto& destinationValues = instance.cast<std::vector<ValueT>>();

		destinationValues.clear();
		destinationValues.reserve(values.size());
		for (const auto& value : values)
		{
			destinationValues.push_back(value.cast<ValueT>());
		}
	};
};

template<typename ValueT>
struct ContainerValueAccessorFactory
{
	static ContainerValueAccessorPtr create(TypeRegistry& registry)
	{
		return nullptr;
	}
};

//! Can return null
template<typename T>
ContainerValueAccessorPtr createContainerValueAccessor(TypeRegistry& registry)
{
    return ContainerValueAccessorFactory<std::decay_t<T>>::create(registry);
}

template <typename T>
inline TypePtr createTypeForClass(TypeRegistry& registry, const std::string& name, const std::type_index& typeIndex)
{
	Type::ValueFactory factory = nullptr;
	if constexpr (std::is_default_constructible_v<T>)
	{
		factory = []() { return std::make_shared<T>(); };
	}

	return std::make_shared<Type>(name, typeIndex, createContainerValueAccessor<T>(registry), std::move(factory));
}

class TypeRegistry
{
public:
	TypeRegistry();
	void addType(const TypePtr& type);

	TypePtr getTypeByName(const std::string& name) const;

	TypePtr getTypeByIndex(const std::type_index& index) const
	{
		if (auto i = mTypesByTypeIndex.find(index); i != mTypesByTypeIndex.end())
		{
			return i->second;
		}
		return nullptr;
	}

	template <typename T>
	TypePtr getType() const
	{
		return getTypeByIndex(typeid(T));
	}

	template <typename T>
	TypePtr getMostDerivedType(const T& object) const
	{
		return getTypeByIndex(typeid(object));
	}

	template <typename T>
	TypePtr getTypeRequired() const
	{
		if (auto type = getType<T>(); type)
		{
			return type;
		}
		throw std::runtime_error("Could not find type " + std::string(typeid(T).name()));
	}

	template <typename T>
	TypePtr getOrCreateType(const std::type_index& index = typeid(T))
	{
		if (TypePtr t = getTypeByIndex(index); t)
		{
			return t;
		}

		Type::ValueFactory factory = nullptr;
		if constexpr (std::is_default_constructible_v<T>)
		{
			factory = []() { return std::make_shared<T>(); };
		}

		auto type = createTypeForClass<T>(*this, index.name(), index);
		addType(type);
		return type;
	}

	template <typename T>
	TypePtr getOrCreateMostDerivedType(const T& object)
	{
		return getOrCreateType<T>(typeid(object));
	}

private:
	std::map<std::string, TypePtr> mTypesByName;
	std::map<std::type_index, TypePtr> mTypesByTypeIndex;
};

template<typename ValueT>
struct ContainerValueAccessorFactory<std::optional<ValueT>>
{
	static ContainerValueAccessorPtr create(TypeRegistry& registry)
	{
		return std::make_shared<StdOptionalValueAccessorT<ValueT>>(registry.template getOrCreateType<ValueT>()->getName());
	}
};

template<typename ValueT>
struct ContainerValueAccessorFactory<std::vector<ValueT>>
{
    static ContainerValueAccessorPtr create(TypeRegistry& registry)
	{
		return std::make_shared<StdVectorValueAccessorT<ValueT>>(registry.template getOrCreateType<ValueT>()->getName());
	}
};

template <typename T>
inline void null_deleter(T*) {};

template <typename T>
Instance makeRefInstance(TypeRegistry& registry, T* object)
{
	TypePtr type = registry.getOrCreateType<T>();
	TypePtr derivedType = registry.getOrCreateMostDerivedType(*object);
	if (type != derivedType)
	{
		if (auto offset = derivedType->getOffsetFromThisToSuper(type->getTypeIndex()); offset)
		{
			T* derivedPointer = addPointerByteOffset(object, -*offset);
			return Instance(std::shared_ptr<void>(derivedPointer, &null_deleter<void>), derivedType);
		}
	}
	return Instance(std::shared_ptr<T>(object, &null_deleter<T>), type);
}

template <typename T>
Instance makeValueInstance(TypeRegistry& registry, T object)
{
	return Instance(std::make_shared<T>(std::move(object)), registry.getOrCreateType<T>());
}

inline std::unique_ptr<Instance> Type::createDefaultInstance() const
{
	if (mValueFactory)
	{
		return std::make_unique<Instance>(mValueFactory(), const_cast<Type*>(this)->shared_from_this());
	}
	return nullptr;
}

class Property
{
public:
	Property(const std::string& name, const TypePtr& type, bool readOnly) :
		mType(type),
		mName(name),
		mReadOnly(readOnly)
	{}

	virtual ~Property() = default;

	const std::string& getName() const { return mName; }

	const TypePtr& getType() const { return mType; }

	//! @returns true if value was set, or false if property is read-only.
	virtual bool setValue(Instance& obj, const Instance& value) = 0;
	virtual Instance getValue(const Instance& obj) const = 0;

	bool isReadOnly() const { return mReadOnly; }

	using MetadataMap = std::map<std::string, std::any>;
	void addMetadata(const MetadataMap& metadata);
	void addMetadata(const std::string& name, const std::any& value);
	std::any getMetadata(const std::string& name) const;
	const MetadataMap& getMetadataMap() const { return mMetadata; }

	void setCategory(const std::string& category) { mCategory = category; }
	const std::string& getCategory() { return mCategory; }

protected:
	const bool mReadOnly;
	const TypePtr mType;

private:
	std::string mName;
	std::map<std::string, std::any> mMetadata;
	std::string mCategory; //!< Optional name of the category/section to group the property in within the UI
};

template <typename T>
std::optional<T> getOptionalValue(const std::any& v)
{
	if (!v.has_value())
	{
		return std::nullopt;
	}

    try
    {
        return std::any_cast<T>(v);
    }
    catch (const std::bad_any_cast&)
	{
		return std::nullopt;
	}
}

template <typename ObjectT>
using ValueChangedCallbackT = void(ObjectT::*)(const std::string&);

template <typename ObjectT, typename MemberT, bool ReadOnlyT>
class MemberProperty : public Property
{
public:
	using ValueChangedCallback = ValueChangedCallbackT<ObjectT>;

	MemberProperty(TypeRegistry* typeRegistry, const std::string& name, const TypePtr& type, MemberT ObjectT::*member, ValueChangedCallback valueChanged) :
		Property(name, type, ReadOnlyT),
		mTypeRegistry(typeRegistry),
		mMember(member),
		mValueChangedCallback(std::move(valueChanged))
	{
		assert(mTypeRegistry);
	}

	~MemberProperty() override = default;

	bool setValue(Instance& obj, const Instance& value) override
	{
		if constexpr (!ReadOnlyT)
		{
			auto& object = obj.cast<ObjectT>();
			object.*mMember = value.cast<MemberT>();

			if (mValueChangedCallback)
			{
				(object.*mValueChangedCallback)(getName());
			}

			return true;
		}
		return false;
	}

	Instance getValue(const Instance& obj) const override
	{
		const ObjectT& objT = obj.cast<ObjectT>();
		return makeRefInstance(*mTypeRegistry, const_cast<MemberT*>(&(objT.*mMember)));
	}

private:
	TypeRegistry* mTypeRegistry;
	MemberT ObjectT::*mMember;
	ValueChangedCallback mValueChangedCallback = nullptr;
};

template <typename ObjectT, typename GetterValueT, typename SetterValueT>
class GetterSetterMethodProperty : public Property
{
public:
	using ValueChangedCallback = ValueChangedCallbackT<ObjectT>;

	GetterSetterMethodProperty(TypeRegistry* typeRegistry, const std::string& name, const TypePtr& type, GetterValueT (ObjectT::*getter)() const, void (ObjectT::*setter)(SetterValueT), ValueChangedCallback valueChanged) :
		Property(name, type, /* readOnly */ false),
		mTypeRegistry(typeRegistry),
		mGetter(getter),
		mSetter(setter),
		mValueChangedCallback(std::move(valueChanged))
	{
		assert(mTypeRegistry);
	}

	~GetterSetterMethodProperty() override = default;

	bool setValue(Instance& obj, const Instance& value) override
	{
		using UnqualifiedValueT = typename std::remove_cv<typename std::remove_reference<SetterValueT>::type>::type;

		auto& object = obj.cast<ObjectT>();
		(object.*mSetter)(value.cast<UnqualifiedValueT>());

		if (mValueChangedCallback)
		{
			(object.*mValueChangedCallback)(getName());
		}
		return true;
	}

	Instance getValue(const Instance& obj) const override
	{
		return makeValueInstance(*mTypeRegistry, (obj.cast<ObjectT>().*mGetter)());
	}

private:
	TypeRegistry* mTypeRegistry;
	GetterValueT (ObjectT::*mGetter)() const;
	void (ObjectT::*mSetter)(SetterValueT);
	ValueChangedCallback mValueChangedCallback = nullptr;
};

template <typename ObjectT, typename ValueT>
class GetterMethodProperty : public Property
{
public:
	GetterMethodProperty(TypeRegistry* typeRegistry, const std::string& name, const TypePtr& type, ValueT (ObjectT::*getter)() const) :
		Property(name, type, /* readOnly */ true),
		mTypeRegistry(typeRegistry),
		mGetter(getter)
	{
		assert(mTypeRegistry);
	}

	~GetterMethodProperty() override = default;

	bool setValue(Instance& obj, const Instance& value) override
	{
		return false;
	}

	Instance getValue(const Instance& obj) const override
	{
		return makeValueInstance(*mTypeRegistry, (obj.cast<ObjectT>().*mGetter)());
	}

private:
	TypeRegistry* mTypeRegistry;
	ValueT (ObjectT::*mGetter)() const;
};

template <typename ObjectT, typename GetterValueT, typename SetterValueT>
class GetterSetterFunctionProperty : public Property
{
public:
	using GetterFunction = std::function<GetterValueT(const ObjectT&)>;
	using SetterFunction = std::function<void(ObjectT&, SetterValueT)>;
	using ValueChangedCallback = ValueChangedCallbackT<ObjectT>;

	GetterSetterFunctionProperty(TypeRegistry* typeRegistry, const std::string& name, const TypePtr& type, GetterFunction getter, SetterFunction setter, ValueChangedCallback valueChangedCallback) :
		Property(name, type, /* readOnly */ false),
		mTypeRegistry(typeRegistry),
		mGetter(std::move(getter)),
		mSetter(std::move(setter)),
		mValueChangedCallback(std::move(valueChangedCallback))
	{
		assert(mTypeRegistry);
	}

	~GetterSetterFunctionProperty() override = default;

	bool setValue(Instance& obj, const Instance& value) override
	{
		auto& object = obj.cast<ObjectT>();

		using UnqualifiedValueT = typename std::remove_cv<typename std::remove_reference<SetterValueT>::type>::type;
		mSetter(object, value.cast<UnqualifiedValueT>());

		if (mValueChangedCallback)
		{
			(object.*mValueChangedCallback)(getName());
		}

		return true;
	}

	Instance getValue(const Instance& obj) const override
	{
		auto value = mGetter(obj.cast<ObjectT>());
		return makeValueInstance(*mTypeRegistry, value);
	}

private:
	TypeRegistry* mTypeRegistry;
	GetterFunction mGetter;
	SetterFunction mSetter;
	ValueChangedCallback mValueChangedCallback = nullptr;
};

template <typename ObjectT, typename GetterValueT>
class GetterFunctionProperty : public Property
{
public:
	using GetterFunction = std::function<GetterValueT(const ObjectT&)>;
	GetterFunctionProperty(TypeRegistry* typeRegistry, const std::string& name, const TypePtr& type, GetterFunction getter) :
		Property(name, type, /* readOnly */ true),
		mTypeRegistry(typeRegistry),
		mGetter(std::move(getter))
	{
		assert(mTypeRegistry);
	}

	~GetterFunctionProperty() override = default;

	bool setValue(Instance& obj, const Instance& value) override
	{
		return false;
	}

	Instance getValue(const Instance& obj) const override
	{
		auto value = mGetter(obj.cast<ObjectT>());
		return makeValueInstance(*mTypeRegistry, value);
	}

private:
	TypeRegistry* mTypeRegistry;
	GetterFunction mGetter;
};

//! This class provies extra "dynamic" properties for objects, in addition to the "static" properties registered for their type.
//! To use, an object should implement this interface.
class DynamicPropertySource
{
public:
	virtual ~DynamicPropertySource() = default;
	virtual Type::PropertyMap getProperties() const = 0;
};

Type::PropertyMap getProperties(const Instance& obj);

using TypeDefinitionRegistrationHandler = std::function<void(TypeRegistry& registry)>;
using TypeDefinitionRegisterLater = std::function<void(TypeDefinitionRegistrationHandler)>;

template <typename TypeT>
class TypeBuilder
{
public:
	//! Since properties refer to other types, we need to register types before properties.
	//! We achieve this with `registerLater`, which delays registration of properties until
	//! types have been registered.
	TypeBuilder(TypePtr type, TypeDefinitionRegisterLater registerLater) :
		mType(std::move(type)),
		mRegisterLater(std::move(registerLater)),
		mCategory(mType->getName())
	{}

	//! Read/writable property backed by a member variable
	template <typename MemberT>
	TypeBuilder<TypeT>& property(const std::string& name, MemberT TypeT::*member, Property::MetadataMap metadata = {}, ValueChangedCallbackT<TypeT> valueChanged = nullptr)
	{
		mRegisterLater([name = std::move(name), member = std::move(member), metadata = std::move(metadata), type = mType, valueChanged = std::move(valueChanged), categoryName = mCategory] (TypeRegistry& registry) {
			auto p = std::make_shared<MemberProperty<TypeT, MemberT, /* readOnly */ false>>(&registry, name, registry.getOrCreateType<MemberT>(), member, std::move(valueChanged));
			p->addMetadata(std::move(metadata));
			p->setCategory(categoryName);
			type->addProperty(p);
		});
		return *this;
	}

	//! Read only property backed by a member variable
	template <typename MemberT>
	std::enable_if_t<!std::is_member_function_pointer_v<MemberT TypeT::*>, TypeBuilder<TypeT>&>
	propertyReadOnly(const std::string& name, MemberT TypeT::*member, Property::MetadataMap metadata = {})
	{
		mRegisterLater([name = std::move(name), member = std::move(member), metadata = std::move(metadata), type = mType, categoryName = mCategory] (TypeRegistry& registry) {
			auto p = std::make_shared<MemberProperty<TypeT, MemberT, /* readOnly */ true>>(&registry, name, registry.getOrCreateType<MemberT>(), member, /* valueChanged */ nullptr);
			p->addMetadata(std::move(metadata));
			p->setCategory(categoryName);
			type->addProperty(p);
		});
		return *this;
	}

	//! Read/writable property backed by getter and setter methods
	template <typename GetterValueT, typename SetterValueT>
	TypeBuilder<TypeT>& property(const std::string& name, GetterValueT (TypeT::*getter)() const, void (TypeT::*setter)(SetterValueT), Property::MetadataMap metadata = {}, ValueChangedCallbackT<TypeT> valueChanged = nullptr)
	{
		mRegisterLater([name = std::move(name), getter = std::move(getter), setter = std::move(setter), metadata = std::move(metadata), type = mType, categoryName = mCategory, valueChanged = std::move(valueChanged)](TypeRegistry& registry) {
			using UnqualifiedGetterValueT = typename std::remove_cv<typename std::remove_reference<GetterValueT>::type>::type;
			using UnqualifiedSetterValueT = typename std::remove_cv<typename std::remove_reference<SetterValueT>::type>::type;
			static_assert(std::is_same<UnqualifiedGetterValueT, UnqualifiedSetterValueT>::value, "Getter and setter must have same value type, ignoring const ref qualifiers");

			auto p = std::make_shared<GetterSetterMethodProperty<TypeT, GetterValueT, SetterValueT>>(&registry, name, registry.getOrCreateType<UnqualifiedGetterValueT>(), getter, setter, std::move(valueChanged));
			p->addMetadata(std::move(metadata));
			p->setCategory(categoryName);
			type->addProperty(p);
		});
		return *this;
	}

	//! Read only property backed by a getter method
	template <typename MethodT>
	TypeBuilder<TypeT>& propertyReadOnly(const std::string& name, MethodT (TypeT::*getter)() const, Property::MetadataMap metadata = {})
	{
		mRegisterLater([name = std::move(name), getter = std::move(getter), metadata = std::move(metadata), type = mType, categoryName = mCategory] (TypeRegistry& registry) {
			auto p = std::make_shared<GetterMethodProperty<TypeT, MethodT>>(&registry, name, registry.getOrCreateType<MethodT>(), getter);
			p->addMetadata(std::move(metadata));
			p->setCategory(categoryName);
			type->addProperty(p);
		});
		return *this;
	}

	//! Read/writable property backed by getter and setter `std::function`s
	template <typename GetterValueT, typename SetterValueT>
	TypeBuilder<TypeT>& propertyFn(const std::string& name, std::function<GetterValueT(const TypeT&)> getter, std::function<void(TypeT&, SetterValueT)> setter, Property::MetadataMap metadata = {}, ValueChangedCallbackT<TypeT> valueChanged = nullptr)
	{
		mRegisterLater([name = std::move(name), getter = std::move(getter), setter = std::move(setter), metadata = std::move(metadata), type = mType, categoryName = mCategory, valueChanged = std::move(valueChanged)](TypeRegistry& registry) {
			using UnqualifiedGetterValueT = typename std::remove_cv<typename std::remove_reference<GetterValueT>::type>::type;
			using UnqualifiedSetterValueT = typename std::remove_cv<typename std::remove_reference<SetterValueT>::type>::type;
			static_assert(std::is_same<UnqualifiedGetterValueT, UnqualifiedSetterValueT>::value, "Getter and setter must have same value type, ignoring const ref qualifiers");

			auto p = std::make_shared<GetterSetterFunctionProperty<TypeT, GetterValueT, SetterValueT>>(&registry, name, registry.getOrCreateType<UnqualifiedGetterValueT>(), getter, setter, std::move(valueChanged));
			p->addMetadata(std::move(metadata));
			p->setCategory(categoryName);
			type->addProperty(p);
		});
		return *this;
	}

	//! Read only property backed by getter `std::function`
	template <typename GetterValueT>
	TypeBuilder<TypeT>& propertyReadOnlyFn(const std::string& name, std::function<GetterValueT(const TypeT&)> getter, Property::MetadataMap metadata = {})
	{
		mRegisterLater([name = std::move(name), getter = std::move(getter), metadata = std::move(metadata), type = mType, categoryName = mCategory](TypeRegistry& registry) {
			using UnqualifiedGetterValueT = typename std::remove_cv<typename std::remove_reference<GetterValueT>::type>::type;

			auto p = std::make_shared<GetterFunctionProperty<TypeT, GetterValueT>>(&registry, name, registry.getOrCreateType<UnqualifiedGetterValueT>(), getter);
			p->addMetadata(std::move(metadata));
			p->setCategory(categoryName);
			type->addProperty(p);
			});
		return *this;
	}

	template <typename SuperT>
	TypeBuilder<TypeT>& superType()
	{
		mRegisterLater([type = mType] (TypeRegistry& registry) {
			TypeT* derived = reinterpret_cast<TypeT*>(reinterpret_cast<char*>(1 << 16));
			std::ptrdiff_t offset = reinterpret_cast<std::ptrdiff_t>(reinterpret_cast<char*>(static_cast<SuperT*>(derived))) - (1 << 16);
			type->addSuperType(registry.getOrCreateType<SuperT>(), offset);
		});
		return *this;
	}

	TypeBuilder<TypeT>& category(const std::string& categoryName)
	{
		mCategory = categoryName;
		return *this;
	}

private:
	TypePtr mType;
	TypeDefinitionRegisterLater mRegisterLater;
	std::string mCategory; //! Category for future properties to be created under
};

class TypeRegistryBuilder
{
public:
	TypeRegistryBuilder(TypeRegistry& registry, TypeDefinitionRegisterLater registerLater) :
		mRegistry(registry),
		mRegisterLater(std::move(registerLater))
	{}

	template <typename T>
	TypeBuilder<T> type(const std::string& name)
	{
		assert(mRegistry.getType<T>() == nullptr);
		TypePtr newType = createTypeForClass<T>(mRegistry, name, typeid(T));
		mRegistry.addType(newType);
		auto typeBuilder = TypeBuilder<T>(newType, mRegisterLater);
		return typeBuilder;
	}

private:
	TypeRegistry& mRegistry;
	TypeDefinitionRegisterLater mRegisterLater;
};

using RegistrationHandler = std::function<void(TypeRegistryBuilder&)>;

void addStaticRegistrationHandler(RegistrationHandler handler);
void addStaticallyRegisteredTypes(TypeRegistry& registry);


#define SKYBOLT_REFLECT(moduleName) \
	/* Forward declare implementation */ \
    inline void module_impl_##moduleName(skybolt::refl::TypeRegistryBuilder& registry); \
	/* Define struct to execute the user's registration logic on construction */ \
    struct Reg_##moduleName { \
        Reg_##moduleName() { skybolt::refl::addStaticRegistrationHandler(&module_impl_##moduleName); } \
    }; \
    /* This dummy variable forces the struct to be instantiated */ \
    inline void* const use_##moduleName = []() { \
		static Reg_##moduleName instance; \
		return (void*)&instance; \
		}(); \
	/* Start the user's implementation block */ \
    inline void module_impl_##moduleName(skybolt::refl::TypeRegistryBuilder& registry)

/*! SKYBOLT_REFLECT_EXTERN ensures reflection registration defined in a .cpp file is linked into the final
	executable. In C++, if an object is not explicitly used by main(), or in a file included by main, the
	linker will often discard the object as "dead code". This is especially	common when modules are compiled
	into static libraries.
	This macro creates a "pull" mechanism to force the linker to include the registration object. */
#define SKYBOLT_REFLECT_EXTERN(moduleName) \
	extern void* const use_##moduleName; \
	static void* const force_use_##moduleName = []() { \
		return use_##moduleName; \
	}();



#ifdef SKYBOLT_REFLECT_IMPL

void Property::addMetadata(const MetadataMap& metadata)
{
	mMetadata.insert(metadata.begin(), metadata.end());
}

void Property::addMetadata(const std::string& name, const std::any& value)
{
	mMetadata[name] = value;
}

std::any Property::getMetadata(const std::string& name) const
{
	if (auto i = mMetadata.find(name); i != mMetadata.end())
	{
		return i->second;
	}
	return {};
}

void Type::addProperty(const PropertyPtr& property)
{
	mProperties[property->getName()] = property;
}

PropertyPtr Type::getProperty(const std::string& name)
{
	if (auto i = mProperties.find(name); i != mProperties.end())
	{
		return i->second;
	}
	for (const auto& [index, type] : mSuperTypes)
	{
		if (const auto& property = type.first->getProperty(name); property)
		{
			return property;
		}
	}
	return nullptr;
}

Type::PropertyMap Type::getProperties() const
{
	Type::PropertyMap r = mProperties;
	for (const auto& [index, type] : mSuperTypes)
	{
		const auto& superTypeProperties = type.first->getProperties();
		r.insert(superTypeProperties.begin(), superTypeProperties.end());
	}

	return r;
}

void Type::addSuperType(const TypePtr& super, std::ptrdiff_t offsetFromThisToSuper)
{
	mSuperTypes[super->getTypeIndex()] = { super, offsetFromThisToSuper };
}

std::optional<std::ptrdiff_t> Type::getOffsetFromThisToSuper(const std::type_index& super) const
{
	// Look for direct super types
	if (auto i = mSuperTypes.find(super); i != mSuperTypes.end())
	{
		return i->second.second;
	}

	// Walk tree to find indirect super types
	for (const auto& [typeIndex, typeAndOffset] : mSuperTypes)
	{
		if (auto offset = typeAndOffset.first->getOffsetFromThisToSuper(super); offset)
		{
			return typeAndOffset.second + *offset;
		}
	}

	// No super type found
	return std::nullopt;
}

Type::PropertyMap getProperties(const Instance& obj)
{
	const auto& type = obj.getType();
	Type::PropertyMap r = type->getProperties();

	if (type->isDerivedFrom<DynamicPropertySource>())
	{
		const DynamicPropertySource& source = obj.cast<DynamicPropertySource>();
		const auto& properties = source.getProperties();
		r.insert(properties.begin(), properties.end());
	}

	return r;
}

static std::vector<RegistrationHandler>& getGlobalHandlers()
{
	static std::vector<RegistrationHandler> r;
	return r;
}

void addStaticRegistrationHandler(RegistrationHandler handler)
{
	getGlobalHandlers().emplace_back(std::move(handler));
}

void addStaticallyRegisteredTypes(TypeRegistry& registry)
{
	std::vector<TypeDefinitionRegistrationHandler> typeDefinitionRegistrationHandlers;
	TypeRegistryBuilder builder(registry, [&](TypeDefinitionRegistrationHandler registry) {
		typeDefinitionRegistrationHandlers.push_back(registry);
		});

	// Register types
	for (const auto& handler : getGlobalHandlers())
	{
		handler(builder);
	}

	// Perform delayed registration of type definitions. This is performed as a second step
	// so that types are registered for properties to refer to.
	for (const auto& i : typeDefinitionRegistrationHandlers)
	{
		i(registry);
	}
}

TypeRegistry::TypeRegistry()
{
	addStaticallyRegisteredTypes(*this);
}

void TypeRegistry::addType(const TypePtr& type)
{
	mTypesByName[type->getName()] = type;
	mTypesByTypeIndex[type->getTypeIndex()] = type;
}

TypePtr TypeRegistry::getTypeByName(const std::string& name) const
{
	if (auto i = mTypesByName.find(name); i != mTypesByName.end())
	{
		return i->second;
	}
	return nullptr;
}

#endif // SKYBOLT_REFLECT_IMPL

} // namespace skybolt::refl