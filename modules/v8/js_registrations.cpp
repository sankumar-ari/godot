#include "js_registrations.h"
#include <unordered_map>
#include "core/engine.h"

static std::unordered_map<std::string, se::Class*> registrations_;
static std::unordered_map<std::string, se::Object*> singletons_;
se::Class * JSRegistrations::get_registration(const StringName & name)
{
	auto it = registrations_.find(name.operator String().utf8().get_data());
	if (it != registrations_.end()) return it->second;
	return nullptr;
}

void JSRegistrations::add_registration(const StringName & name, se::Class * cls)
{
	registrations_[name.operator String().utf8().get_data()] = cls;
}

se::Value JSRegistrations::variant_to_js(const Variant & p_value)
{
	auto type = p_value.get_type();
	switch (type)
	{
	case Variant::NIL:
		return se::Value();
	case Variant::BOOL:
		return se::Value(bool(p_value));
	case Variant::INT:
		return se::Value(int(p_value));
	case Variant::REAL:
		return se::Value(double(p_value));
	case Variant::STRING:
		return se::Value(String(p_value).utf8());
	case Variant::OBJECT:
	{
		Object* obj = (Object*)p_value;
		if (p_value.is_ref())
		{
			auto reg_class = get_registration(obj->get_class_name());
			if (reg_class)
			{
				auto js_obj = se::Object::createObjectWithClass(reg_class);
				js_obj->setPrivateData(new Variant(p_value));
				return se::Value(js_obj);
			}
		}
	}
	break;
	case Variant::VECTOR2:
	case Variant::RECT2:
	case Variant::VECTOR3:
	case Variant::TRANSFORM2D:
	case Variant::PLANE:
	case Variant::QUAT:
	case Variant::AABB:
	case Variant::BASIS:
	case Variant::TRANSFORM:
	case Variant::COLOR:
	case Variant::NODE_PATH:
	case Variant::_RID:
	case Variant::DICTIONARY:
	{
		auto reg_class = get_registration(Variant::get_type_name(p_value.get_type()));
		if (reg_class)
		{
			auto js_obj = se::Object::createObjectWithClass(reg_class);
			js_obj->setPrivateData(new Variant(p_value));
			return se::Value(js_obj);
		}
	}
	break;
	case Variant::POOL_BYTE_ARRAY:
	case Variant::POOL_INT_ARRAY:
	case Variant::POOL_REAL_ARRAY:
	case Variant::POOL_STRING_ARRAY:
	case Variant::POOL_VECTOR2_ARRAY:
	case Variant::POOL_VECTOR3_ARRAY:
	case Variant::POOL_COLOR_ARRAY:
	case Variant::VARIANT_MAX:
	case Variant::ARRAY:
	{
		auto arr = p_value.operator Array();
		auto ret_obj = se::Object::createArrayObject(arr.size());
		for (int i = 0; i < arr.size(); i++)
		{
			ret_obj->setArrayElement(i, variant_to_js(arr[i]));
		}
		return se::Value(ret_obj);
	}
	break;
	default:
		break;
	}
	return se::Value();
}

void JSRegistrations::variant_to_js(const Variant ** p_args, int p_argcount, se::ValueArray* arr)
{

	for (int i = 0; i < p_argcount; i++)
	{
		arr->push_back(variant_to_js(*p_args[i]));
	}
}

void JSRegistrations::js_to_variant(const se::ValueArray& values, Vector<Variant>& out_values, Vector<const Variant*>& out_valuesp)
{
	out_values.resize(values.size());
	out_valuesp.resize(values.size());
	int i = 0;
	for (size_t i = 0; i < values.size(); i++)
	{
		out_values.write[i] = js_to_variant(values[i]);
		out_valuesp.write[i] = &out_values[i];
	}
	
}
Variant JSRegistrations::js_to_variant(const se::Value& value)
{
	auto type = value.getType();
	switch (type)
	{
	case se::Value::Type::Undefined:
	case se::Value::Type::Null:
		return Variant();
		break;
	case se::Value::Type::Number:
		return Variant(value.toInt32());
		break;
	case se::Value::Type::Boolean:
		return Variant(value.toBoolean());
		break;
	case se::Value::Type::String:
		return Variant(value.toString().c_str());
		break;
	case se::Value::Type::Object:
	{
		auto obj = value.toObject();
		auto private_data = obj->getPrivateData();
		if (private_data) {
			return *(reinterpret_cast<Variant*>(private_data));
		}
		if (obj->isArray())
		{
			Array arr;
			uint32_t length;
			obj->getArrayLength(&length);
			for (uint32_t i = 0; i < length; i++)
			{
				se::Value data;
				obj->getArrayElement(i, &data);
				arr.append(js_to_variant(data));
			}
			return Variant(arr);
		}
	}
	break;
	default:
		break;
	}

	return Variant();
}

static bool CreatClassObject(se::State& state)
{
	String name(state.data().toString().c_str());
	if (!ClassDB::can_instance(name)) return false;
	auto instance = ClassDB::instance(name);

	state.thisObject()->setPrivateData(new Variant(instance));
	return true;
}
static bool DestrocyClassObject(se::State& state)
{
	auto obj = static_cast<Variant*>(state.nativeThisObject());
	delete obj;
	return true;
}

static bool FunctionCall(se::State& state)
{
	auto obj = static_cast<Variant*>(state.nativeThisObject());
	if (!obj) return false;
	String name(state.data().toString().c_str());
	auto&& js_args = state.args();
	Vector<Variant> variant_args;
	Vector<const Variant*> variant_argsp;
	JSRegistrations::js_to_variant(js_args, variant_args, variant_argsp);
	
	Variant::CallError r_error;
	Variant result;
	result = obj->call(name, (const Variant **)variant_argsp.ptr(), variant_argsp.size(), r_error);

	if (r_error.error == Variant::CallError::CALL_OK)
	{
		state.rval() = JSRegistrations::variant_to_js(result);
	}
	else
	{
		return false;
	}
	return true;
}

static bool PropertySet(se::State& state)
{
	auto obj = static_cast<Variant*>(state.nativeThisObject());
	if (!obj) return false;
	String name(state.data().toString().c_str());
	auto&& js_args = state.args();
	auto variant_arg = JSRegistrations::js_to_variant(js_args[0]);
	bool r_valid = false;
	obj->set_named(name, variant_arg, &r_valid);

	if (!r_valid)
	{
		return false;
	}
	return true;
}
static bool PropertyGet(se::State& state)
{
	auto obj = static_cast<Variant*>(state.nativeThisObject());
	if (!obj) return false;
	String name(state.data().toString().c_str());
	bool r_valid = false;
	auto result = obj->get_named(name, &r_valid);

	if (!r_valid)
	{
		return false;
	}
	state.rval() = JSRegistrations::variant_to_js(result);
	return true;
}

SE_DECLARE_FINALIZE_FUNC(DestrocyClassObject)
SE_BIND_CTOR(CreatClassObject, nullptr, DestrocyClassObject);
SE_BIND_FINALIZE_FUNC(DestrocyClassObject);
SE_BIND_FUNC(FunctionCall);
SE_BIND_PROP_GET(PropertyGet);
SE_BIND_PROP_SET(PropertySet);

void JSRegistrations::register_singletons()
{
	List<Engine::Singleton> singletons;
	Engine::get_singleton()->get_singletons(&singletons);
	auto global_obj = se::ScriptEngine::getInstance()->getGlobalObject();
	for (List<Engine::Singleton>::Element *E = singletons.front(); E; E = E->next())
	{
		String name = E->get().name;
		auto obj = se::Object::createPlainObject();
		register_type_members(E->get().name, obj);
		global_obj->setProperty(name.utf8().get_data(), se::Value(obj));
		singletons_[name.utf8().get_data()] = obj;
	}

}

void JSRegistrations::register_type_members(const Variant::Type& type, se::Class* cls)
{
	Variant::CallError r_error;
	auto var = Variant::construct(type, nullptr, 0, r_error);
	List<MethodInfo> methods;
	var.get_method_list(&methods);
	List<PropertyInfo> props;
	var.get_property_list(&props);
	register_type_members(methods, props, cls);
}
void JSRegistrations::register_type_members(const StringName& type, se::Class* cls)
{
	List<MethodInfo> methods;
	ClassDB::get_method_list(type, &methods, true, true);
	List<PropertyInfo> props;
	ClassDB::get_property_list(type, &props, true);
	register_type_members(methods, props, cls);
}
void JSRegistrations::register_type_members(const StringName& type, se::Object* cls)
{

	List<MethodInfo> methods;
	ClassDB::get_method_list(type, &methods, true, true);
	List<PropertyInfo> props;
	ClassDB::get_property_list(type, &props, true);
	register_type_members(methods, props, cls);

}
void JSRegistrations::register_type_members(const List<MethodInfo>& methods, const List<PropertyInfo>& props, se::Class* cls)
{
	for (const List<MethodInfo>::Element *E = methods.front(); E; E = E->next())
	{
		if (E->get().flags & (METHOD_FLAG_VIRTUAL | METHOD_FLAG_NOSCRIPT)) continue;
		cls->defineFunction(E->get().name.utf8().get_data(), _SE(FunctionCall));
	}
	for (const List<PropertyInfo>::Element *E = props.front(); E; E = E->next())
	{
		if (E->get().usage & (PROPERTY_USAGE_GROUP | PROPERTY_USAGE_CATEGORY)) continue;
		cls->defineProperty(E->get().name.utf8().get_data(), _SE(PropertyGet), _SE(PropertySet));
	}
}
void JSRegistrations::register_type_members(const List<MethodInfo>& methods, const List<PropertyInfo>& props, se::Object* cls)
{
	for (const List<MethodInfo>::Element *E = methods.front(); E; E = E->next())
	{
		if (E->get().flags & (METHOD_FLAG_VIRTUAL | METHOD_FLAG_NOSCRIPT)) continue;
		cls->defineFunction(E->get().name.utf8().get_data(), _SE(FunctionCall));
	}
	for (const List<PropertyInfo>::Element *E = props.front(); E; E = E->next())
	{
		if (E->get().usage & (PROPERTY_USAGE_GROUP | PROPERTY_USAGE_CATEGORY)) continue;
		cls->defineProperty(E->get().name.utf8().get_data(), _SE(PropertyGet), _SE(PropertySet));
	}
}
void JSRegistrations::register_type(const StringName &p_type, se::Class* parent)
{

	String type(p_type);
	if (Engine::get_singleton()->has_singleton(type)) return;
	if (p_type == "Object") type = String("GodotObject");
	auto cls = se::Class::create(type.utf8().get_data(), se::ScriptEngine::getInstance()->getGlobalObject(), parent?parent->getProto():nullptr, _SE(CreatClassObject));
	register_type_members(type, cls);
	cls->install();
	registrations_[type.utf8().get_data()] = cls;

	List<StringName> sub_types;
	ClassDB::get_direct_inheriters_from_class(p_type, &sub_types);

	for (List<StringName>::Element *E = sub_types.front(); E; E = E->next()) {
		StringName parent = ClassDB::get_parent_class_nocheck(E->get());
		if (parent != p_type) continue;
		register_type(E->get(), cls);
	}
}

void JSRegistrations::register_builtins()
{
	auto types = { Variant::VECTOR2,
	Variant::RECT2,
	Variant::VECTOR3,
	Variant::TRANSFORM2D,
	Variant::PLANE,
	Variant::QUAT,
	Variant::AABB,
	Variant::BASIS,
	Variant::TRANSFORM,
	Variant::COLOR,
	Variant::NODE_PATH,
	Variant::_RID,
		Variant::DICTIONARY };
	for (auto&& type : types)
	{
		auto name = Variant::get_type_name(type);
		auto cls = se::Class::create(name.utf8().get_data(), se::ScriptEngine::getInstance()->getGlobalObject(), nullptr, _SE(CreatClassObject));
		register_type_members(type, cls);
		registrations_[name.utf8().get_data()] = cls;
		cls->install();
	}
}
void JSRegistrations::register_all()
{
	register_type("Object", nullptr);
	register_singletons();
	register_builtins();
}

void JSRegistrations::get_members(se::Object* obj, std::set<std::string>& members, std::set<std::string>& methods, 
	std::set<std::string>& properties, std::set<std::string>& signals)
{
	members.clear();
	methods.clear();
	properties.clear();
	signals.clear();
	se::Value rval;
	auto gobj = se::ScriptEngine::getInstance()->getGlobalObject();
	gobj->getProperty("Object", &rval);
	se::Value proto;
	obj->getProperty("prototype", &proto);
	auto protoObj = proto.toObject();

	se::Value propNames;
	se::Value value;
	rval.toObject()->getProperty("getOwnPropertyNames", &propNames);
	

	se::ValueArray args;
	args.push_back(proto);
	se::Value rvalue;
	propNames.toObject()->call(args, nullptr, &rvalue);
	auto namesObj = rvalue.toObject();
	uint32_t len;
	namesObj->getArrayLength(&len);
	for(uint32_t i = 0; i < len; i++)
	{
		se::Value elem;
		namesObj->getArrayElement(i, &elem);
		auto key = elem.toString();
		
		members.insert(key);
		se::Value key_value;
		protoObj->getProperty(key.c_str(), &key_value);
		if(key_value.isObject() && key_value.toObject()->isFunction())
		{
			methods.insert(key);
		}
		else if(key == "signals" && key_value.isObject())
		{
			auto signal_array = key_value.toObject();
			if(signal_array->isArray())
			{
				uint32_t length;
				signal_array->getArrayLength(&length);
				for(uint32_t i = 0; i < length; i++)
				{
					se::Value signal;
					signal_array->getArrayElement(i, &signal);
					signals.insert(signal.toStringForce());
				}
			}
		}
		else
		{
			properties.insert(key);
		}
		
	}
}