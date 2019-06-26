
#ifndef JS_LANGUAGE_H
#define JS_LANGUAGE_H

#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/list.h"
#include "core/script_language.h"
#include "core/ustring.h"
#include "core/vector.h"
class JSLanguage : public ScriptLanguage
{
public:

	JSLanguage();
	virtual ~JSLanguage();
	// Inherited via ScriptLanguage
	virtual String get_name() const override;

	virtual void init() override;

	virtual String get_type() const override;

	virtual String get_extension() const override;

	virtual Error execute_file(const String & p_path) override;

	virtual void finish() override;

	virtual void get_reserved_words(List<String>* p_words) const override;

	virtual void get_comment_delimiters(List<String>* p_delimiters) const override;

	virtual void get_string_delimiters(List<String>* p_delimiters) const override;

	virtual Ref<Script> get_template(const String & p_class_name, const String & p_base_class_name) const override;

	virtual bool validate(const String & p_script, int & r_line_error, int & r_col_error, String & r_test_error, const String & p_path = "", List<String>* r_functions = NULL, List<Warning>* r_warnings = NULL, Set<int>* r_safe_lines = NULL) const override;

	virtual Script * create_script() const override;

	virtual bool has_named_classes() const override;

	virtual bool supports_builtin_mode() const override;

	virtual int find_function(const String & p_function, const String & p_code) const override;

	virtual String make_function(const String & p_class, const String & p_name, const PoolStringArray & p_args) const override;

	virtual void auto_indent_code(String & p_code, int p_from_line, int p_to_line) const override;

	virtual void add_global_constant(const StringName & p_variable, const Variant & p_value) override;

	virtual String debug_get_error() const override;

	virtual int debug_get_stack_level_count() const override;

	virtual int debug_get_stack_level_line(int p_level) const override;

	virtual String debug_get_stack_level_function(int p_level) const override;

	virtual String debug_get_stack_level_source(int p_level) const override;

	virtual void debug_get_stack_level_locals(int p_level, List<String>* p_locals, List<Variant>* p_values, int p_max_subitems = -1, int p_max_depth = -1) override;

	virtual void debug_get_stack_level_members(int p_level, List<String>* p_members, List<Variant>* p_values, int p_max_subitems = -1, int p_max_depth = -1) override;

	virtual void debug_get_globals(List<String>* p_globals, List<Variant>* p_values, int p_max_subitems = -1, int p_max_depth = -1) override;

	virtual String debug_parse_stack_level_expression(int p_level, const String & p_expression, int p_max_subitems = -1, int p_max_depth = -1) override;

	virtual void reload_all_scripts() override;

	virtual void reload_tool_script(const Ref<Script>& p_script, bool p_soft_reload) override;

	virtual void get_recognized_extensions(List<String>* p_extensions) const override;

	virtual void get_public_functions(List<MethodInfo>* p_functions) const override;

	virtual void get_public_constants(List<Pair<String, Variant>>* p_constants) const override;

	virtual void profiling_start() override;

	virtual void profiling_stop() override;

	virtual int profiling_get_accumulated_data(ProfilingInfo * p_info_arr, int p_info_max) override;

	virtual int profiling_get_frame_data(ProfilingInfo * p_info_arr, int p_info_max) override;
	_FORCE_INLINE_ static JSLanguage *get_singleton() { return singleton; }
private:
	static JSLanguage *JSLanguage::singleton;

};
#endif
