
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
	void set_script_path(const String &p_path) { _path = p_path; } //because subclasses need a path too...
private:
	
	bool tool;
	bool valid;
	String _source;
	se::Object* _constructor;
	JSLanguage* _language;
	Set<const Object*> _instances;
	String _path;
};
#endif
