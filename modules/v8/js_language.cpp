#include "js_language.h"
#include "js_script.h"
#include "jslib/jswrapper/SeApi.h"
JSLanguage *JSLanguage::singleton = NULL;
JSLanguage::JSLanguage()
{
	singleton = this;
	se::ScriptEngine* se = se::ScriptEngine::getInstance();
	se->
}
String JSLanguage::get_name() const
{
	return "ECMAScript";
}

void JSLanguage::init()
{
}

String JSLanguage::get_type() const
{
	return "JSLanguage";
}

String JSLanguage::get_extension() const
{
	return "js";
}

Error JSLanguage::execute_file(const String & p_path)
{
	return Error();
}

void JSLanguage::finish()
{
}

void JSLanguage::get_reserved_words(List<String>* p_words) const
{
	static const char *_reserved_words[] = { "abstract", "arguments", "await*", "boolean", "break", "byte", "case", "catch", 
		"char", "class*", "const", "continue", "debugger", "default", "delete", "do", "double", "else", "enum*", "eval", 
		"export*", "extends*", "false", "final", "finally", "float", "for", "function", "goto", "if", "implements", 
		"import*", "in", "instanceof", "int", "interface", "let*", "long", "native", "new", "null", "package", "private", 
		"protected", "public", "return", "short", "static", "super*", "switch", "synchronized", "this", "throw", "throws", 
		"transient", "true", "try", "typeof", "var", "void", "volatile", "while", "with", "yield", 0 };
	
	const char **w = _reserved_words;

	while (*w) {
		p_words->push_back(*w);
		w++;
	}
}

void JSLanguage::get_comment_delimiters(List<String>* p_delimiters) const
{
	p_delimiters->push_back("//"); // single-line comment
	p_delimiters->push_back("/* */"); // delimited comment
}

void JSLanguage::get_string_delimiters(List<String>* p_delimiters) const
{
	p_delimiters->push_back("' '"); // character literal
	p_delimiters->push_back("\" \""); // regular string literal

}

static String get_base_class_name(const String &p_base_class_name, const String p_class_name) {

	String base_class = p_base_class_name;
	if (p_class_name == base_class) {
		base_class = "Godot." + base_class;
	}
	return base_class;
}

Ref<Script> JSLanguage::get_template(const String & p_class_name, const String & p_base_class_name) const
{
	String script_template = ""
"class %CLASS% extends %BASE% {\n"
"\n"
" constructor() {\n"
"   // this.name = name;\n"
" }\n"
"\n"
" _ready() {\n"
"   \n"
" }\n"
"\n"
"}\n"
"exports = User";
	String base_class_name = get_base_class_name(p_base_class_name, p_class_name);
	script_template = script_template.replace("%BASE%", base_class_name)
							  .replace("%CLASS%", p_class_name);

	Ref<JS_Script> script;
	script.instance();
	script->set_source_code(script_template);
	script->set_name(p_class_name);

	return script;
}

bool JSLanguage::validate(const String & p_script, int & r_line_error, int & r_col_error, String & r_test_error, 
	const String & p_path/* = "" */, List<String>* r_functions /* = NULL */, List<Warning>* r_warnings /*= NULL*/, 
	Set<int>* r_safe_lines /* = NULL */) const
{
	return false;
}

Script * JSLanguage::create_script() const
{
	auto script =  memnew(JS_Script);
	script->initialize(const_cast<JSLanguage*>(this));
	return script;
}

bool JSLanguage::has_named_classes() const
{
	return false;
}

bool JSLanguage::supports_builtin_mode() const
{
	return false;
}

int JSLanguage::find_function(const String & p_function, const String & p_code) const
{
	return 0;
}

String JSLanguage::make_function(const String & p_class, const String & p_name, const PoolStringArray & p_args) const
{	// FIXME
	// - Due to Godot's API limitation this just appends the function to the end of the file
	// - Use fully qualified name if there is ambiguity
	String s = "private void " + p_name + "(";
	for (int i = 0; i < p_args.size(); i++) {
		const String &arg = p_args[i];

		if (i > 0)
			s += ", ";

		s += arg.get_slice(":", 0);
	}
	s += ")\n{\n    // Replace with function body.\n}\n";

	return s;
}

void JSLanguage::auto_indent_code(String & p_code, int p_from_line, int p_to_line) const
{
}

void JSLanguage::add_global_constant(const StringName & p_variable, const Variant & p_value)
{
}

String JSLanguage::debug_get_error() const
{
	return String();
}

int JSLanguage::debug_get_stack_level_count() const
{
	return 0;
}

int JSLanguage::debug_get_stack_level_line(int p_level) const
{
	return 0;
}

String JSLanguage::debug_get_stack_level_function(int p_level) const
{
	return String();
}

String JSLanguage::debug_get_stack_level_source(int p_level) const
{
	return String();
}

void JSLanguage::debug_get_stack_level_locals(int p_level, List<String>* p_locals, List<Variant>* p_values, int p_max_subitems/*  = -1 */, int p_max_depth/*  = -1 */)
{
}

void JSLanguage::debug_get_stack_level_members(int p_level, List<String>* p_members, List<Variant>* p_values, int p_max_subitems/*  = -1 */, int p_max_depth/*  = -1 */)
{
}

void JSLanguage::debug_get_globals(List<String>* p_globals, List<Variant>* p_values, int p_max_subitems/*  = -1 */, int p_max_depth/*  = -1 */)
{
}

String JSLanguage::debug_parse_stack_level_expression(int p_level, const String & p_expression, int p_max_subitems/*  = -1 */, int p_max_depth/*  = -1 */)
{
	return String();
}

void JSLanguage::reload_all_scripts()
{
}

void JSLanguage::reload_tool_script(const Ref<Script>& p_script, bool p_soft_reload)
{
}

void JSLanguage::get_recognized_extensions(List<String>* p_extensions) const
{
}

void JSLanguage::get_public_functions(List<MethodInfo>* p_functions) const
{
}

void JSLanguage::get_public_constants(List<Pair<String, Variant>>* p_constants) const
{
}

void JSLanguage::profiling_start()
{
}

void JSLanguage::profiling_stop()
{
}

int JSLanguage::profiling_get_accumulated_data(ProfilingInfo * p_info_arr, int p_info_max)
{
	return 0;
}

int JSLanguage::profiling_get_frame_data(ProfilingInfo * p_info_arr, int p_info_max)
{
	return 0;
}
