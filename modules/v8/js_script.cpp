#include "js_script.h"
#include "js_script_instance.h"
#include "core/os/file_access.h"
JS_Script::JS_Script() : tool(false), valid(false), _constructor(nullptr)
{
	
	_language = JSLanguage::get_singleton();
}

bool JS_Script::can_instance() const
{
	return  _constructor && (!tool && !ScriptServer::is_scripting_enabled());
}

Ref<Script> JS_Script::get_base_script() const
{
	return Ref<Script>();
}

StringName JS_Script::get_instance_base_type() const
{
	return StringName();
}

ScriptInstance * JS_Script::instance_create(Object * p_this)
{
	if(!_constructor) return nullptr;
	JavaScriptInstance* instance = memnew(JavaScriptInstance);
	se::Value ret;
	if(!_constructor->CallAsConstructor(se::ValueArray(), &ret) || !ret.isObject()) return nullptr;

	instance->initialize(ret.toObject(), p_this, Ref<Script>(this), _language);
	_instances.insert(p_this);
	return instance;
}

bool JS_Script::instance_has(const Object * p_this) const
{
	return _instances.has(p_this);
}

bool JS_Script::has_source_code() const
{
	return !_source.empty();
}

String JS_Script::get_source_code() const
{
	return _source;
}

void JS_Script::set_source_code(const String & p_code)
{
	_source = p_code;
}

Error JS_Script::reload(bool p_keep_state)
{
	valid = false;
	se::Value rval;
	if(!se::ScriptEngine::getInstance()->evalString(_source.utf8().get_data(), _source.length(), &rval, "script.js") || !rval.isObject())
	{
		return ERR_COMPILATION_FAILED;
	}

	auto obj = rval.toObject();
	se::Value exports;
	if(!obj->getProperty("exports", &exports) || !exports.isObject() || !exports.toObject()->isFunction()) 
	{
		return ERR_COMPILATION_FAILED;
	}
	_constructor = exports.toObject();

	return OK;
}

bool JS_Script::has_method(const StringName & p_method) const
{
	//if(_constructor->getAllKeys())
	return false;
}

MethodInfo JS_Script::get_method_info(const StringName & p_method) const
{
	return MethodInfo();
}

bool JS_Script::is_tool() const
{
	return tool;
}

bool JS_Script::is_valid() const
{
	return valid;
}

ScriptLanguage * JS_Script::get_language() const
{
	return _language;
}

bool JS_Script::has_script_signal(const StringName & p_signal) const
{
	return false;
}

void JS_Script::get_script_signal_list(List<MethodInfo>* r_signals) const
{
}

bool JS_Script::get_property_default_value(const StringName & p_property, Variant & r_value) const
{
	return false;
}

void JS_Script::get_script_method_list(List<MethodInfo>* p_list) const
{
}

void JS_Script::get_script_property_list(List<PropertyInfo>* p_list) const
{
}

void JS_Script::initialize(JSLanguage * language)
{
	_language = language;
}
Error JS_Script::load_source_code(const String &p_path) 
{

	PoolVector<uint8_t> sourcef;
	Error err;
	FileAccess *f = FileAccess::open(p_path, FileAccess::READ, &err);
	if (err) {

		ERR_FAIL_COND_V(err, err);
	}

	int len = f->get_len();
	sourcef.resize(len + 1);
	PoolVector<uint8_t>::Write w = sourcef.write();
	int r = f->get_buffer(w.ptr(), len);
	f->close();
	memdelete(f);
	ERR_FAIL_COND_V(r != len, ERR_CANT_OPEN);
	w[len] = 0;

	String s;
	if (s.parse_utf8((const char *)w.ptr())) {

		ERR_EXPLAIN("Script '" + p_path + "' contains invalid unicode (utf-8), so it was not loaded. Please ensure that scripts are saved in valid utf-8 unicode.");
		ERR_FAIL_V(ERR_INVALID_DATA);
	}

	_source = s;
	_path = p_path;
	return OK;
}