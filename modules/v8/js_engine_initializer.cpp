#include "js_engine_initializer.h"
#include "core/list.h"
#include "core/os/dir_access.h"
#include "core/os/file_access.h"
#include "core/os/os.h"
#include "core/script_language.h"
#include "core/ustring.h"
#include "core/vector.h"
#include "js_registrations.h"
#include "jslib/jswrapper/SeApi.h"
#include <iostream>
#include <map>
static void PrintValue(const se::Value &v) {
	se::Value::Type t = v.getType();
	switch (t) {
		case se::Value::Type::Undefined:
			std::cout << "Undefined";
			break;
		case se::Value::Type::Null:
			std::cout << "Null";
			break;
		case se::Value::Type::Number:
		case se::Value::Type::Boolean:
		case se::Value::Type::String:
			std::cout << v.toStringForce().c_str();
			break;
		case se::Value::Type::Object: {
			auto obj = v.toObject();
			std::cout << obj->toJSONString().c_str();
		} break;
		default:
			break;
	}
}
static std::vector<String> dirs;
static String findPath(const String &name) {
	String path = name;
	static DirAccess *f = DirAccess::create(DirAccess::ACCESS_RESOURCES);

	if (dirs.empty()) {
		dirs.push_back(f->get_current_dir());
	}
	if (path.is_abs_path()) return path;
	for (auto &&dir : dirs) {
		path = dir;
		path.plus_file(name);
		path.simplify_path();
		if (f->exists(path)) return path;
		path += ".js";
		if (f->exists(path)) return path;
	}
	path = f->get_current_dir();
	path.plus_file(name);
	path.simplify_path();
	return path;
}
static String findFile(const std::string &name) {
	auto file = findPath(name.c_str());
	auto file_indexjs = file.plus_file("index.js");
	if (DirAccess::exists(file_indexjs)) {
		return file_indexjs;
	}
	return file;
}
static bool print(se::State &state) {
	for (auto &&arg : state.args()) {
		PrintValue(arg);
	}
	std::cout << std::endl;
	return true;
}

SE_BIND_FUNC(print);
enum PrintFunctions
{
		TEXT_PRINT,
		TEXT_PRINT_TABBED,
		TEXT_PRINT_SPACED,
		TEXT_PRINTERR,
		TEXT_PRINTRAW,
		TEXT_PRINT_DEBUG,
};
static bool printCB(se::State& state)
{
	static const std::map<std::string, PrintFunctions> PrintFunctionsMap={
		{"print", TEXT_PRINT},
		{"printt", TEXT_PRINT_TABBED},
		{"prints", TEXT_PRINT_SPACED},
		{"printe", TEXT_PRINTERR},
		{"printraw", TEXT_PRINTRAW},
		{"printd", TEXT_PRINT_DEBUG}
	};
	auto it = PrintFunctionsMap.find(state.data().toString().c_str());
	if(it == PrintFunctionsMap.end()) return false;
	auto&& js_args = state.args();
	Vector<Variant> variant_args;
	Vector<const Variant*> p_args;
	JSRegistrations::js_to_variant(js_args, variant_args, p_args);
	auto p_arg_count = variant_args.size();
	Variant r_ret;
	switch (it->second)
	{
	
		case TEXT_PRINT: {

			String str;
			for (int i = 0; i < p_arg_count; i++) {

				str += p_args[i]->operator String();
			}

			print_line(str);
			r_ret = Variant();

		} break;
		case TEXT_PRINT_TABBED: {

			String str;
			for (int i = 0; i < p_arg_count; i++) {

				if (i)
					str += "\t";
				str += p_args[i]->operator String();
			}

			print_line(str);
			r_ret = Variant();

		} break;
		case TEXT_PRINT_SPACED: {

			String str;
			for (int i = 0; i < p_arg_count; i++) {

				if (i)
					str += " ";
				str += p_args[i]->operator String();
			}

			print_line(str);
			r_ret = Variant();

		} break;

		case TEXT_PRINTERR: {

			String str;
			for (int i = 0; i < p_arg_count; i++) {

				str += p_args[i]->operator String();
			}

			print_error(str);
			r_ret = Variant();

		} break;
		case TEXT_PRINTRAW: {
			String str;
			for (int i = 0; i < p_arg_count; i++) {

				str += p_args[i]->operator String();
			}

			OS::get_singleton()->print("%s", str.utf8().get_data());
			r_ret = Variant();

		} break;
		case TEXT_PRINT_DEBUG: {
			String str;
			for (int i = 0; i < p_arg_count; i++) {

				str += p_args[i]->operator String();
			}

			print_line(str);
			r_ret = Variant();
		} break;
		
	default:
		return false;
	}
	return true;
}

SE_BIND_FUNC(printCB);

static bool runScript(se::State &state) {
	auto se = se::ScriptEngine::getInstance();
	if (state.args().size() != 1) return false;
	auto arg = state.args()[0];
	{
		se::Value ret;
		auto file = findFile(arg.toString());
		auto extension = file.get_extension();
		std::string file_str = file.utf8().get_data();
		if (extension.nocasecmp_to(".json") == 0) {
			auto src = se->getFileOperationDelegate().onGetStringFromFile(file_str);
			ret.setObject(se::Object::createJSONObject(src));
			PrintValue(ret);
		} else {

			dirs.push_back(file.get_base_dir());
			se->evalString("module={exports:{}}; module.exports=exports={}");
			se->runScript(file_str, &ret);
			se->evalString("exports", -1, &ret);
			se->evalString("exports={}");
			dirs.pop_back();
			//PrintValue(ret);
		}
		state.rval() = ret;
	}

	return true;
}
SE_BIND_FUNC(runScript);

void InitFileOperationDeligate() {
	static se::ScriptEngine::FileOperationDelegate delegate;
	if (!delegate.isValid()) {
		delegate.onGetDataFromFile = [](const std::string &path, const std::function<void(const uint8_t *, size_t)> &readCallback) -> void {
			assert(!path.empty());
			String p_path = path.c_str();
			PoolVector<uint8_t> sourcef;
			Error err;
			FileAccess *f = FileAccess::open(p_path, FileAccess::READ, &err);
			if (err) {

				SE_LOGE("ScriptEngine::onGetStringFromFile %s not found, possible missing file.\n", path.c_str());
				return;
			}

			int len = f->get_len();
			sourcef.resize(len);
			PoolVector<uint8_t>::Write w = sourcef.write();
			int r = f->get_buffer(w.ptr(), len);
			f->close();
			memdelete(f);
			if (r != len) {
				SE_LOGE("ScriptEngine::onGetDataFromFile %s not found, possible missing file.\n", path.c_str());
			}

			readCallback(w.ptr(), len);
		};

		delegate.onGetStringFromFile = [](const std::string &path) -> std::string {
			assert(!path.empty());
			String p_path = path.c_str();
			PoolVector<uint8_t> sourcef;
			Error err;
			FileAccess *f = FileAccess::open(p_path, FileAccess::READ, &err);
			if (err) {
				SE_LOGE("ScriptEngine::onGetStringFromFile %s not found, possible missing file.\n", path.c_str());
				return "";
			}

			int len = f->get_len();
			sourcef.resize(len + 1);
			PoolVector<uint8_t>::Write w = sourcef.write();
			int r = f->get_buffer(w.ptr(), len);
			f->close();
			memdelete(f);
			if (r != len) {
				SE_LOGE("ScriptEngine::onGetStringFromFile %s not found, possible missing file.\n", path.c_str());
				return "";
			}
			w[len] = 0;

			String s;
			if (s.parse_utf8((const char *)w.ptr())) {

				ERR_EXPLAIN("Script '" + p_path + "' contains invalid unicode (utf-8), so it was not loaded. Please ensure that scripts are saved in valid utf-8 unicode.");

				SE_LOGE("ScriptEngine::onGetStringFromFile %s not found, possible missing file.\n", path.c_str());
				return "";
			}
			return s.utf8().get_data();
		};

		delegate.onGetFullPath = [](const std::string &path) -> std::string {
			assert(!path.empty());
			String gpath = path.c_str();
			if (gpath.is_abs_path()) return path;
			gpath = DirAccess::get_full_path(gpath, DirAccess::ACCESS_RESOURCES);
			return gpath.utf8().get_data();
		};

		delegate.onCheckFileExist = [](const std::string &path) -> bool {
			assert(!path.empty());
			return DirAccess::exists(path.c_str());
		};

		assert(delegate.isValid());

		se::ScriptEngine::getInstance()->setFileOperationDelegate(delegate);
	}
}

bool JSEngineInitializer::Initialize() {

	auto se = se::ScriptEngine::getInstance();

	se->enableDebugger("127.0.0.1", 6086, false);
	se->addRegisterCallback([](se::Object *global) -> bool {
		global->defineFunction("print_n", _SE(print));
		global->defineFunction("print", _SE(printCB));
		global->defineFunction("printt", _SE(printCB));
		global->defineFunction("prints", _SE(printCB));
		global->defineFunction("printe", _SE(printCB));
		global->defineFunction("printd", _SE(printCB));
		global->defineFunction("printraw", _SE(printCB));
		global->defineFunction("runScript", _SE(runScript));
		JSRegistrations::register_all();
		return true;
	});
	InitFileOperationDeligate();
	return se->start();
}

void JSEngineInitializer::Unload() 
{
	se::ScriptEngine::destroyInstance();
}