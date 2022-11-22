project "sandbox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	
	files
	{
		"src/**.h",
		"src/**.cpp"
	}

	includedirs
	{
		"%{wks.location}/lazy/src"
	}