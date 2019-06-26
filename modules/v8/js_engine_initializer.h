#ifndef JS_ENGINE_INITIALIZER_H
#define JS_ENGINE_INITIALIZER_H
class JSEngineInitializer
{
public:
    JSEngineInitializer() = delete;
    static bool Initialize();
    static void Unload();
};

#endif //JS_ENGINE_INITIALIZER_H
