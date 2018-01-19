import os

rootDir = os.path.dirname(os.path.abspath( __file__))

def FlagsForFile( filename, **kwargs ):
	return {
		'flags': "-x c++ -DBOOST_COROUTINES_NO_DEPRECATION_WARNING -DBOOST_COROUTINE_NO_DEPRECATION_WARNING -DSOL_ENABLE_INTEROP -DSOL_EXCEPTIONS_SAFE_PROPAGATION -DURHO3D_IK -DURHO3D_LOGGING -DURHO3D_LUA -DURHO3D_PHYSICS -DURHO3D_PROFILING -DURHO3D_THREADING -Ivendor/Urho3D/include -Ivendor/Urho3D/include/Urho3D/ThirdParty -Ivendor/Urho3D/include/Urho3D/ThirdParty/Bullet -Ivendor/Urho3D/include/Urho3D/ThirdParty/Lua -Iinclude -Ivendor/Felt/include -Ivendor/LuaCppMsg/include -Ivendor/Urho3D/Source/ThirdParty/LuaJIT/src -Ivendor/sml/include -Ivendor/include -Ivendor/CImg -Ivendor/zstr/src -std=gnu++11 -Wno-invalid-offsetof -march=native -ffast-math -pthread -fdiagnostics-color=auto -O0 -g -g3 -DNDEBUG -std=c++14 -fopenmp -stdlib=libc++ -isystem /usr/include/c++/7 -isystem /usr/include/x86_64-linux-gnu/c++/7".split(" "),
		'include_paths_relative_to_dir': rootDir
	}
