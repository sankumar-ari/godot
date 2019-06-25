#include "js_engine_initializer.h"
#include "jslib/jswrapper/SeApi.h"
#include "core/list.h"
#include "core/script_language.h"
#include "core/ustring.h"
#include "core/vector.h"
#include <iostream>
#include "core/os/dir_access.h"
static void PrintValue(const se::Value& v)
{
	se::Value::Type t = v.getType();
	switch (t)
	{
	case se::Value::Type::Undefined:
		std::cout<<"Undefined";
		break;
	case se::Value::Type::Null:
		std::cout<<"Null";
		break;
	case se::Value::Type::Number:
	case se::Value::Type::Boolean:
	case se::Value::Type::String:
	std::cout<<v.toStringForce().c_str();
		break;
	case se::Value::Type::Object:
	{
		auto obj = v.toObject();
		std::cout<<obj->toJSONString().c_str();
	}
		break;
	default:
		break;
	}

}
static std::vector<String> dirs;
static String findPath(const String& name)
{
	String path = name;
	static DirAccess *f = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	
	if(dirs.empty())
	{
		dirs.push_back(f->get_current_dir());
	}
	if(path.is_abs_path()) return path;
	for(auto&& dir : dirs)
	{
		path = dir;
		path.plus_file(name);
		path.simplify_path();
		if(f->exists(path)) return path;
		path+=".js";
		if(f->exists(path)) return path;

	}
	path=f->get_current_dir();
	path.plus_file(name);
	path.simplify_path();
	return path;
}
static String findFile(const std::string& name)
{
	auto file = findPath(name);
	auto file_indexjs = file.plus_file("index.js");
	if(DirAccess::exists(file_indexjs)) 
	{
		return file_indexjs;
	}
	return file;
}
static bool print(se::State& state)
{
	for(auto&& arg : state.args())
	{		
		PrintValue(arg);
	}
	std::cout << std::endl;
	return true;
}

SE_BIND_FUNC(print);


static bool runScript(se::State& state)
{
	auto se = se::ScriptEngine::getInstance();
	if(state.args().size() != 1) return false;
	auto arg = state.args()[0];
	{		
		se::Value ret;
		auto file = findFile(arg.toString());
		auto extension = file.get_extension();
		std::string file_str = file.utf8().get_data();
		if(extension.nocasecmp_to(".json") == 0)
		{
			auto src = se->getFileOperationDelegate().onGetStringFromFile(file_str);
 			ret.setObject(se::Object::createJSONObject(src));
			PrintValue(ret);
		}
		else
		{
			
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


bool JSEngineInitializer::Initialize()
{

	auto se = se::ScriptEngine::getInstance();
	
	se->enableDebugger("127.0.0.1", 6086, false);
	se->addRegisterCallback([](se::Object* global)->bool{
		global->defineFunction("print", _SE(print));
		global->defineFunction("runScript", _SE(runScript));
		auto cls = se::Class::create("Test", global, nullptr, _SE(CreatClassObject));
		cls->defineFunction("methodOne", _SE(FunctionCall));		
		cls->install();
		return true;
	});
	InitFileOperationDeligate();
	se->start();
}