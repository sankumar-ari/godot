#include "js_script.h"
#include "js_script_instance.h"
#include "core/os/file_access.h"
#include "js_registrations.h"
JS_Script::JS_Script() : script_list(this), tool(false), valid(false), _constructor()
{
	
	_language = JSLanguage::get_singleton();
	
#ifdef TOOLS_ENABLED
	source_changed_cache = false;
	placeholder_fallback_enabled = false;
#endif

	_resource_path_changed();

#ifdef DEBUG_ENABLED
	{
		
		JSLanguage::get_singleton()->script_list.add(&this->script_list);
	}
#endif

}
JS_Script::~JS_Script()
{
	
#ifdef DEBUG_ENABLED
	
	JSLanguage::get_singleton()->script_list.remove(&this->script_list);
#endif
}
bool JS_Script::can_instance() const
{
#ifdef TOOLS_ENABLED
	return _constructor.isObject() && (tool || ScriptServer::is_scripting_enabled());
#else
	return _constructor.isObject();
#endif
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
	
	se::AutoHandleScope hs;
	if(!_constructor.isObject()) return nullptr;
	JavaScriptInstance* instance = memnew(JavaScriptInstance);
	se::Value ret;
	if(!_constructor.toObject()->CallAsConstructor(se::ValueArray(), &ret) || !ret.isObject()) return nullptr;
	auto obj = ret.toObject();
	auto pdata = reinterpret_cast<Variant*>(obj->getPrivateData());
	if(pdata) delete pdata;
	obj->clearPrivateData();
	obj->setPrivateData(new Variant(p_this));
	
	if(members.empty()) JSRegistrations::get_members(obj, members, methods, properties, signals);
	instance->initialize(ret, p_this, Ref<Script>(this), _language);
	instances.insert(p_this);
	return instance;
}

bool JS_Script::instance_has(const Object * p_this) const
{
	return instances.has(p_this);
}
void JS_Script::remove_instance(Object* owner)
{
	instances.erase(owner);
}
bool JS_Script::has_source_code() const
{
	return !source.empty();
}

String JS_Script::get_source_code() const
{
	return source;
}

void JS_Script::set_source_code(const String & p_code)
{
	if (source == p_code)
		return;
	source = p_code;
#ifdef TOOLS_ENABLED
	source_changed_cache = true;
#endif
}

Error JS_Script::reload(bool p_keep_state)
{
	ERR_FAIL_COND_V(!p_keep_state && instances.size(), ERR_ALREADY_IN_USE);
	valid = false;
	se::Value rval;
		
	se::AutoHandleScope hs;
	if(!se::ScriptEngine::getInstance()->evalString(source.utf8().get_data(), source.length(), &rval, path.utf8().get_data()) || !rval.isObject())
	{
		return ERR_COMPILATION_FAILED;
	}
	
	se::Object* obj;
	if(!rval.isObject()|| !(obj = rval.toObject()) || !obj->isFunction()) 
	{
		return ERR_COMPILATION_FAILED;
	}
	_constructor = rval;
	
	se::Object* ctor_obj = _constructor.toObject();
	
	// se::Value ret;
	// if(!ctor_obj->CallAsConstructor(se::ValueArray(), &ret) || !ret.isObject())
	// { 
	// 	return ERR_COMPILATION_FAILED;
	// }
	JSRegistrations::get_members(ctor_obj, members, methods, properties, signals);
   	_parse_members();
	valid=true;
	return OK;
}
void  JS_Script::_parse_members()
{
}
bool JS_Script::has_method(const StringName & p_method) const
{
	return methods.find(p_method.operator String().utf8().get_data()) != methods.end();
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
	if (signals.find(p_signal.operator String().utf8().get_data())!=signals.end())
		return true;
	// if (base.is_valid()) {
	// 	return base->has_script_signal(p_signal);
	// }
// #ifdef TOOLS_ENABLED
// 	else if (base_cache.is_valid()) {
// 		return base_cache->has_script_signal(p_signal);
// 	}
// #endif
	return false;
}

void JS_Script::get_script_signal_list(List<MethodInfo>* r_signals) const
{
	for(auto&& signal : signals)
	{
		MethodInfo mi;
		mi.name = signal.c_str();
		r_signals->push_back(mi);
	}

}

bool JS_Script::get_property_default_value(const StringName & p_property, Variant & r_value) const
{
#ifdef TOOLS_ENABLED

	const Map<StringName, Variant>::Element *E = member_default_values_cache.find(p_property);
	if (E) {
		r_value = E->get();
		return true;
	}

	if (base_cache.is_valid()) {
		return base_cache->get_property_default_value(p_property, r_value);
	}
#endif
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

	source = s;
#ifdef TOOLS_ENABLED
	source_changed_cache = true;
#endif
	path = p_path;
	return OK;
}


void JS_Script::get_constants(Map<StringName, Variant> *p_constants) {

}

void JS_Script::get_members(Set<StringName> *p_members) {
	if (p_members) {
		for(auto&& member : members) {
			p_members->insert(member.c_str());
		}
	}
}


#ifdef TOOLS_ENABLED
void JS_Script::_update_exports_values(Map<StringName, Variant> &values, List<PropertyInfo> &propnames) {

	// if (base_cache.is_valid()) {
	// 	base_cache->_update_exports_values(values, propnames);
	// }

	// for (Map<StringName, Variant>::Element *E = member_default_values_cache.front(); E; E = E->next()) {
	// 	values[E->key()] = E->get();
	// }

	// for (List<PropertyInfo>::Element *E = members_cache.front(); E; E = E->next()) {
	// 	propnames.push_back(E->get());
	// }
}
#endif

bool JS_Script::_update_exports() {

#ifdef TOOLS_ENABLED

	bool changed = false;

	if (source_changed_cache) {
		source_changed_cache = false;
		changed = true;

		String basedir = path;

		if (basedir == "")
			basedir = get_path();

		if (basedir != "")
			basedir = basedir.get_base_dir();

		// JS_ScriptParser parser;
		// Error err = parser.parse(source, basedir, true, path);

		// if (err == OK) {

		// 	const JS_ScriptParser::Node *root = parser.get_parse_tree();
		// 	ERR_FAIL_COND_V(root->type != JS_ScriptParser::Node::TYPE_CLASS, false);

		// 	const JS_ScriptParser::ClassNode *c = static_cast<const JS_ScriptParser::ClassNode *>(root);

		// 	if (base_cache.is_valid()) {
		// 		base_cache->inheriters_cache.erase(get_instance_id());
		// 		base_cache = Ref<JS_Script>();
		// 	}

		// 	if (c->extends_used) {
		// 		String path = "";
		// 		if (String(c->extends_file) != "" && String(c->extends_file) != get_path()) {
		// 			path = c->extends_file;
		// 			if (path.is_rel_path()) {

		// 				String base = get_path();
		// 				if (base == "" || base.is_rel_path()) {

		// 					ERR_PRINT(("Could not resolve relative path for parent class: " + path).utf8().get_data());
		// 				} else {
		// 					path = base.get_base_dir().plus_file(path);
		// 				}
		// 			}
		// 		} else if (c->extends_class.size() != 0) {
		// 			String base = c->extends_class[0];

		// 			if (ScriptServer::is_global_class(base))
		// 				path = ScriptServer::get_global_class_path(base);
		// 		}

		// 		if (path != "") {
		// 			if (path != get_path()) {

		// 				Ref<JS_Script> bf = ResourceLoader::load(path);

		// 				if (bf.is_valid()) {

		// 					base_cache = bf;
		// 					bf->inheriters_cache.insert(get_instance_id());
		// 				}
		// 			} else {
		// 				ERR_PRINT(("Path extending itself in  " + path).utf8().get_data());
		// 			}
		// 		}
		// 	}

		// 	members_cache.clear();
		// 	member_default_values_cache.clear();

		// 	for (int i = 0; i < c->variables.size(); i++) {
		// 		if (c->variables[i]._export.type == Variant::NIL)
		// 			continue;

		// 		members_cache.push_back(c->variables[i]._export);
		// 		member_default_values_cache[c->variables[i].identifier] = c->variables[i].default_value;
		// 	}

		// 	_signals.clear();

		// 	for (int i = 0; i < c->_signals.size(); i++) {
		// 		_signals[c->_signals[i].name] = c->_signals[i].arguments;
		// 	}
		// } else {
		// 	placeholder_fallback_enabled = true;
		// 	return false;
		// }
	} else if (placeholder_fallback_enabled) {
		return false;
	}

	placeholder_fallback_enabled = false;

	if (base_cache.is_valid()) {
		if (base_cache->_update_exports()) {
			changed = true;
		}
	}

	if (placeholders.size()) { //hm :(

		// update placeholders if any
		Map<StringName, Variant> values;
		List<PropertyInfo> propnames;
		_update_exports_values(values, propnames);

		for (Set<PlaceHolderScriptInstance *>::Element *E = placeholders.front(); E; E = E->next()) {
			E->get()->update(propnames, values);
		}
	}

	return changed;

#else
	return false;
#endif
}
#ifdef TOOLS_ENABLED

void JS_Script::_placeholder_erased(PlaceHolderScriptInstance *p_placeholder) {

	placeholders.erase(p_placeholder);
}
#endif
void JS_Script::update_exports() {

#ifdef TOOLS_ENABLED

	_update_exports();

	Set<ObjectID> copy = inheriters_cache; //might get modified

	for (Set<ObjectID>::Element *E = copy.front(); E; E = E->next()) {
		Object *id = ObjectDB::get_instance(E->get());
		JS_Script *s = Object::cast_to<JS_Script>(id);
		if (!s)
			continue;
		s->update_exports();
	}

#endif
}

