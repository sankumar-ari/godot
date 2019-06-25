
#ifndef JS_SCRIPT_INSTANCE_H
#define JS_SCRIPT_INSTANCE_H

#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/list.h"
#include "core/script_language.h"
#include "core/ustring.h"
#include "core/vector.h"
#include "jslib/jswrapper/SeApi.h"
#include "js_script.h"
#include "js_language.h"
class JavaScriptInstance : public ScriptInstance 
{
public:
	JavaScriptInstance();
	// Inherited via ScriptInstance
	virtual bool set(const StringName & p_name, const Variant & p_value) override;
	virtual bool get(const StringName & p_name, Variant & r_ret) const override;
	virtual void get_property_list(List<PropertyInfo>* p_properties) const override;
	virtual Variant::Type get_property_type(const StringName & p_name, bool * r_is_valid = NULL) const override;
	virtual void get_method_list(List<MethodInfo>* p_list) const override;
	virtual bool has_method(const StringName & p_method) const override;
	virtual Variant call(const StringName & p_method, const Variant ** p_args, int p_argcount, Variant::CallError & r_error) override;
	virtual void notification(int p_notification) override;
	virtual Ref<Script> get_script() const override;
	virtual MultiplayerAPI::RPCMode get_rpc_mode(const StringName & p_method) const override;
	virtual MultiplayerAPI::RPCMode get_rset_mode(const StringName & p_variable) const override;
	virtual ScriptLanguage * get_language() override;
	void initialize(se::Object* jsobj, Object* owner, Ref<JS_Script> script, JSLanguage* language);
private:
	se::Object* _object;
	Object* _owner;
	Ref<JS_Script> _script;
	JSLanguage* _language;
};
#endif
