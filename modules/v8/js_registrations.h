
#ifndef JS_REGISTRATIONS_H
#define JS_REGISTRATIONS_H


#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/list.h"
#include "core/script_language.h"
#include "core/ustring.h"
#include "core/vector.h"
#include "jslib/jswrapper/SeApi.h"
class JSRegistrations
{
public:
	JSRegistrations() = delete;
	~JSRegistrations() = default;
	static se::Class* get_registration(const StringName& name);
	static void add_registration(const StringName& name, se::Class* cls);
	static se::Value variant_to_js(const Variant& v);
	static void variant_to_js(const Variant ** p_args, int p_argcount, se::ValueArray* arr);
	static Variant js_to_variant(const se::Value& value);
	static void register_singletons();
	static void register_type_members(const Variant::Type & type, se::Class * cls);
	static void register_type_members(const StringName & type, se::Class * cls);
	static void register_type_members(const StringName & type, se::Object * cls);
	static void register_type_members(const List<MethodInfo>& methods, const List<PropertyInfo>& props, se::Class * cls);
	static void register_type_members(const List<MethodInfo>& methods, const List<PropertyInfo>& props, se::Object * cls);
	static void register_type(const StringName & p_type, se::Class * parent);
	static void register_builtins();
	static void register_all();
};
#endif
