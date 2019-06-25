#include "js_resource_format.h"
#include "js_script.h"
#include "core/os/file_access.h"
RES ResourceFormatLoaderJS::load(const String &p_path, const String &p_original_path, Error *r_error) {

	if (r_error)
		*r_error = ERR_FILE_CANT_OPEN;

	JS_Script *script = memnew(JS_Script);

	Ref<JS_Script> scriptres(script);

	if (p_path.ends_with(".js")) {
		Error err = script->load_source_code(p_path);
		ERR_FAIL_COND_V(err != OK, RES());

		script->set_script_path(p_original_path); // script needs this.
		script->set_path(p_original_path);

		script->reload();
	}
	if (r_error)
		*r_error = OK;

	return scriptres;
}

void ResourceFormatLoaderJS::get_recognized_extensions(List<String> *p_extensions) const {

	p_extensions->push_back("js");
}

bool ResourceFormatLoaderJS::handles_type(const String &p_type) const {

	return (p_type == "Script" || p_type == "ECMAScript");
}

String ResourceFormatLoaderJS::get_resource_type(const String &p_path) const {

	String el = p_path.get_extension().to_lower();
	if (el == "js")
		return "ECMAScript";
	return "";
}

void ResourceFormatLoaderJS::get_dependencies(const String &p_path, List<String> *p_dependencies, bool p_add_types) {

	FileAccessRef file = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND(!file);

	String source = file->get_as_utf8_string();
	if (source.empty()) {
		return;
	}

}

Error ResourceFormatSaverJS::save(const String &p_path, const RES &p_resource, uint32_t p_flags) {

	Ref<JS_Script> sqscr = p_resource;
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
		JSLanguage::get_singleton()->reload_tool_script(p_resource, false);
	}

	return OK;
}

void ResourceFormatSaverJS::get_recognized_extensions(const RES &p_resource, List<String> *p_extensions) const {

	if (Object::cast_to<JS_Script>(*p_resource)) {
		p_extensions->push_back("js");
	}
}
bool ResourceFormatSaverJS::recognize(const RES &p_resource) const {

	return Object::cast_to<JS_Script>(*p_resource) != NULL;
}
