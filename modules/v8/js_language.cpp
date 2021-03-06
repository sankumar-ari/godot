/*************************************************************************/
/*  js_language.cpp                                                      */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                    http://www.godotengine.org                         */
/*************************************************************************/
/* Copyright (c) 2007-2016 Juan Linietsky, Ariel Manzur.                 */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "js_language.h"
#include "core/class_db.h"
#include "core/core_string_names.h"
#include "core/global_constants.h"
#include "core/io/file_access_encrypted.h"
#include "core/os/file_access.h"
#include "core/os/os.h"
#include "js/global.js.h"
#include "js_functions.h"

#include "debugger/inspector_agent.h"
#include "libplatform/libplatform.h"
#include "v8.h"
JavaScriptLanguage *JavaScriptLanguage::singleton = NULL;

void JavaScriptLanguage::_add_class(const StringName &p_type, const v8::Local<v8::FunctionTemplate> &p_parent) {

	String type(p_type);
	// Ignore proxy classes and singletons
	//if (type.begins_with("_")) return;
	if (Engine::get_singleton()->has_singleton(type)) return;

	// Change object name to avoid conflict with V8JavaScript's own Object
	if (p_type == "Object") type = String("GodotObject");

	v8::Isolate::Scope isolate_scope(isolate);
	v8::HandleScope handle_scope(isolate);
	v8::EscapableHandleScope escapable_scope(isolate);

	v8::Local<v8::FunctionTemplate> local_constructor = v8::FunctionTemplate::New(isolate, Bindings::js_constructor);
	local_constructor->SetClassName(v8::String::NewFromUtf8(isolate, type.utf8().get_data()));
	local_constructor->InstanceTemplate()->SetInternalFieldCount(2);

	v8::Local<v8::ObjectTemplate> templ = local_constructor->PrototypeTemplate();

	List<MethodInfo> methods;
	ClassDB::get_method_list(p_type, &methods, true);

	for (List<MethodInfo>::Element *E = methods.front(); E; E = E->next()) {
		if (E->get().flags & (METHOD_FLAG_VIRTUAL | METHOD_FLAG_NOSCRIPT)) continue;
		v8::Local<v8::String> method_name = v8::String::NewFromUtf8(isolate, E->get().name.utf8().get_data());
		v8::Local<v8::FunctionTemplate> method = v8::FunctionTemplate::New(isolate, Bindings::js_method, method_name);
		local_constructor->PrototypeTemplate()->Set(method_name, method);
	}

	local_constructor = escapable_scope.Escape(local_constructor);

	if (!p_parent.IsEmpty()) {
		local_constructor->Inherit(p_parent);
	}

	global_template.Get(isolate)->Set(v8::String::NewFromUtf8(isolate, type.utf8().get_data()), local_constructor, v8::PropertyAttribute::ReadOnly);

	List<StringName> sub_types;
	ClassDB::get_inheriters_from_class(p_type, &sub_types);

	for (List<StringName>::Element *E = sub_types.front(); E; E = E->next()) {
		StringName parent = ClassDB::get_parent_class_nocheck(E->get());
		if (parent != p_type) continue;
		_add_class(E->get(), local_constructor);
	}
}

void JavaScriptLanguage::_add_builtin_type(Variant::Type p_type) {

	String type_name = Variant::get_type_name(p_type);

	Variant::CallError cerror;
	Variant v = Variant::construct(p_type, NULL, 0, cerror);
	v8::Local<v8::String> v8_type_name = v8::String::NewFromUtf8(isolate, type_name.utf8().get_data());
	v8::Local<v8::FunctionTemplate>
			constructor = v8::FunctionTemplate::New(isolate, Bindings::js_builtin_constructor, v8_type_name);
	constructor->SetClassName(v8_type_name);
	constructor->InstanceTemplate()->SetInternalFieldCount(2);

	v8::Local<v8::ObjectTemplate> prototype = constructor->PrototypeTemplate();

	List<MethodInfo> methods;
	v.get_method_list(&methods);

	for (List<MethodInfo>::Element *E = methods.front(); E; E = E->next()) {
		v8::Local<v8::String> name = v8::String::NewFromUtf8(isolate, E->get().name.utf8().get_data());
		prototype->Set(
				name,
				v8::FunctionTemplate::New(isolate, Bindings::js_builtin_method, name),
				v8::ReadOnly);
	}

	// Add operators
	static const char *operators[]{
		"add",
		"subtract",
		"divide",
		"multiply",
		"negate",
		"is_equal",
		"is_less",
		"is_less_or_equal",
		"is_greater",
		"is_greater_or_equal",
		NULL
	};
	for (int i = 0; operators[i] != NULL; i++) {
		v8::Local<v8::String> name = v8::String::NewFromUtf8(isolate, operators[i]);
		prototype->Set(
				name,
				v8::FunctionTemplate::New(isolate, Bindings::js_builtin_op, name),
				v8::ReadOnly);
	}

	List<PropertyInfo> properties;
	v.get_property_list(&properties);
	for (List<PropertyInfo>::Element *E = properties.front(); E; E = E->next()) {
		prototype->SetAccessor(
				v8::String::NewFromUtf8(isolate, E->get().name.utf8().get_data()),
				Bindings::js_builtin_getter, Bindings::js_builtin_setter);
	}

	global_template.Get(isolate)->Set(v8::String::NewFromUtf8(isolate, type_name.utf8().get_data()), constructor, v8::ReadOnly);
}

void JavaScriptLanguage::init() {

	// Initialize V8.
	v8::V8::InitializeICUDefaultLocation(OS::get_singleton()->get_executable_path().utf8().get_data());
	v8::V8::InitializeExternalStartupData(OS::get_singleton()->get_executable_path().utf8().get_data());
	platform = v8::platform::CreateDefaultPlatform();
	v8::V8::InitializePlatform(platform);
	v8::V8::Initialize();

	// Create the isolate
	create_params.array_buffer_allocator = &allocator;

	isolate = v8::Isolate::New(create_params);
	isolate->Enter();

	v8::Isolate::Scope isolate_scope(isolate);
	v8::HandleScope scope(isolate);
	//v8::EscapableHandleScope escapable_scope(isolate);

	// Global template
	v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate, v8::FunctionTemplate::New(isolate, Bindings::js_constructor));

	// Add global functions
	global->Set(v8::String::NewFromUtf8(isolate, "print"), v8::FunctionTemplate::New(isolate, JavaScriptFunctions::print), v8::PropertyAttribute::ReadOnly);
	global->Set(v8::String::NewFromUtf8(isolate, "require"), v8::FunctionTemplate::New(isolate, JavaScriptFunctions::require), v8::PropertyAttribute::ReadOnly);

	global_template.Set(isolate, global);
	v8::Local<v8::ObjectTemplate> shallow = v8::ObjectTemplate::New(isolate);

	// Register type bindings in a tree fashion
	_add_class(StringName("Object"));

	// Register singletons
	List<StringName> types;
	List<Engine::Singleton> singletons;
	Engine::get_singleton()->get_singletons(&singletons);

	for (List<Engine::Singleton>::Element *E = singletons.front(); E; E = E->next()) {

		String singleton_name = E->get().name;

		v8::EscapableHandleScope singleton_scope(isolate);

		v8::Local<v8::FunctionTemplate> singleton_constructor = v8::FunctionTemplate::New(isolate);
		singleton_constructor->SetClassName(v8::String::NewFromUtf8(isolate, singleton_name.utf8().get_data()));

		v8::Local<v8::ObjectTemplate> singleton = v8::ObjectTemplate::New(isolate, singleton_constructor);

		List<MethodInfo> methods;
		E->get().ptr->get_method_list(&methods);

		for (List<MethodInfo>::Element *E = methods.front(); E; E = E->next()) {
			if (E->get().flags & (METHOD_FLAG_VIRTUAL | METHOD_FLAG_NOSCRIPT)) continue;
			v8::Local<v8::String> method_name = v8::String::NewFromUtf8(isolate, E->get().name.utf8().get_data());
			v8::Local<v8::FunctionTemplate>
					method = v8::FunctionTemplate::New(isolate, Bindings::js_singleton_method, method_name);
			singleton->Set(method_name, method);
		}

		global_template.Get(isolate)->Set(v8::String::NewFromUtf8(isolate, singleton_name.utf8().get_data()), singleton_scope.Escape(singleton), v8::PropertyAttribute::ReadOnly);
	}

	static const Variant::Type builtin_types[] = {
		// Math types
		Variant::VECTOR2,
		Variant::RECT2,
		Variant::VECTOR3,
		// Variant::MATRIX32,
		Variant::PLANE,
		Variant::QUAT,
		// Variant::_AABB,
		// Variant::MATRIX3,
		Variant::TRANSFORM,
		// Misc types
		Variant::COLOR,
		// Variant::IMAGE,
		Variant::NODE_PATH,
		Variant::_RID,
		// Variant::INPUT_EVENT,
		Variant::NIL
	};

	for (int i = 0; builtin_types[i] != Variant::NIL; i++) {
		_add_builtin_type(builtin_types[i]);
	}

	// Create JS globals
	v8::EscapableHandleScope global_scope(isolate);
	v8::Local<v8::Context> context = v8::Context::New(isolate);
	v8::Context::Scope context_scope(context);

	v8::TryCatch trycatch(isolate);

	v8::Local<v8::String> global_js = v8::String::NewFromUtf8(isolate, javascript_global_module);
	v8::ScriptCompiler::Source global_js_src(global_js);
	v8::Local<v8::UnboundScript> global_js_script = v8::ScriptCompiler::CompileUnboundScript(isolate, &global_js_src).ToLocalChecked();
	global_script.Set(isolate, global_scope.Escape(global_js_script));

	if (trycatch.HasCaught()) {
		v8::String::Utf8Value ex(isolate, trycatch.Exception());
		ERR_PRINT(*ex);
	}
	//inspector::Agent *agent = new inspector::Agent("localhost", "c:/temp/frontend.url");
	//agent->Start(isolate, platform, nullptr);
}

void JavaScriptLanguage::finish() {

	// using namespace v8;
	// Locker locker(isolate);
	// v8::Isolate::Scope isolate_scope(isolate);
	// v8::HandleScope handle_scole(isolate);
	// isolate->Dispose();
	// V8::Dispose();
	// V8::ShutdownPlatform();
	// delete platform;
}

String JavaScriptLanguage::get_type() const {

	return "V8JavaScript";
}

String JavaScriptLanguage::get_extension() const {

	return "js";
}

Error JavaScriptLanguage::execute_file(const String &p_path) {

	return OK;
}

void JavaScriptLanguage::add_global_constant(const StringName &p_variable, const Variant &p_value) {}

void JavaScriptLanguage::reload_all_scripts() {}

void JavaScriptLanguage::reload_tool_script(const Ref<Script> &p_script, bool p_soft_reload) {}

void JavaScriptLanguage::profiling_start() {}

void JavaScriptLanguage::profiling_stop() {}

int JavaScriptLanguage::profiling_get_accumulated_data(ProfilingInfo *p_info_arr, int p_info_max) {
	return 0;
}

int JavaScriptLanguage::profiling_get_frame_data(ProfilingInfo *p_info_arr, int p_info_max) {
	return 0;
}

void JavaScriptLanguage::frame() {}

JavaScriptLanguage::JavaScriptLanguage() {

	ERR_FAIL_COND(singleton);
	singleton = this;
}

JavaScriptLanguage::~JavaScriptLanguage() {

	singleton = NULL;
}

void V8JavaScript::_update_exports() {
}

void V8JavaScript::_bind_methods() {}

bool V8JavaScript::can_instance() const {

	return compiled || (!tool && !ScriptServer::is_scripting_enabled());
}

Variant V8JavaScript::call(const StringName &p_method, const Variant **p_args, int p_argcount, Variant::CallError &r_error) {
	return Script::call(p_method, p_args, p_argcount, r_error);
}

Ref<Script> V8JavaScript::get_base_script() const {
	return Ref<Script>();
}

StringName V8JavaScript::get_instance_base_type() const {
	return StringName();
}

ScriptInstance *V8JavaScript::instance_create(Object *p_this) {

	if (!tool && !ScriptServer::is_scripting_enabled()) {

#ifdef TOOLS_ENABLED

		PlaceHolderScriptInstance *si = memnew(PlaceHolderScriptInstance(JavaScriptLanguage::get_singleton(), Ref<Script>(this), p_this));
		placeholders.insert(si);
		_update_exports();
		return si;
#else
		return NULL;
#endif
	}

	ERR_FAIL_COND_V(!compiled, NULL);

	JavaScriptInstance *instance = memnew(JavaScriptInstance);

	instance->owner = p_this;
	instance->script = Ref<V8JavaScript>(this);

	instance->_run();

	instances.insert(p_this);

	return instance;
}

bool V8JavaScript::instance_has(const Object *p_this) const {

	return instances.has((Object *)p_this);
}

bool V8JavaScript::has_source_code() const {

	return !source.empty();
}

String V8JavaScript::get_source_code() const {

	return source;
}

void V8JavaScript::set_source_code(const String &p_code) {

	if (source == p_code)
		return;
	source = p_code;
}

Error V8JavaScript::reload(bool p_keep_state) {

	ERR_FAIL_COND_V(!p_keep_state && instances.size(), ERR_ALREADY_IN_USE);

	compiled = false;

	if (!has_source_code()) {
		return ERR_COMPILATION_FAILED;
	}

	v8::Isolate *isolate = JavaScriptLanguage::get_singleton()->isolate;
	v8::Isolate::Scope isolate_scope(isolate);
	v8::HandleScope handle_scope(isolate);

	v8::Local<v8::Context> ctx = v8::Context::New(isolate, NULL, JavaScriptLanguage::get_singleton()->global_template.Get(isolate));

	v8::Context::Scope scope(ctx);
	ctx->Global()->Set(v8::String::NewFromUtf8(isolate, "__filename"), v8::String::NewFromUtf8(isolate, get_path().utf8().get_data()));
	ctx->Global()->Set(v8::String::NewFromUtf8(isolate, "__dirname"), v8::String::NewFromUtf8(isolate, get_path().get_base_dir().utf8().get_data()));

	v8::Local<v8::String> source =
			v8::String::NewFromUtf8(
					isolate,
					get_source_code().utf8().get_data(),
					v8::NewStringType::kNormal)
					.ToLocalChecked();

	if (origin) {
		memdelete(origin);
	}
	origin = memnew(v8::ScriptOrigin(v8::String::NewFromUtf8(isolate, path.utf8().get_data())));

	v8::TryCatch trycatch(isolate);

	v8::Local<v8::UnboundScript> global_script = JavaScriptLanguage::get_singleton()->global_script.Get(isolate);
	v8::Local<v8::Script> global_js_bound_script = global_script->BindToCurrentContext();
	global_js_bound_script->Run(ctx);

	if (trycatch.HasCaught()) {
		v8::String::Utf8Value ex(isolate, trycatch.Exception());
		ERR_PRINT(*ex);
	}

	v8::MaybeLocal<v8::Script> script = v8::Script::Compile(ctx, source, origin);

	if (script.IsEmpty()) {
		return ERR_COMPILATION_FAILED;
	}

	v8::MaybeLocal<v8::Value> temp_result = script.ToLocalChecked()->Run(ctx);

	if (temp_result.IsEmpty()) {
		v8::Local<v8::Value> exception = trycatch.Exception();
		v8::String::Utf8Value exception_str(isolate, exception->ToDetailString(ctx).ToLocalChecked());
		print_line(*exception_str);
		return ERR_COMPILATION_FAILED;
	}

	v8::Local<v8::Value> result = temp_result.ToLocalChecked();
	v8::MaybeLocal<v8::Value> exported = ctx->Global()->Get(ctx, v8::String::NewFromUtf8(isolate, "exports"));

	if (exported.IsEmpty() || !exported.ToLocalChecked()->IsFunction()) {
		if (trycatch.HasCaught()) {
			v8::Local<v8::Value> exception = trycatch.Exception();
			v8::String::Utf8Value exception_str(isolate, exception->ToDetailString(ctx).ToLocalChecked());
			print_line(*exception_str);
		}
		return ERR_COMPILATION_FAILED;
	}

	constructor = v8::Persistent<v8::Function, v8::CopyablePersistentTraits<v8::Function> >(isolate, v8::Local<v8::Function>::Cast(exported.ToLocalChecked()));

	compiled = true;

	return OK;
}

bool V8JavaScript::has_method(const StringName &p_method) const {
	return false;
}

MethodInfo V8JavaScript::get_method_info(const StringName &p_method) const {
	return MethodInfo();
}

bool V8JavaScript::is_tool() const {
	return tool;
}

String V8JavaScript::get_node_type() const {
	return String();
}

ScriptLanguage *V8JavaScript::get_language() const {

	return JavaScriptLanguage::get_singleton();
}

bool V8JavaScript::has_script_signal(const StringName &p_signal) const {
	return false;
}

void V8JavaScript::get_script_signal_list(List<MethodInfo> *r_signals) const {}

bool V8JavaScript::get_property_default_value(const StringName &p_property, Variant &r_value) const {
	return false;
}

void V8JavaScript::get_script_method_list(List<MethodInfo> *p_list) const {}

void V8JavaScript::get_script_property_list(List<PropertyInfo> *p_list) const {

	p_list->push_back(PropertyInfo(Variant::STRING, "script/source", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR));
}

Error V8JavaScript::load_source_code(const String &p_path) {

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
	path = p_path;
	return OK;
}

V8JavaScript::V8JavaScript() {
	compiled = false;
	tool = false;
	origin = NULL;
}

V8JavaScript::~V8JavaScript() {
	if (!constructor.IsEmpty())
		constructor.SetWeak();
}

RES ResourceFormatLoaderJavaScript::load(const String &p_path, const String &p_original_path, Error *r_error) {

	if (r_error)
		*r_error = ERR_FILE_CANT_OPEN;

	V8JavaScript *script = memnew(V8JavaScript);

	Ref<V8JavaScript> scriptres(script);

	Error err = script->load_source_code(p_path);

	if (err != OK) {

		ERR_FAIL_COND_V(err != OK, RES());
	}

	script->set_path(p_original_path);
	script->set_name(p_path.get_file());

	script->reload();

	if (r_error)
		*r_error = OK;

	return scriptres;
}

void ResourceFormatLoaderJavaScript::get_recognized_extensions(List<String> *p_extensions) const {

	p_extensions->push_back("js");
}

bool ResourceFormatLoaderJavaScript::handles_type(const String &p_type) const {

	return (p_type == "Script" || p_type == "V8JavaScript");
}

String ResourceFormatLoaderJavaScript::get_resource_type(const String &p_path) const {

	if (p_path.get_extension().to_lower() == "js") {
		return "V8JavaScript";
	}
	return "";
}

Error ResourceFormatSaverJavaScript::save(const String &p_path, const RES &p_resource, uint32_t p_flags) {

	Ref<V8JavaScript> sqscr = p_resource;
	ERR_FAIL_COND_V(sqscr.is_null(), ERR_INVALID_PARAMETER);

	String source = sqscr->get_source_code();

	Error err;
	FileAccess *file = FileAccess::open(p_path, FileAccess::WRITE, &err);

	if (err) {

		ERR_FAIL_COND_V(err, err);
	}

	file->store_string(source);
	if (file->get_error() != OK && file->get_error() != ERR_FILE_EOF) {
		memdelete(file);
		return ERR_CANT_CREATE;
	}
	file->close();
	memdelete(file);

	if (ScriptServer::is_reload_scripts_on_save_enabled()) {
		JavaScriptLanguage::get_singleton()->reload_tool_script(p_resource, false);
	}

	return OK;
}

void ResourceFormatSaverJavaScript::get_recognized_extensions(const RES &p_resource, List<String> *p_extensions) const {

	if (Object::cast_to<V8JavaScript>(p_resource.ptr()) != NULL) {
		p_extensions->push_back("js");
	}
}

bool ResourceFormatSaverJavaScript::recognize(const RES &p_resource) const {

	return (Object::cast_to<V8JavaScript>(p_resource.ptr()) != NULL);
}

void JavaScriptInstance::_run() {

	v8::Isolate::Scope isolate_scope(isolate);
	v8::HandleScope scope(isolate);
	v8::EscapableHandleScope handle_scope(isolate);

	// Create a context for this instance
	v8::Local<v8::Context> local_context = v8::Context::New(isolate, NULL, JavaScriptLanguage::get_singleton()->global_template.Get(isolate));
	v8::Context::Scope local_context_scope(local_context);

	v8::Context::Scope context_scope(local_context);

	v8::TryCatch trycatch(isolate);

	v8::Local<v8::Function> cons = script->constructor.Get(isolate);
	v8::Local<v8::Value> args[] = { v8::External::New(isolate, owner) };
	v8::MaybeLocal<v8::Value> maybe_inst = cons->CallAsConstructor(local_context, 1, args);

	if (trycatch.HasCaught()) {
		v8::String::Utf8Value e(isolate, trycatch.Exception()->ToDetailString(local_context).ToLocalChecked());
		ERR_EXPLAIN(String(*e));
		ERR_FAIL();
	}

	if (maybe_inst.IsEmpty()) return;

	v8::Local<v8::Object> inst = v8::Local<v8::Object>::Cast(maybe_inst.ToLocalChecked());

	instance = v8::Persistent<v8::Object, v8::CopyablePersistentTraits<v8::Object> >(isolate, inst);

	context = v8::Persistent<v8::Context, v8::CopyablePersistentTraits<v8::Context> >(isolate, handle_scope.Escape(local_context));
	compiled = true;
}

bool JavaScriptInstance::set(const StringName &p_name, const Variant &p_value) {
	return false;
}

bool JavaScriptInstance::get(const StringName &p_name, Variant &r_ret) const {
	return false;
}

void JavaScriptInstance::get_property_list(List<PropertyInfo> *p_properties) const {}

Variant::Type JavaScriptInstance::get_property_type(const StringName &p_name, bool *r_is_valid) const {
	return Variant::Type();
}

void JavaScriptInstance::get_property_state(List<Pair<StringName, Variant> > &state) {}

void JavaScriptInstance::get_method_list(List<MethodInfo> *p_list) const {

	MethodInfo m;
	m.name = "_ready";
	p_list->push_back(m);
}

bool JavaScriptInstance::has_method(const StringName &p_method) const {
	return true;
}

Variant JavaScriptInstance::call(const StringName &p_method, const Variant **p_args, int p_argcount, Variant::CallError &r_error) {
	if ("_unhandled_input" == p_method) {
		r_error.error = Variant::CallError::CALL_OK;
		return Variant();
	}
	if (!compiled) {

		r_error.error = Variant::CallError::CALL_ERROR_INSTANCE_IS_NULL;
		return Variant();
	}
	v8::Isolate::Scope isolate_scope(isolate);
	v8::HandleScope handle_scole(isolate);

	v8::Local<v8::Context> ctx = context.Get(isolate);
	v8::Local<v8::Object> inst = instance.Get(isolate);

	String m = p_method;

	v8::MaybeLocal<v8::Value> func_val = inst->Get(ctx, v8::String::NewFromUtf8(isolate, m.utf8().get_data()));

	if (func_val.IsEmpty() || !func_val.ToLocalChecked()->IsFunction()) {
		r_error.error = Variant::CallError::CALL_ERROR_INVALID_METHOD;
		return Variant();
	}
	v8::String::Utf8Value name(isolate, inst->GetConstructorName());
	WARN_PRINT(*name);
	WARN_PRINT(m.utf8().get_data());
	v8::Local<v8::Value> args[20];
	for (int i = 0; i < p_argcount && i < 20; i++) {
		args[i] = JavaScriptFunctions::variant_to_js(isolate, *(p_args[i]));
	}

	v8::TryCatch trycatch(isolate);
	v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(func_val.ToLocalChecked());
	v8::MaybeLocal<v8::Value> func_result = func->CallAsFunction(ctx, inst, p_argcount, args);

	if (func_result.IsEmpty()) {
		if (trycatch.HasCaught()) {
			v8::String::Utf8Value ex(isolate, trycatch.Exception()->ToString(ctx).ToLocalChecked());
			ERR_PRINT(*ex);
		}
		r_error.error = Variant::CallError::CALL_ERROR_INVALID_METHOD;
		return Variant();
	}

	r_error.error = Variant::CallError::CALL_OK;
	return JavaScriptFunctions::js_to_variant(isolate, func_result.ToLocalChecked());
}

void JavaScriptInstance::call_multilevel(const StringName &p_method, const Variant **p_args, int p_argcount) {

	Variant::CallError err;
	call(p_method, p_args, p_argcount, err);
}

void JavaScriptInstance::call_multilevel_reversed(const StringName &p_method, const Variant **p_args, int p_argcount) {

	Variant::CallError err;
	call(p_method, p_args, p_argcount, err);
}

void JavaScriptInstance::notification(int p_notification) {}

Ref<Script> JavaScriptInstance::get_script() const {

	return script;
}

MultiplayerAPI::RPCMode JavaScriptInstance::get_rpc_mode(const StringName &p_method) const {
	return MultiplayerAPI::RPC_MODE_DISABLED;
}

MultiplayerAPI::RPCMode JavaScriptInstance::get_rset_mode(const StringName &p_variable) const {
	return MultiplayerAPI::RPC_MODE_DISABLED;
}

ScriptLanguage *JavaScriptInstance::get_language() {

	return JavaScriptLanguage::get_singleton();
}

JavaScriptInstance::JavaScriptInstance() {

	isolate = JavaScriptLanguage::get_singleton()->isolate;
	compiled = false;
}

JavaScriptInstance::~JavaScriptInstance() {
	if (!context.IsEmpty())
		context.SetWeak();
	if (!instance.IsEmpty())
		instance.SetWeak();
}

void JavaScriptLanguage::Bindings::js_constructor(const v8::FunctionCallbackInfo<v8::Value> &p_args) {
	auto isolate = p_args.GetIsolate();
	if (!p_args.IsConstructCall()) {
		p_args.GetIsolate()->ThrowException(v8::String::NewFromUtf8(isolate, "Can't call type as a function"));
		return;
	}

	v8::String::Utf8Value c(isolate, p_args.This()->GetConstructorName());
	String class_name(*c);
	WARN_PRINT(class_name.utf8().get_data());

	if (p_args.Length() == 1) {
		p_args.This()->SetInternalField(0, p_args[0]);
		WARN_PRINT("constructed internal");
	} else if (ClassDB::can_instance(class_name)) {
		Object *obj = ClassDB::instance(class_name);
		p_args.This()
				->SetInternalField(0, v8::External::New(isolate, obj));
		WARN_PRINT("constructed and set");
	} else {
		WARN_PRINT("constructed and not set");
	}
	// Set the object as JS created by default
	p_args.This()
			->SetInternalField(1, v8::Boolean::New(isolate, true));
	WARN_PRINT("constructed ");
}

void JavaScriptLanguage::Bindings::js_method(const v8::FunctionCallbackInfo<v8::Value> &p_args) {

	v8::Isolate *isolate = p_args.GetIsolate();

	if (p_args.This()->InternalFieldCount() != 2) {
		isolate->ThrowException(v8::String::NewFromUtf8(isolate, "Invalid V8JavaScript object"));
		return;
	}

	Object *obj = JavaScriptFunctions::unwrap_object(p_args.This());

	if (!obj) {
		isolate->ThrowException(v8::String::NewFromUtf8(isolate, "Empty V8JavaScript object"));
		return;
	}

	v8::String::Utf8Value m(isolate, p_args.Data());
	String method_name(*m);

	Vector<Variant *> args;
	args.resize(p_args.Length());
	for (int i = 0; i < args.size(); i++) {
		args.set(i, new Variant(JavaScriptFunctions::js_to_variant(isolate, p_args[i])));
	}

	Variant::CallError err;

	// If the object is external to V8JavaScript, just use variant get/call
	if (v8::Local<v8::Boolean>::Cast(p_args.This()->GetInternalField(1))->IsFalse()) {
		Variant result = obj->call(method_name, (const Variant **)args.ptr(), args.size(), err);

		if (err.error == Variant::CallError::CALL_OK) {
			p_args.GetReturnValue().Set(JavaScriptFunctions::variant_to_js(isolate, result));
		} else {
			isolate->ThrowException(v8::String::NewFromUtf8(isolate, "Error calling method"));
		}

		// Internal object, looks for the property/function internally
	} else {
		MethodBind *method = ClassDB::get_method(obj->get_class(), method_name);
		if (method) {
			Variant result = method->call(obj, (const Variant **)args.ptr(), args.size(), err);

			if (err.error == Variant::CallError::CALL_OK) {
				p_args.GetReturnValue().Set(JavaScriptFunctions::variant_to_js(isolate, result));
			} else {
				isolate->ThrowException(v8::String::NewFromUtf8(isolate, "Error calling method"));
			}
		}
	}
}

void JavaScriptLanguage::Bindings::js_singleton_method(const v8::FunctionCallbackInfo<v8::Value> &p_args) {
	v8::Isolate *isolate = p_args.GetIsolate();
	v8::String::Utf8Value c(isolate, p_args.This()->GetConstructorName());
	String singleton_name(*c);

	v8::String::Utf8Value p(isolate, p_args.Data());
	String prop(*p);

	Object *singleton = Engine::get_singleton()->get_singleton_object(singleton_name);

	Array args;
	args.resize(p_args.Length());

	for (int i = 0; i < p_args.Length(); i++) {
		args[i] = JavaScriptFunctions::js_to_variant(p_args.GetIsolate(), p_args[i]);
	}

	Variant result = singleton->callv(prop, args);
	p_args.GetReturnValue().Set(JavaScriptFunctions::variant_to_js(p_args.GetIsolate(), result));
}

void JavaScriptLanguage::Bindings::js_builtin_constructor(const v8::FunctionCallbackInfo<v8::Value> &p_args) {

	v8::Isolate *isolate = p_args.GetIsolate();

	if (!p_args.IsConstructCall()) {
		isolate->ThrowException(v8::String::NewFromUtf8(isolate, "Can't call type as a function"));
		return;
	}

	v8::String::Utf8Value t(isolate, p_args.Data());
	String type_name(*t);
	Variant::Type type = JavaScriptFunctions::type_from_string(type_name);

	Vector<Variant *> args;
	args.resize(p_args.Length());
	for (int i = 0; i < p_args.Length(); i++) {
		Variant *arg = new Variant(JavaScriptFunctions::js_to_variant(isolate, p_args[i]));
		args.set(i, arg);
	}

	Variant::CallError err;
	Variant result = Variant::construct(type, (const Variant **)args.ptr(), args.size(), err);

	if (err.error != Variant::CallError::CALL_OK) {
		isolate->ThrowException(v8::String::NewFromUtf8(isolate, ("Error calling constructor " + type_name).utf8().get_data()));
		return;
	}

	p_args.This()->SetInternalField(0, v8::External::New(isolate, new Variant(result)));
	p_args.This()->SetInternalField(1, v8::Integer::New(isolate, int(type)));
}

void JavaScriptLanguage::Bindings::js_builtin_method(const v8::FunctionCallbackInfo<v8::Value> &p_args) {

	v8::Isolate *isolate = p_args.GetIsolate();

	if (p_args.IsConstructCall()) {
		isolate->ThrowException(v8::String::NewFromUtf8(isolate, "Can't call function as constructor"));
		return;
	}

	void *ptr = v8::Local<v8::External>::Cast(p_args.This()->GetInternalField(0))->Value();
	Variant *obj = static_cast<Variant *>(ptr);
	Variant::Type type = Variant::Type(v8::Local<v8::Integer>::Cast(p_args.This()->GetInternalField(1))->IntegerValue(isolate->GetCurrentContext()).ToChecked());

	v8::String::Utf8Value method_name(isolate, p_args.Data());

	Vector<Variant *> args;
	args.resize(p_args.Length());
	for (int i = 0; i < p_args.Length(); i++) {
		args.set(i, &JavaScriptFunctions::js_to_variant(isolate, p_args[i]));
	}

	Variant::CallError err;
	Variant result = obj->call(*method_name, (const Variant **)args.ptr(), args.size(), err);

	if (err.error != Variant::CallError::CALL_OK) {
		isolate->ThrowException(v8::String::NewFromUtf8(isolate, "Error calling method"));
		return;
	}

	p_args.GetReturnValue().Set(JavaScriptFunctions::variant_to_js(isolate, result));
}

void JavaScriptLanguage::Bindings::js_builtin_getter(v8::Local<v8::Name> p_prop, const v8::PropertyCallbackInfo<v8::Value> &p_args) {

	v8::String::Utf8Value prop(p_args.GetIsolate(), p_prop);

	Variant *obj = static_cast<Variant *>(v8::Local<v8::External>::Cast(p_args.This()->GetInternalField(0))->Value());

	if (Variant::has_constant(obj->get_type(), *prop)) {
		int result = Variant::get_constant_value(obj->get_type(), *prop);
		p_args.GetReturnValue().Set(result);
		return;
	}

	bool valid = false;
	Variant result = obj->get(String(*prop), &valid);

	if (valid) {
		p_args.GetReturnValue().Set(JavaScriptFunctions::variant_to_js(p_args.GetIsolate(), result));
	}
}

void JavaScriptLanguage::Bindings::js_builtin_setter(v8::Local<v8::Name> p_prop, v8::Local<v8::Value> p_value, const v8::PropertyCallbackInfo<void> &p_args) {

	v8::String::Utf8Value prop(p_args.GetIsolate(), p_prop);

	Variant *obj = static_cast<Variant *>(v8::Local<v8::External>::Cast(p_args.This()->GetInternalField(0))->Value());

	bool valid = false;
	obj->set(*prop, JavaScriptFunctions::js_to_variant(p_args.GetIsolate(), p_value), &valid);

	if (valid) {
		p_args.GetReturnValue().Set(p_value);
	}
}

void JavaScriptLanguage::Bindings::js_builtin_op(const v8::FunctionCallbackInfo<v8::Value> &p_args) {

	v8::Isolate *isolate = p_args.GetIsolate();

	if (p_args.IsConstructCall()) {
		isolate->ThrowException(v8::String::NewFromUtf8(isolate, "Can't call function as constructor"));
		return;
	}

	v8::String::Utf8Value method_name_char(isolate, p_args.Data());
	String method_name(*method_name_char);

	Variant *var = JavaScriptFunctions::unwrap_variant(p_args.This());

	// TODO: separate this into one function per operator?
	if (method_name == "add") {
		Variant result = Variant::evaluate(Variant::OP_ADD, *var, JavaScriptFunctions::js_to_variant(isolate, p_args[0]));
		p_args.GetReturnValue().Set(JavaScriptFunctions::variant_to_js(isolate, result));
	} else if (method_name == "subtract") {
		Variant result = Variant::evaluate(Variant::OP_SUBTRACT, *var, JavaScriptFunctions::js_to_variant(isolate, p_args[0]));
		p_args.GetReturnValue().Set(JavaScriptFunctions::variant_to_js(isolate, result));
	} else if (method_name == "divide") {
		Variant result = Variant::evaluate(Variant::OP_DIVIDE, *var, JavaScriptFunctions::js_to_variant(isolate, p_args[0]));
		p_args.GetReturnValue().Set(JavaScriptFunctions::variant_to_js(isolate, result));
	} else if (method_name == "multiply") {
		Variant result = Variant::evaluate(Variant::OP_MULTIPLY, *var, JavaScriptFunctions::js_to_variant(isolate, p_args[0]));
		p_args.GetReturnValue().Set(JavaScriptFunctions::variant_to_js(isolate, result));
	} else if (method_name == "negate") {
		Variant result = Variant::evaluate(Variant::OP_NEGATE, *var, Variant());
		p_args.GetReturnValue().Set(JavaScriptFunctions::variant_to_js(isolate, result));
	} else if (method_name == "is_equal") {
		Variant result = Variant::evaluate(Variant::OP_EQUAL, *var, JavaScriptFunctions::js_to_variant(isolate, p_args[0]));
		p_args.GetReturnValue().Set(JavaScriptFunctions::variant_to_js(isolate, result));
	} else if (method_name == "is_less") {
		Variant result = Variant::evaluate(Variant::OP_LESS, *var, JavaScriptFunctions::js_to_variant(isolate, p_args[0]));
		p_args.GetReturnValue().Set(JavaScriptFunctions::variant_to_js(isolate, result));
	} else if (method_name == "is_less_or_equal") {
		Variant result = Variant::evaluate(Variant::OP_LESS_EQUAL, *var, JavaScriptFunctions::js_to_variant(isolate, p_args[0]));
		p_args.GetReturnValue().Set(JavaScriptFunctions::variant_to_js(isolate, result));
	} else if (method_name == "is_greater") {
		Variant result = Variant::evaluate(Variant::OP_GREATER, *var, JavaScriptFunctions::js_to_variant(isolate, p_args[0]));
		p_args.GetReturnValue().Set(JavaScriptFunctions::variant_to_js(isolate, result));
	} else if (method_name == "is_greater_or_equal") {
		Variant result = Variant::evaluate(Variant::OP_GREATER_EQUAL, *var, JavaScriptFunctions::js_to_variant(isolate, p_args[0]));
		p_args.GetReturnValue().Set(JavaScriptFunctions::variant_to_js(isolate, result));
	}
}
