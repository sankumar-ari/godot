/*************************************************************************/
/*  js_functions.cpp                                                     */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                    http://www.godotengine.org                         */
/*************************************************************************/
/* Copyright (c) 2007-2016 Juan Linietsky, Ariel Manzur.                 */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "js_functions.h"
#include "js_language.h"

#include "core/class_db.h"
#include "core/os/dir_access.h"
#include "core/os/file_access.h"
#include "core/print_string.h"
#include "core/variant.h"
#include "core/vector.h"

#include "v8.h"

Object *JavaScriptFunctions::unwrap_object(const v8::Local<v8::Object> &p_value) {

	v8::Local<v8::External> field = v8::Local<v8::External>::Cast(p_value->GetInternalField(0));
	void *ptr = field->Value();
	return static_cast<Object *>(ptr);
}

Variant *JavaScriptFunctions::unwrap_variant(const v8::Local<v8::Object> &p_value) {

	v8::Local<v8::External> field = v8::Local<v8::External>::Cast(p_value->GetInternalField(0));
	void *ptr = field->Value();
	return static_cast<Variant *>(ptr);
}

v8::Local<v8::Value> JavaScriptFunctions::variant_to_js(v8::Isolate *p_isolate, const Variant &p_var) {

	v8::EscapableHandleScope scope(p_isolate);
	v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
	v8::Local<v8::Value> val;

	switch (p_var.get_type()) {
		case Variant::BOOL: {
			val = v8::Boolean::New(p_isolate, bool(p_var));
		} break;
		case Variant::STRING: {
			val = v8::String::NewFromUtf8(p_isolate, String(p_var).utf8().get_data());
		} break;
		case Variant::INT: {
			val = v8::Integer::New(p_isolate, int(p_var));
		} break;
		case Variant::REAL: {
			val = v8::Number::New(p_isolate, double(p_var));
		} break;
		case Variant::OBJECT: {
			Object *obj = (Object *)p_var;
			Reference *ref = Object::cast_to<Reference>(obj);

			if (ref) {
				// Unsafe refcount increment. The managed instance also counts as a reference.
				// This way if the unmanaged world has no references to our owner
				// but the managed instance is alive, the refcount will be 1 instead of 0.
				// See: _GodotSharp::_dispose_object(Object *p_object)

				ref->reference();
			}
			if (obj) {
				WARN_PRINT(obj->get_class().utf8().get_data());
				v8::Local<v8::Context> ctx = p_isolate->GetCurrentContext();
				auto cv = ctx->Global()->Get(
						v8::String::NewFromUtf8(p_isolate, obj->get_class().utf8().get_data()));
				v8::Local<v8::Function> constructor;
				if (cv.IsEmpty()) {
					WARN_PRINT(obj->get_class().utf8().get_data());
					WARN_PRINT("returning null");
					val = v8::Null(p_isolate);

				} else {
					constructor = v8::Local<v8::Function>::Cast(cv);
					// Call the constructor with the object as argument
					v8::Local<v8::Value> cargs[] = { v8::External::New(p_isolate, obj) };
					auto maybeobj = constructor->CallAsConstructor(context, 1, cargs);
					if (maybeobj.IsEmpty()) {
						WARN_PRINT(obj->get_class().utf8().get_data());
						WARN_PRINT("returning null");
						val = v8::Null(p_isolate);
					} else {
						v8::Local<v8::Object> instance = v8::Local<v8::Object>::Cast(maybeobj.ToLocalChecked());
						// Set as external to V8JavaScript
						instance->SetInternalField(1, v8::Boolean::New(p_isolate, false));
						val = instance;
					}
				}
			}
		} break;
			// Math types
		case Variant::VECTOR2:
		case Variant::RECT2:
		case Variant::VECTOR3:
		case Variant::TRANSFORM2D:
		case Variant::PLANE:
		case Variant::QUAT:
		case Variant::BASIS:
		case Variant::TRANSFORM:
			// Misc types
		case Variant::COLOR:
		case Variant::NODE_PATH:
		case Variant::_RID:
			// case Variant::INPUT_EVENT:
			// case Variant::IMAGE:
			{

				Variant *obj = new Variant(p_var);

				v8::Local<v8::Context> ctx = p_isolate->GetCurrentContext();
				v8::Local<v8::Function> constructor = v8::Local<v8::Function>::Cast(ctx->Global()->Get(
						v8::String::NewFromUtf8(p_isolate, Variant::get_type_name(p_var.get_type()).utf8().get_data())));

				// Call the constructor with the no arguments
				v8::Local<v8::Object> instance = v8::Local<v8::Object>::Cast(constructor->CallAsConstructor(context, 0, NULL).ToLocalChecked());
				// Set the inner object manually
				instance->SetInternalField(0, v8::External::New(p_isolate, obj));

				val = instance;
			}
			break;
		default: {
			val = v8::Null(p_isolate);
		}
	}

	return scope.Escape(val);
}

Variant JavaScriptFunctions::js_to_variant(v8::Isolate *p_isolate, const v8::Local<v8::Value> &p_value) {

	v8::Local<v8::Context> context = p_isolate->GetCurrentContext();
	if (p_value->IsNull() || p_value->IsUndefined()) {
		return Variant();
	}
	if (p_value->IsBoolean()) {
		return Variant(p_value->BooleanValue(p_isolate));
	}
	if (p_value->IsString()) {
		v8::String::Utf8Value v(p_isolate, p_value);
		return Variant(String(*v));
	}
	if (p_value->IsInt32() || p_value->IsUint32()) {
		return Variant(p_value->IntegerValue(context).ToChecked());
	}
	if (p_value->IsNumber()) {
		return Variant(p_value->NumberValue(context).ToChecked());
	}
	if (p_value->IsObject()) {
		v8::Local<v8::Object> js_obj = v8::Local<v8::Object>::Cast(p_value);
		if (js_obj->InternalFieldCount() == 2) {
			if (js_obj->GetInternalField(1)->IsBoolean()) {
				Object *obj = unwrap_object(js_obj);
				Reference *ref = Object::cast_to<Reference>(obj);

				if (ref) {
					Ref<Reference> ref(ref);
					return Variant(ref);
				}
				return Variant(obj);
			} else {
				Variant *var = unwrap_variant(js_obj);
				return *var;
			}
		}
	}

	return Variant();
}

Variant::Type JavaScriptFunctions::type_from_string(const String &p_type) {

	// Only for binding purpose. Missing types here can be handled with JS types.

	// Math Types
	if (p_type == "Vector2")
		return Variant::VECTOR2;
	if (p_type == "Rect2")
		return Variant::RECT2;
	if (p_type == "Vector3")
		return Variant::VECTOR3;
	// if (p_type == "Matrix32")
	// 	return Variant::MATRIX32;
	if (p_type == "Plane")
		return Variant::PLANE;
	if (p_type == "Quat")
		return Variant::QUAT;
	// if (p_type == "AABB")
	// 	return Variant::_AABB;
	// if (p_type == "Matrix3")
	// 	return Variant::MATRIX3;
	if (p_type == "Transform")
		return Variant::TRANSFORM;

	// Misc Types
	if (p_type == "Color")
		return Variant::COLOR;
	// if (p_type == "Image")
	// 	return Variant::IMAGE;
	if (p_type == "NodePath")
		return Variant::NODE_PATH;
	if (p_type == "RID")
		return Variant::_RID;
	// if (p_type == "InputEvent")
	// 	return Variant::INPUT_EVENT;

	// Should never get here
	return Variant::NIL;
}

/****** JAVASCRIPT FUNCTIONS ******/

void JavaScriptFunctions::require(const v8::FunctionCallbackInfo<v8::Value> &p_args) {
	v8::Isolate *isolate = p_args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::String::Utf8Value dir(isolate, context->Global()->Get(v8::String::NewFromUtf8(isolate, "__dirname")));
	v8::String::Utf8Value file(isolate, p_args[0]->ToString(context).ToLocalChecked());

	String path = String(*dir).plus_file(*file);

	if (DirAccess::exists(path)) {
		Error err = OK;
		FileAccess *access = FileAccess::open(path, FileAccess::READ, &err);

		if (err != OK) {
			isolate->ThrowException(v8::String::NewFromUtf8(isolate, "Error opening file"));
			return;
		}

		String content;
		String l = access->get_line();
		while (!access->eof_reached()) {
			content += l + "\n";
			l = access->get_line();
		}
		content += l;

		v8::Local<v8::String> source = v8::String::NewFromUtf8(isolate, content.utf8().get_data());
		v8::ScriptOrigin origin(v8::String::NewFromUtf8(isolate, path.utf8().get_data()));
		v8::Local<v8::Script> script = v8::Script::Compile(context, source, &origin).ToLocalChecked();
		v8::Local<v8::Value> result = script->Run(context).ToLocalChecked();

		p_args.GetReturnValue().Set(result);
	}

	p_args.GetReturnValue().SetUndefined();
}

void JavaScriptFunctions::print(const v8::FunctionCallbackInfo<v8::Value> &args) {

	if (args.Length() < 1) return;

	v8::HandleScope scope(args.GetIsolate());
	v8::Local<v8::Value> arg = args[0];
	v8::String::Utf8Value value(args.GetIsolate(), arg);
	if (*value)
		print_line(*value);
}

void JavaScriptFunctions::Vector2_constructor(const v8::FunctionCallbackInfo<v8::Value> &p_args) {
	v8::Isolate *isolate = p_args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	if (!p_args.IsConstructCall()) {
		isolate->ThrowException(v8::String::NewFromUtf8(isolate, "Can't call type as a function"));
		return;
	}

	switch (p_args.Length()) {
		case 0: {
			p_args.This()->SetInternalField(0, v8::External::New(isolate, memnew(Vector2)));
			return;
		} break;
		case 2: {
			Vector2 *vec = memnew(Vector2);
			vec->x = p_args[0]->NumberValue(context).ToChecked();
			vec->y = p_args[1]->NumberValue(context).ToChecked();
			p_args.This()->SetInternalField(0, v8::External::New(isolate, vec));
			return;
		} break;
		default:
			break;
	}
}

void JavaScriptFunctions::Vector2_add(const v8::FunctionCallbackInfo<v8::Value> &p_args) {}

void JavaScriptFunctions::Vector2_length(const v8::FunctionCallbackInfo<v8::Value> &p_args) {}

void JavaScriptFunctions::Vector2_length_squared(const v8::FunctionCallbackInfo<v8::Value> &p_args) {

	v8::Isolate *isolate = p_args.GetIsolate();

	if (p_args.Length() != 0) {
		isolate->ThrowException(v8::String::NewFromUtf8(isolate, "Too many arguments"));
		return;
	}

	Vector2 *vec = static_cast<Vector2 *>(v8::Local<v8::External>::Cast(p_args.This()->GetInternalField(0))->Value());
	Variant var_vec = Variant(*vec);

	Variant::CallError err;
	Variant res = var_vec.call("length_squared", NULL, 0, err);

	p_args.GetReturnValue().Set(double(res));
}
