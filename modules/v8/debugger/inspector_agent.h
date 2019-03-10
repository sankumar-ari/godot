/*
*    Copyright Node.js contributors. All rights reserved.
*
*    Permission is hereby granted, free of charge, to any person obtaining a copy
*    of this software and associated documentation files (the "Software"), to
*    deal in the Software without restriction, including without limitation the
*    rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
*    sell copies of the Software, and to permit persons to whom the Software is
*    furnished to do so, subject to the following conditions:
*
*    The above copyright notice and this permission notice shall be included in
*    all copies or substantial portions of the Software.
*
*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
*    IN THE SOFTWARE.
*/

#ifndef SRC_INSPECTOR_AGENT_H_
#define SRC_INSPECTOR_AGENT_H_

#if defined _WIN32 || defined __CYGWIN__ || defined __MINGW32__
//#ifdef BUILDING_DLL
//#ifdef __GNUC__
//#define DLL_PUBLIC __attribute__ ((dllexport))
//#else
//#define DLL_PUBLIC __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
//#endif
//#else
//#ifdef __GNUC__
//#define DLL_PUBLIC __attribute__ ((dllimport))
//#else
//#define DLL_PUBLIC __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
//#endif
//#endif
//#define DLL_LOCAL
//#else
//#if __GNUC__ >= 4
//#define DLL_PUBLIC __attribute__ ((visibility ("default")))
//#define DLL_LOCAL  __attribute__ ((visibility ("hidden")))
//#else
#define DLL_PUBLIC
#define DLL_LOCAL
//#endif
#endif

#include "v8-inspector.h"
#include "v8.h"
#include <memory>
#include <string>

#include <stddef.h>

namespace inspector {

class InspectorSessionDelegate {
public:
	virtual ~InspectorSessionDelegate() = default;
	virtual bool WaitForFrontendMessageWhilePaused() = 0;
	virtual void SendMessageToFrontend(const v8_inspector::StringView &message) = 0;
};

class InspectorIo;
class CBInspectorClient;

class Agent {
public:
	DLL_PUBLIC Agent(std::string host_name, std::string file_path);
	DLL_PUBLIC ~Agent();

	// Create client_, may create io_ if option enabled
	DLL_PUBLIC bool Start(v8::Isolate *isolate, v8::Platform *platform, const char *path);
	// Stop and destroy io_
	DLL_PUBLIC void Stop();

	bool IsStarted() { return !!client_; }

	// IO thread started, and client connected
	bool IsConnected();

	void WaitForDisconnect();
	void FatalException(v8::Local<v8::Value> error,
			v8::Local<v8::Message> message);

	// These methods are called by the WS protocol and JS binding to create
	// inspector sessions.  The inspector responds by using the delegate to send
	// messages back.
	void Connect(InspectorSessionDelegate *delegate);
	void Disconnect();
	void Dispatch(const v8_inspector::StringView &message);
	InspectorSessionDelegate *delegate();

	void RunMessageLoop();
	bool enabled() { return enabled_; }
	DLL_PUBLIC void PauseOnNextJavascriptStatement(const std::string &reason);

	// Initialize 'inspector' module bindings
	static void InitInspector(v8::Local<v8::Object> target,
			v8::Local<v8::Value> unused,
			v8::Local<v8::Context> context,
			void *priv);

	InspectorIo *io() {
		return io_.get();
	}

	// Can only be called from the the main thread.
	bool StartIoThread(bool wait_for_connect);

	// Calls StartIoThread() from off the main thread.
	void RequestIoThreadStart();

private:
	std::unique_ptr<CBInspectorClient> client_;
	std::unique_ptr<InspectorIo> io_;
	v8::Platform *platform_;
	v8::Isolate *isolate_;
	bool enabled_;
	std::string path_;
	std::string host_name_;
	std::string file_path_;
};

} // namespace inspector

#endif // SRC_INSPECTOR_AGENT_H_
