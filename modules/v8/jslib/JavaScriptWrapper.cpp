// JavaScriptWrapper.cpp : This file contains the 'main' function. Program execution begins and ends there.
//


#include <iostream>
#include <functional>
#include <unordered_map>
#include "jswrapper/SeApi.h"
#include <filesystem>
#include <fstream>
#include <streambuf>
#include <set>
#include <algorithm>
#include <vector>
#include <thread>
namespace fs = std::experimental::filesystem;
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
static std::vector<fs::path> dirs = {
	"D:/Aristocrat/POCs/JS/GDK2.0/server/build/src/",
	"D:/Aristocrat/POCs/JS/GDK2.0/server/node_modules/"
};
static fs::path findPath(const std::string& name)
{
	fs::path file(name);
	if(fs::exists(file)) return fs::absolute(file).string();

	for(auto&& dir : dirs)
	{
		fs::path dirp(dir);
		fs::path path = dirp/file;
		if(fs::exists(path)) return path.string();
		path.replace_extension(".js");
		if(fs::exists(path)) return path.string();

	}
	return  fs::absolute(file).string();

}
static std::string findFile(const std::string& name)
{
	auto file = findPath(name);
	if(fs::is_directory(file) && fs::exists(file/"index.js")) 
	{
		return (file/"index.js").string();
	}
	return file.string();
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
		fs::path file = findFile(arg.toString());
		auto extension = file.extension().string();
		std::for_each(extension.begin(), extension.end(), [](char & c){
			c = ::tolower(c);
		});
		if(extension == ".json")
		{
			auto src = se->getFileOperationDelegate().onGetStringFromFile(file.string());
 			ret.setObject(se::Object::createJSONObject(src));
			PrintValue(ret);
		}
		else
		{
			dirs.push_back(file.parent_path());
			se->evalString("module={exports:{}}; module.exports=exports={}");
			se->runScript(file.string(), &ret);
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


class Test{
public:
	void methodOne(const se::Value& v1)
	{
		PrintValue(v1);
		std::cout << std::endl;
	}
	void methodTwo(const se::Value& v1, const se::Value& v2){
		PrintValue(v1);	
		std::cout << std::endl;	
		PrintValue(v2);
		std::cout << std::endl;
	}
};
struct Registery
{
	std::function<void*()> factory;
	std::function<se::Value(const std::string& fn, se::State&)> invoker;
	std::function<void(void*)> deleter;
	se::Class* cls = nullptr;
	
};
template <typename T>
struct RegisteryTyped : public Registery{
	RegisteryTyped(std::function<se::Value(const std::string& fn, se::State&)> invok){
		factory = [](){return new T(); };
		deleter = [](void* v){ delete static_cast<T*>(v);};
		invoker = invok;
	}
};

struct ObjectWrapper{
	Registery * registery = nullptr;
	void* obj = nullptr;

	ObjectWrapper(Registery * reg):registery(reg){
		Create();
	}
	void Create()
	{
		if(!obj && registery->factory) obj=registery->factory();
	}
	void Destroy(){
		if(obj && registery->deleter){
			registery->deleter(obj);
			obj =nullptr;
			registery = nullptr;
			delete this;
		}
	}
	void invoke(const std::string& name, se::State& state)
	{
		if(registery && registery->invoker) registery->invoker(name, state);
	}
};
static std::unordered_map<std::string, Registery*> registeries={
	{"Test", new RegisteryTyped<Test>([](const std::string& fn, se::State& s)->se::Value{ 
								auto t = static_cast<Test*>(s.nativeThisObject());  
								if(fn == "methodOne") t->methodOne(s.args()[0]); 
								else t->methodTwo(s.args()[0], s.args()[1]); 
								return se::Value();
								}
	)}
};
static bool CreatClassObject(se::State& state)
{
	std::string name = state.data().toString();
	auto it = registeries.find(name);
	if(it == registeries.end()){
		return false;
	}
	
	auto obj = new ObjectWrapper(it->second);
	std::cout << "Creted " << name.c_str() << std::endl;
	state.thisObject()->setPrivateData(obj);
	return true;
}
static bool DestrocyClassObject(se::State& state)
{
	auto obj = static_cast<ObjectWrapper*>(state.nativeThisObject());
	obj->Destroy();
	std::cout << "Deleted "<< std::endl;
	return true;
}

static bool FunctionCall(se::State& state) 
{
	auto obj = static_cast<ObjectWrapper*>(state.nativeThisObject());
	
	std::string name = state.data().toString();

	obj->invoke(name, state);
	return true;
}
SE_DECLARE_FINALIZE_FUNC(DestrocyClassObject)
SE_BIND_CTOR(CreatClassObject, nullptr, DestrocyClassObject);
SE_BIND_FINALIZE_FUNC(DestrocyClassObject);
SE_BIND_FUNC(FunctionCall);

void InitFileOperationDeligate()
{
    static se::ScriptEngine::FileOperationDelegate delegate;
    if (!delegate.isValid())
    {
        delegate.onGetDataFromFile = [](const std::string& path, const std::function<void(const uint8_t*, size_t)>& readCallback) -> void{
            assert(!path.empty());
			std::ifstream file(path.c_str(), std::ios::binary | std::ios::ate);
			std::streamsize size = file.tellg();
			file.seekg(0, std::ios::beg);

			std::vector<uint8_t> buffer(size);
			if (file.read(reinterpret_cast<char*>(buffer.data()), size))
			{
				readCallback(buffer.data(), buffer.size());
			}
			else
			{
                SE_LOGE("ScriptEngine::onGetDataFromFile %s not found, possible missing file.\n", path.c_str());
			}
        };

        delegate.onGetStringFromFile = [](const std::string& path) -> std::string{
            assert(!path.empty());
			std::ifstream t(path.c_str());
            if (t) {
				std::string str((std::istreambuf_iterator<char>(t)),
							 std::istreambuf_iterator<char>());
				return str;
            }
            else {
                SE_LOGE("ScriptEngine::onGetStringFromFile %s not found, possible missing file.\n", path.c_str());
            }
            return "";
        };

        delegate.onGetFullPath = [](const std::string& path) -> std::string{
            assert(!path.empty());
            
			return std::experimental::filesystem::absolute(path).string();
        };

        delegate.onCheckFileExist = [](const std::string& path) -> bool{
            assert(!path.empty());
            return std::experimental::filesystem::exists(path);
        };

        assert(delegate.isValid());

        se::ScriptEngine::getInstance()->setFileOperationDelegate(delegate);
    }
}

int main()
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
	uint64_t counter = 0;
	{
		se::AutoHandleScope hs;
		se->evalString("var exports = {}; function require(file) \
{	\
	return runScript(file);	\
} \
require('index.js') \
");

		
		se->evalString("(new Test()).methodOne('blah');");
	}
	//while(true)
	{
	//se->mainLoopUpdate();
	{
		se::AutoHandleScope hs;
		se->evalString("main();");
		if(counter++%100==0) se->garbageCollect();
		std::this_thread::yield();
	}
	}
    std::cout << "Hello World!\n"; 
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
