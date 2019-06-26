#include "js_script_instance.h"
#include "js_registrations.h"
JavaScriptInstance::JavaScriptInstance()
{
}
bool JavaScriptInstance::set(const StringName & p_name, const Variant & p_value)
{
	return _object.toObject()->setProperty(p_name.operator String().utf8().get_data(), JSRegistrations::variant_to_js(p_value));
}

bool JavaScriptInstance::get(const StringName & p_name, Variant & r_ret) const
{
	se::Value value;
	if(!_object.toObject()->getProperty(p_name.operator String().utf8().get_data(), &value)) return false;
	r_ret = JSRegistrations::js_to_variant(value);
	return true;
}

void JavaScriptInstance::get_property_list(List<PropertyInfo>* p_properties) const
{
}

Variant::Type JavaScriptInstance::get_property_type(const StringName & p_name, bool * r_is_valid) const
{
	return Variant::Type();
}

void JavaScriptInstance::get_method_list(List<MethodInfo>* p_list) const
{
}

bool JavaScriptInstance::has_method(const StringName & p_method) const
{
	return false;
}

Variant JavaScriptInstance::call(const StringName & p_method, const Variant ** p_args, int p_argcount, Variant::CallError & r_error)
{
	
	se::AutoHandleScope hs;
	if ("_unhandled_input" == p_method) 
	{
		r_error.error = Variant::CallError::CALL_OK;
		return Variant();
	}
	se::ValueArray varr;
	JSRegistrations::variant_to_js(p_args, p_argcount, &varr);
	se::Value fn;
	se::Object* fn_object;
	if(!_object.toObject()->getProperty(p_method.operator String().utf8().get_data(), &fn) || !fn.isObject() || !(fn_object = fn.toObject())->isFunction())
	{
		r_error.error = Variant::CallError::CALL_ERROR_INVALID_METHOD;
		return Variant();
	}
	se::Value rval;
	if(!fn_object->call(varr, _object.toObject(), &rval))
	{
		r_error.error = Variant::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS;
		return Variant();
	}
	return JSRegistrations::js_to_variant(rval);
}

void JavaScriptInstance::notification(int p_notification)
{
}

Ref<Script> JavaScriptInstance::get_script() const
{
	return _script;
}

MultiplayerAPI::RPCMode JavaScriptInstance::get_rpc_mode(const StringName & p_method) const
{
	return MultiplayerAPI::RPC_MODE_DISABLED;
}

MultiplayerAPI::RPCMode JavaScriptInstance::get_rset_mode(const StringName & p_variable) const
{
	return MultiplayerAPI::RPC_MODE_DISABLED;
}

ScriptLanguage * JavaScriptInstance::get_language()
{
	return _language;
}

void JavaScriptInstance::initialize(se::Value& jsobj, Object * owner, Ref<JS_Script> script, JSLanguage * language)
{
	_object = jsobj;
	_owner = owner;
	_script = script;
	_language = language;
}
