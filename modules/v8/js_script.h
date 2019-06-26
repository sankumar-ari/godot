
#ifndef JS_SCRIPT_H
#define JS_SCRIPT_H

#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/list.h"
#include "core/script_language.h"
#include "core/ustring.h"
#include "core/vector.h"
#include "jslib/jswrapper/SeApi.h"
#include "js_language.h"

class JS_Script : public Script
{
public:
	JS_Script();
	// Inherited via Script
	virtual bool can_instance() const override;
	virtual Ref<Script> get_base_script() const override;
	virtual StringName get_instance_base_type() const override;
	virtual ScriptInstance * instance_create(Object * p_this) override;
	virtual bool instance_has(const Object * p_this) const override;
	virtual bool has_source_code() const override;
	virtual String get_source_code() const override;
	virtual void set_source_code(const String & p_code) override;
	virtual Error reload(bool p_keep_state = false) override;
	virtual bool has_method(const StringName & p_method) const override;
	virtual MethodInfo get_method_info(const StringName & p_method) const override;
	virtual bool is_tool() const override;
	virtual bool is_valid() const override;
	virtual ScriptLanguage * get_language() const override;
	virtual bool has_script_signal(const StringName & p_signal) const override;
	virtual void get_script_signal_list(List<MethodInfo>* r_signals) const override;
	virtual bool get_property_default_value(const StringName & p_property, Variant & r_value) const override;
	virtual void get_script_method_list(List<MethodInfo>* p_list) const override;
	virtual void get_script_property_list(List<PropertyInfo>* p_list) const override;
	void initialize(JSLanguage* language);
	Error load_source_code(const String &p_path);
	void set_script_path(const String &p_path) { path = p_path; } //because subclasses need a path too...

	virtual int get_member_line(const StringName &p_member) const {
#ifdef TOOLS_ENABLED
		if (member_lines.has(p_member))
			return member_lines[p_member];
		else
#endif
			return -1;
	}

	virtual void get_constants(Map<StringName, Variant> *p_constants);
	virtual void get_members(Set<StringName> *p_members);

#ifdef TOOLS_ENABLED
	virtual bool is_placeholder_fallback_enabled() const { return placeholder_fallback_enabled; }
#endif

	virtual void update_exports();
private:
	
#ifdef TOOLS_ENABLED

	Map<StringName, int> member_lines;

	Map<StringName, Variant> member_default_values;

	List<PropertyInfo> members_cache;
	Map<StringName, Variant> member_default_values_cache;
	Ref<JS_Script> base_cache;
	Set<ObjectID> inheriters_cache;
	bool source_changed_cache;
	bool placeholder_fallback_enabled;
	void _update_exports_values(Map<StringName, Variant> &values, List<PropertyInfo> &propnames);

	Set<PlaceHolderScriptInstance *> placeholders;
	//void _update_placeholder(PlaceHolderScriptInstance *p_placeholder);
	virtual void _placeholder_erased(PlaceHolderScriptInstance *p_placeholder);

#endif
#ifdef DEBUG_ENABLED

	Map<ObjectID, List<Pair<StringName, Variant> > > pending_reload_state;

#endif

	bool _update_exports();
	
	Set<StringName> members; //members are just indices to the instanced script.
	Map<StringName, Variant> constants;
	// Map<StringName, GDScriptFunction *> member_functions;
	// Map<StringName, MemberInfo> member_indices; //members are just indices to the instanced script.
	// Map<StringName, Ref<GDScript> > subclasses;
	Map<StringName, Vector<StringName> > _signals;

	bool tool;
	bool valid;
	String source;
	se::Value _constructor;
	JSLanguage* _language;
	Set<const Object*> _instances;
	String path;
};
#endif
