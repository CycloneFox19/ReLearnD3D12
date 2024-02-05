//--------------------------------------------------------
// Includes
//--------------------------------------------------------
#include "App.h"

int wmain(int argc, wchar_t** argv, wchar_t** envp)
{
#if defined(DEBUG) || defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif // defined(DEBUG) || defined(_DEBUG)

	// run application
	App app(960, 540);
	app.Run();

	return 0;
}