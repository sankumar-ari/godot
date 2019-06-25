#ifndef JS_RESOURCE_FORMAT_H
#define JS_RESOURCE_FORMAT_H

#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"

class ResourceFormatLoaderJS : public ResourceFormatLoader {
	GDCLASS(ResourceFormatLoaderJS, ResourceFormatLoader)
public:
	virtual RES load(const String &p_path, const String &p_original_path = "", Error *r_error = NULL);
	virtual void get_recognized_extensions(List<String> *p_extensions) const;
	virtual bool handles_type(const String &p_type) const;
	virtual String get_resource_type(const String &p_path) const;
	virtual void get_dependencies(const String &p_path, List<String> *p_dependencies, bool p_add_types = false);
};

class ResourceFormatSaverJS : public ResourceFormatSaver {
	GDCLASS(ResourceFormatSaverJS, ResourceFormatSaver)
public:
	virtual Error save(const String &p_path, const RES &p_resource, uint32_t p_flags = 0);
	virtual void get_recognized_extensions(const RES &p_resource, List<String> *p_extensions) const;
	virtual bool recognize(const RES &p_resource) const;
};


#endif //JS_RESOURCE_FORMAT_H